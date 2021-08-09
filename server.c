
#include "server.h"

hclient *hclient_init(httpd *server, const char *address, int port, int fd)
{
    hclient *client = (hclient*) malloc(sizeof(hclient));
    
    client->server = server;
    strcpy(client->address, address);
    client->port = port;
    client->fd = fd;

    return client;
}

void hclient_free(hclient *client)
{
    free(client);
}

/* httpd_create - Create an HTTPD instance to handle files on a specific path and port. */ 
httpd *httpd_create(const char *path, int port)
{
    httpd *server = (httpd*) malloc(sizeof(server));

    int fd = 0;
    struct sockaddr_in *saddr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));

    if (!(fd = socket(AF_INET, SOCK_STREAM, 0)))
    {
        perror("The socket could not be created");
        return NULL;
    }

    int _opt = 1;
    saddr->sin_family = AF_INET;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &_opt, sizeof(_opt));
    saddr->sin_addr.s_addr = INADDR_ANY;
    saddr->sin_port = htons(port);

    server->path = strdup(path);
    server->port = port;
    server->fd = fd;
    server->saddr = saddr;

    if (bind(fd, (struct sockaddr*) saddr, sizeof(*saddr)) < 0)
    {
        perror("Error while binding");
        return NULL;
    }

    return server;
}

/* httpd_handle - Process the HTTP request from the client. */
void *httpd_handle(void *_client)
{
    /* Ignore SIGPIPE */
    signal(SIGPIPE, SIG_IGN);

    hclient *client = (hclient*) _client;
    httpd *server = client->server;

    int fd = client->fd;

    /* Initialize all with NULL, for safety reasons! */
    struct data_t *response = NULL;
    struct data_t *post_data = NULL;

    char *top = NULL; // First header--the method--sent by the client.
    char *headers = NULL;

    sarray *request_fields = NULL;
    sarray *files = NULL;
    sarray *uri = NULL;

    char *absolute = NULL;
    char *nameofdir = NULL;

    struct client_http_headers *hdrs = NULL;

    bool cgi_allowed = true;
    bool list_allowed = true;

    bool keep_alive = false;

    do
    {
        /* During a keep-alive, if we abruptly continue, we don't want a memory leak! */
        if (keep_alive)
            clean_up_client_handler();

        response = recvall(fd);
        if (response->data == NULL)
        {
            hsendtext(fd, http_bad_request, "Data was not received.", true);
            break;
        }

        /* For both of these, we use strstr to find how far a certain substring is in a string. */
        /* We then find the size of the beginning of the string to that substring by taking the */
        /* difference between the pointers and casting it to size_t, copying only that amount to a */
        /* newly allocated string. We essentially do the opposite for the POST data.*/

        /* Get the method efficiently. */
        char *beginning = strstr(response->data, "\r\n");
        if (beginning == NULL)
        {
            hsendtext(fd, http_bad_request, "HTTP Not CRLF Delimited.", true);
            break;
        }

        size_t top_len = ((size_t) beginning - (size_t) response->data);
        top = (char*) malloc(top_len);
        memcpy(top, response->data, top_len);

        /* Get all of the headers for proper processing. */

        char *divider = strstr(response->data, "\r\n\r\n");
        if (divider == NULL)
        {
            hsendtext(fd, http_bad_request, "HTTP Headers Not Terminated With CRLF CRLF", true);
            break;
        }

        size_t header_len = ((size_t) divider - (size_t) response->data);
        headers = (char*) malloc(header_len);
        memcpy(headers, response->data, header_len);
    
        
        /* POST data */
        /* Get the first occurance of \r\n\r\n (after HTTP headers) and go past the occurance of \r\n\r\n. */
        char *_post = strstr(response->data, "\r\n\r\n") + 4;

        size_t _post_offset = ((size_t) _post) - ((size_t) response->data);
        /* What is the space between the beginning and the start of the post data? */
        size_t _post_size = response->len - _post_offset;
        /* Therefore, the post data is the entire response length minus that space in-between. */
        
    
        post_data = data_t_init(_post, _post_size);

        /* Request was not formatted correctly. */
        if (top == NULL)
        {
            hsendtext(fd, http_bad_request, "Malformed headers.", true);
            break;
        }

        char *request = top;
        request_fields = split_string(request, " ");

        /* Request was not formatted correctly. */
        /* Oh god, I almost tried to free the thing I was checking to be NULL! */
        if (request_fields == NULL)
        {
            hsendtext(fd, http_bad_request, "Malformed headers.", true);
            break;
        }

        char *method = sarray_get(request_fields, 0);
        
        /* Separate query from file. This can be made less expensive with strstr! */
        sarray *uri = split_string(sarray_get(request_fields, 1), "?");
        char *path = sarray_get(uri, 0);
        char *query = (sarray_get(uri, 1) == NULL) ? "" : sarray_get(uri, 1); 
        
        if (strlen(path) > MAX_URI)
        {
            hsendtext(fd, http_long, "URI is too long!", true);
            break;
        }

        char *http_version = sarray_get(request_fields, 2);

        /* Formatted incorrectly. */
        if (method == NULL || path == NULL || http_version == NULL)
        {
            hsendtext(fd, http_bad_request, "Bad method string.", true);
            break;
        }

        /* Max URI length */
        if (strlen(path) > MAX_URI)
        {
            hsendtext(fd, http_long, "URI too long.", true);
            break;
        }

        char *extension = strrchr(path, '.');

        if (!extension)
            extension = "";

        char newpath[PATH_MAX];
        strncpy(newpath, path, sizeof(newpath));

        uri_decode(newpath, strlen(newpath), newpath);

        /* Is the page a directory? */
        bool defaultp = false;
        bool directory = false;


        /* Containment being the serving directory being appended onto it. */
        /* We need to be extremely careful with our strings. */
        char containment[PATH_MAX];
        size_t c_len = snprintf(containment, sizeof(containment), "%s%s", server->path, newpath);
        if (c_len < 0)
        {
            hsendtext(fd, http_bad_request, "You're trying to do an exploit?", true);
            break;
        }

        /* If we are dealing with a directory. */
        if (ifdir(containment))
        {
            if (lastchar(containment) == '/')
                containment[strlen(containment)-1] = '\0'; // Remove last char, which is /

            strncat(containment, default_page, sizeof(containment)-1-c_len);
            extension = strrchr(default_page, '.');
            defaultp = true; 
        }


        /* If the path is a directory, but the index.html doesn't exist, display the contents. */
        if (defaultp && !file_exists(containment))
        {
            containment[strlen(containment) - sizeof(default_page)+2] = '\0'; // Truncate before default page.
            defaultp = false;
            directory = true;
        }

        char neu[64] = "";
        uri_decode(path, strlen(path), neu);

        if (!file_exists(containment))
        {
            hsendfile(fd, http_not_found, mime_html, not_found_page, true);
            break;
        }


        /* Dangerous exploit to go beyond the html/ folder. */
        char _absolute[PATH_MAX];
        absolute = realpath(containment, _absolute);
        if (absolute == NULL)
            break;

        /* Hmm... I wonder what kind of exploit can be done here, given you have a folder named html elsewhere. */
        if (!strin(absolute, server->path))
        {
            hsendfile(fd, http_forbidden, mime_html, forbidden_page, true);
            break;
        }

        nameofdir = strdup(containment);
        
        if (!directory)
        {
            nameofdir = dirname(nameofdir); 
        }


        files = list_dir(nameofdir);

        
        if (sarray_in(files, ".private"))
        {
            hsendfile(fd, http_error, mime_html, forbidden_page, true);
            break;
        }

        /* Disallow PHP/CGI if specified. */
        if (sarray_in(files, ".nocgi"))
            cgi_allowed = false;
        
        /* Disallow directory listing if specified. */
        if (sarray_in(files, ".nolist"))
            list_allowed = false;

        int rtype = request_type(method);
        switch (rtype)
        {
            case REQUEST_HEAD:
            case REQUEST_GET:
            {
                printf("[%s:%d]: %s %s\n", client->address, client->port, method, path);


                hdrs = process_headers(client->address, containment, query, headers);
                
                //if (hdrs->keep_alive)
                //    keep_alive = true;

                if (strequ(extension, ".php") && cgi_allowed)
                {
                    hsendphp(fd, hdrs, containment, post_data, false, rtype == REQUEST_GET);
                }

                /* Display the contents of the directory if no index.html. */
                else if (ifdir(containment))
                {
                    if (!list_allowed)
                    {
                        hsendfile(fd, http_forbidden, mime_html, forbidden_page, rtype == REQUEST_GET);
                        break;
                    }

                    carray *contents = dir_table(containment, server->path);
                    hsendtext(fd, http_ok, contents->data, true);
                    carray_free(contents);
                }
                else
                { 
                    /* Check if file has been modified since. */
                    if (hdrs->modified_since != NULL)
                    {
                        char last[32] = "";
                        filelastmodifytime(containment, last, sizeof(last));

                        /* The file hasn't changed according to the client's records. */
                        /* Therefore, the client does not want us to send it again. */

                        if (strequ(last, hdrs->modified_since))
                        {
                            hsendtext(fd, http_notmod, containment, false);
                            break;
                        }

                    }

                    /*if (hdrs->ranged)
                    {
                        hsendfileranged(fd, http_ok, ext2mime(extension), containment, hdrs, 
                                        rtype == REQUEST_GET);
                    }
                    else*/
                        hsendfile(fd, http_ok, ext2mime(extension), containment, rtype == REQUEST_GET);
                }

                break; 
            }

            case REQUEST_POST:
            {
                printf("[%s:%d]: POST %s\n", client->address, client->port, path);
                
                hdrs = process_headers(client->address, containment, NULL, headers);

                if (!hdrs->content_length)
                {
                    hsendtext(fd, http_nolength, "Content-Length is required", true);
                    break;
                }

                if (hdrs->content_length > MAX_POST_LEN)
                {
                    hsendtext(fd, http_payload, "POST Payload too large!", true);
                    break; 
                }

                /* This server only supports PHP CGI. */
                if (!strequ(extension, ".php") || !cgi_allowed)
                {
                    hsendtext(fd, http_not_allowed, "CGI Invalid", true);
                    break;
                }

                hsendphp(fd, hdrs, containment, post_data, true, true);
                break;
            }

            default:
                hsendtext(fd, http_not_implemented, "This server does not support your HTTP method.", true); 
        }

    } while (0);

    clean_up_client_handler();

    hclient_free(client);
    close(fd);
}

/* httpd_host - Start listening for incoming connections on a specific server instance. */
int httpd_host(httpd *server)
{
    signal(SIGPIPE, SIG_IGN);
    listen(server->fd, 0);

    while (1)
    {
        struct sockaddr_in client;
        int clientlen = sizeof(client);

        int connection = accept(server->fd, (struct sockaddr*) &client, (socklen_t*) &clientlen);
        uint16_t cport = htons(client.sin_port);

        char ip_address[16] = "";
        get_ip_str((struct sockaddr*) &client, ip_address, sizeof(ip_address));

        hclient *conn = hclient_init(server, ip_address, cport, connection);

        pthread_t handlert;
        pthread_create(&handlert, NULL, httpd_handle, (void*) conn);
    }

}

/* httpd_close - Close the server instance. Free memory. Don't forget to call this! */
void httpd_close(httpd *server)
{
    free(server->path);
    free(server->saddr);
    close(server->fd);
    free(server);
}