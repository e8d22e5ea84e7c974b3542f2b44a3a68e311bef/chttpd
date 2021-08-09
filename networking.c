#include "networking.h"


/* Return a string array of files in a given directory. */
/* If the file is a directory, it will be terminated with a slash (/) */
/* Will return NULL if path does not exist! */
sarray *list_dir(const char *path)
{
    DIR *directory;
    struct dirent *entry;

    directory = opendir(path);
    if (directory == NULL)
        return NULL;
    
    /* You don't want to allocate memory, then just return NULL, forgetting about it. */
    sarray *files = sarray_init();
    
    /* Read all files. */
    while (entry = readdir(directory))
    {
        char *filename = entry->d_name;
        bool isadir = (entry->d_type == DT_DIR); // A directory?

        size_t length = strlen(filename);
        char *realname = (char*) malloc(length + 3); // One byte for the slash and another for zero.
        strcpy(realname, filename);

        /* Add the slash, along with the null-terminator. (Can't be sure of malloc!)*/
        if (isadir)
        {
            /* UNIX special directories. */
            if (strequ(realname, ".") || strequ(realname, ".."))
                continue; 

            strcat(realname, "/");
        }
        
        sarray_add(files, realname);
        free(realname);
    }

    closedir(directory);
    return files;
}

/* Using a script, create an HTML website representing a directory. */
/* You must free the contents of the dynamic string! */
carray *dir_table(const char *path, const char *dir)
{
    carray *content = carray_init();

    const char *title = "<h1>Directory listing of %s:</h1><br>";
    const char *none_found = "<br><h2>No files were found and no index.html.</h2>";
    const char *format = "<a href=\"%s/%s\">%s</a><br>";
    
    char *path_no_containment = strstr(path, dir) + strlen(dir);

    /* Get rid of the trailing / . For this, we need a mutable string. */
    char *notrailing = strdup(path_no_containment);
    notrailing[strlen(notrailing)-1] = '\0';

    sarray *files = list_dir(path);
    char atitle[256];
    snprintf(atitle, sizeof(atitle), title, notrailing);
    carray_add(content, atitle);

    if (!files->size)
        carray_add(content, none_found);

    for (size_t i = 0; i < files->size; i++)
    {
        char text[512] = {0}; // Linux filenames are not more than 256.
        char *filename = sarray_get(files, i);

        snprintf(text, sizeof(text), format, notrailing, filename, filename);
        carray_add(content, text);
    }

    sarray_free(files);
    free(notrailing);

    return content;
}

bool file_exists(const char *path)
{
    return !access(path, F_OK);
}

/* Whether the file is a directory. */
bool ifdir(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

/* File size in bytes. */
size_t file_size(const char *filename)
{
    struct stat st;
    stat(filename, &st);
    return st.st_size;
}


/* Execute a PHP file and return the output, doing CGI as best we can. */
int hsendphp(int fd, struct client_http_headers *hdrs, char *phpfile, struct data_t *block,
            bool post, bool body)
{    
    char randpath[16];
    
    char postpath[sizeof(randpath) + sizeof(TMP) + 8];
    //char headerpath[sizeof(postpath)];
    char datapath[sizeof(postpath)];

    char *command;

    /* Random filename to prevent collisons. We need somewhere to store the POST data. */
    /* I don't want to deal with fork(), pipe(), and execlpe() */
    /* It is also used to store the response of PHP or something else to send back. */

    /* Problem: /tmp/ is limited by swap and ram space typically on a tmpfs. */
    /* You must use set TMP to be disk space if you need to upload large files! */

    randtext(randpath, sizeof(randpath));

    /* Temporary files */
    sprintf(postpath, "%s%s_post", TMP, randpath); // For post from the client
    sprintf(datapath, "%s%s_data", TMP, randpath); // For response from PHP

    if (post)
    {
        /* Write the POST request to a file that we can cat into php-cgi/others */
        FILE *fp = fopen(postpath, "w+");
        fwrite(block->data, 1, block->len, fp);

        /* We have read some part of the client's POST, but not all of it. */
        /* Luckily, the client told us all of the bytes it will send, so let's */
        /* recv until we make up the difference, writing it to the temporary POST file. */
        /* It isn't as slow as one would expect, since /tmp/ is basically a ramdisk. */

        recvuntil(fd, fp, hdrs->content_length - block->len);
        fclose(fp);

        command = (char*) malloc(sizeof(php_post_command) + strlen(phpfile) + 1024);
        sprintf(command, php_post_command, postpath, phpfile, hdrs->filename,
                hdrs->filename, hdrs->content_length,
                hdrs->content_type, (hdrs->cookies == NULL) ? "" : hdrs->cookies,
                hdrs->address);
    }
    else
    {
        command = (char*) malloc(sizeof(php_get_command) + strlen(phpfile) + 1024);
        sprintf(command, php_get_command, hdrs->query, phpfile, hdrs->filename,
                hdrs->filename, hdrs->content_length,
                hdrs->content_type, (hdrs->cookies == NULL) ? "" : hdrs->cookies,
                hdrs->address);
    }

    FILE *process = popen(command, "r");
    FILE *data = fopen(datapath, "w+b");

    /* Bug: PHP-generated headers cannot be larger than 4096 bytes, which shouldn't be a problem */
    char headers[4096];
    size_t hdr_read = 0;
    size_t header_len = 0;
    
    /* Get header size by seeing how far until \r\n\r\n it is. */
    hdr_read = fread(headers, 1, sizeof(headers), process);
    header_len = (size_t) strstr(headers, "\r\n\r\n")+4 - (size_t) headers;
    fwrite(headers, 1, hdr_read, data);
    /* Also, write this file to the temporary file. */

    /* PHP could send a Status: header to change the status code. */
    char _status[256] = "";
    char *status = get_status(headers, _status);

    /* If there's more data, write it as well. */
    while (!feof(process))
    {
        hdr_read = fread(headers, 1, sizeof(headers), process);
        fwrite(headers, 1, hdr_read, data);
    }

    pclose(process);
    fclose(data);

    /* Writing a file and then re-reading it is expensive but it helps to more efficiently. */
    /* Obtain the filesize and then subtract the header length to get the Content-Length. */
    /* As opposed to holding the entire response in memory (php may not always return text/html) */

    size_t fsize = file_size(datapath);
    size_t responsesize = fsize - header_len;

    response_gen(fd, (status == NULL) ? http_ok : status, responsesize, "text/html", NULL, true);

    char buffer[2048];

    /* We need to send the file again, now that we have the full size. */
    /* Why does PHP NOT SEND THE CONTENT-LENGTH FOR US???? */

    FILE *cgi = fopen(datapath, "rb");

    size_t len = 0;
    while (!feof(cgi))
    {
        len = fread(buffer, 1, sizeof(buffer), cgi);
        send(fd, buffer, len, 0);
    }

    /* Clean up. */
    fclose(cgi);

    if (post)
        remove(postpath);

    remove(datapath);

    free(command);
}

/* Send text as an HTTP response with a specific response like hsendfile. */
/* If status is Not Modified, message contains a filename which will return the */
/* previous modification date. This was done to prevent rewriting the whole program. */
int hsendtext(int fd, const char *status, const char *message, bool body)
{
    bool lmodified = strequ(status, http_notmod);
    size_t msg_len = strlen(message);
    response_gen(fd, status, msg_len, mime_html, lmodified ? message : NULL, false);
    
    if (body)
        send(fd, message, msg_len, 0);
}

/* Send a file using Content-Length and a specific response (e.g., 200 or 404) */
int hsendfile(int fd, const char *response, const char *mime, const char *filename, bool body)
{
    signal(SIGPIPE, SIG_IGN);
    FILE *fp = fopen(filename, "rb");

    if (!fp)
        return ENOENT;

    size_t fsize = file_size(filename);

    char headers[1024] = "";
    char buffer[FILE_BUFFER_SIZE] = "";

    size_t bytes_read = 0;
    size_t total_bytes = 0;

    response_gen(fd, http_ok, fsize, mime, filename, false);


    /* HEAD support */
    /* Don't send content if body isn't true (HEAD REQUEST.) */
    if (body)
    {
        while (!feof(fp))
        {
            bytes_read = fread(buffer, 1, sizeof(buffer), fp);
            if (send(fd, buffer, bytes_read, MSG_NOSIGNAL) < 0)
                break;
        }
    }

    fclose(fp);
}


/* Send a file, but with a specific offset and length. No multipart range support! */
int hsendfileranged(int fd, const char *response, const char *mime, const char *filename, 
                    struct client_http_headers *hdrs, bool body)
{
    signal(SIGPIPE, SIG_IGN);
    FILE *fp = fopen(filename, "rb");

    if (!fp)
        return ENOENT;

    size_t amount = hdrs->range_end; // Amount to go up to
    size_t fsize = file_size(filename);

    if (!hdrs->range_end)
        amount = fsize;


    /* If the ranges don't make sense. */
    /*if (hdrs->range_begin > fsize || amount > fsize || 
        hdrs->range_begin > amount)
    {
        hsendtext(fd, http_bad_range, "Your range doesn't make sense!", true);
        fclose(fp);
        return 0; 
    }
    */

    fseek(fp, hdrs->range_begin, SEEK_SET);
    char headers[1024] = "";
    char buffer[FILE_BUFFER_SIZE] = "";

    size_t bytes_read = 0;
    size_t total_bytes = 0;

    /* Partial content requires a special header. */
    char ranged[4096] = "";

    size_t ranged_size = snprintf(ranged, sizeof(ranged), http_ranged_response, 
                                amount - hdrs->range_begin,
                                hdrs->range_begin, amount-1, 
                                fsize, mime);
    
    send(fd, ranged, ranged_size, 0);

    /* HEAD support */
    /* Don't send content if body isn't true (HEAD REQUEST.) */

    errno = 0;
    if (body)
    {
        while (total_bytes < amount)
        {
            bytes_read = fread(buffer, 1, sizeof(buffer), fp);
            if ((bytes_read = send(fd, buffer, bytes_read, MSG_NOSIGNAL)) == -1)
            {
                printf("BroKE1");
                perror("WAS?");
                break;
            }

            if (bytes_read == 0)
                break;

            write(1, buffer, bytes_read);
            printf("READ: %lu\n", bytes_read);
            perror("LOLOL");

            total_bytes += bytes_read;
        }
    }

    fclose(fp);
}

/* Receive, then write to a stream until a certain amount of bytes have been transferred. */
/* You should know when from Content-Length on the client side. */
void recvuntil(int from, FILE *to, size_t when)
{
    size_t aread = 0;
    size_t len = 0;

    char buffer[4096] = "";

    while (len != when)
    {
        aread = recv(from, buffer, sizeof(buffer), 0);
        fwrite(buffer, 1, aread, to);
        len += aread;
    }
}

/* Receive all data on the buffer from an fd until EWOULDBLOCK. struct data_t* must be freed!*/
struct data_t *recvall(int fd)
{
    struct data_t *data = (struct data_t*) malloc(sizeof(struct data_t));
    
    size_t len = 0;
    char buffer[RECVALL_BUFFER] = "";

    len = recv(fd, buffer, sizeof(buffer), 0);
    if (len < 0)
    {
        perror("recv");
        return NULL;
    }

    data->data = malloc(len);
    memcpy(data->data, buffer, len);
    data->len = len; 

    return data;
}

/* Convert sockaddr into a string ipv4/v6 address. Use ntohs or htons for the (unsigned uint16_t) port. */
char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen)
{
    switch (sa->sa_family) 
    {
        case AF_INET:
            inet_ntop(AF_INET, &(((struct sockaddr_in *)sa)->sin_addr),
                    s, maxlen);
            break;

        default:
            strncpy(s, "Unknown AF", maxlen);
            return NULL;
    }

    return s;
}


