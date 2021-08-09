#include "http.h"

/* Get the request type enumeration from a method. */
int request_type(const char *method)
{
    int type = 0;

    if (strequ(method, "GET"))
        type = REQUEST_GET;

    else if (strequ(method, "POST"))
        type = REQUEST_POST;

    else if (strequ(method, "HEAD"))
        type = REQUEST_HEAD;

    else
        type = REQUEST_UNKNOWN;

    return type;
}

/* Return the query string. */
char *get_request(const char *path)
{
    if (!strin("?", path))
        return "";

    return strstr(path, "?") + 1;
}

/* Convert headers into a processable structure. Pass the first element of splitting */
/* the entire whole request by CRLFCRLF (rnrn). Include the method. */
struct client_http_headers *process_headers(char *addr, char *path, char *query, char *headers)
{
    /* Create lengthy heap-allocated struct used to contain this mess. */
    struct client_http_headers *hheaders =
        (struct client_http_headers *) malloc(sizeof(struct client_http_headers));

    /* Initialize headers. */

    strncpy(hheaders->address, addr, sizeof(hheaders->address));

    hheaders->content_encoding = NULL;
    hheaders->content_length = 0;
    hheaders->content_type = NULL;

    hheaders->cookies = NULL;

    hheaders->ct_boundary = NULL;
    hheaders->ct_charset = NULL;
    hheaders->do_not_track = false;
    hheaders->method = REQUEST_UNKNOWN;
    hheaders->range_begin = 0;
    hheaders->range_end = 0;
    hheaders->ranged = false;
    hheaders->user_agent = NULL;
    hheaders->keep_alive = false;
    hheaders->modified_since = NULL;

    hheaders->path = strdup(path);
    hheaders->query = query;
    hheaders->filename = strdup(path);
    hheaders->filename = basename(hheaders->filename);
    /* basename does not return a new pointer? If it does, then memory leak! */

    sarray *header = split_string(headers, "\r\n");

    /* Split the client's request line by line */

    /* Skip the first header. We don't want the method. */
    for (size_t i = 1; i < header->size; i++)
    {
        char header_key[32] = "";
        char *separator = strchr(sarray_get(header, i), ':');
        if (separator == NULL)
            continue;

        /* I'm anxious about this. I know there's an exploit somewhere... */
        size_t header_key_size = ((size_t)separator - (size_t)sarray_get(header, i));

        /* Overflow protection. */
        if (header_key_size > sizeof(header_key)-1)
            continue;

        memcpy(header_key, sarray_get(header, i), header_key_size);

        char *header_value = rwhitespace(separator + 1); // Skip colon and space(s).

        char *__boundary;

        if (strequ(header_key, "Content-Length"))
            hheaders->content_length = strtoul(header_value, &__boundary, 10);

        if (strequ(header_key, "Content-Type") && hheaders->content_type == NULL)
            hheaders->content_type = strdup(header_value);

        if (strequ(header_key, "Cookie"))
            hheaders->cookies = strdup(header_value);

        if (strequ(header_key, "If-Modified-Since"))
            hheaders->modified_since = strdup(header_value);

        if (strequ(header_key, "Range"))
        {
            char *range = strstr(header_value, "bytes=") + (sizeof("bytes=") - 1);
            if (range == NULL)
                return NULL;

            sarray *ranges = split_string(range, "-");
            size_t mark = 0;

            char *all = "0";
            char *_low = sarray_get(ranges, 0);
            char *_high = sarray_get(ranges, 1);

            if (_low == NULL)
            {
                sarray_free(ranges);
                continue;
            }

            /* If there is no high, read until the end of the file. */
            /* We represent this as end being zero. */

            if (_high == NULL)
            {
                _high = all;
            }

            size_t low = strtoul(_low, NULL, 10);
            size_t high = strtoul(_high, NULL, 10);

            hheaders->ranged = true;
            hheaders->range_begin = low;
            hheaders->range_end = high;

            sarray_free(ranges);
        }

        if (strequ(header_key, "Connection"))
        {
            if (strequ(header_value, "keep-alive"))
            {
                hheaders->keep_alive = true;
            }
        }
    }

    return hheaders;
}

/* Free a header struct and their many malloc'd members. */
void free_headers(struct client_http_headers *headers)
{
    if (headers == NULL)
        return;

    sfree(headers->content_encoding);
    sfree(headers->ct_boundary);
    sfree(headers->ct_charset);
    //sfree(headers->filename);
    sfree(headers->path);
    sfree(headers->user_agent);

    sfree(headers->cookies);
    sfree(headers->modified_since);

    sfree(headers);
}

/* Take a time struct and convert it to the format that the HTTP protocol uses. */
void timegen(struct tm *info, char *mut, size_t len)
{
    strftime(mut, len, "%a, %d %b %Y %H:%M:%S %Z", info);
}

/* When was a file last modified, in HTTP format? */
void filelastmodifytime(const char *filename, char *result, size_t len)
{
    struct stat path_stat;
    stat(filename, &path_stat);
    timegen(gmtime(&(path_stat.st_ctim)), result, len);
}

/* What's the current time in HTTP format? */
void currenttime(char *result, size_t len)
{
    time_t now = time(NULL);
    struct tm *info = gmtime(&now); // Greenwich Mean Time
    timegen(info, result, len);
}

/* Generate and send the headers that the server will send back. You still need to send data!*/
void response_gen(int fd, const char *status, size_t len, const char *type, const char *filename,
                  bool terminate)
{
    /* The date. */

    char date[32];
    char lastmodified[32];

    currenttime(date, sizeof(date));

    /* If there is no filename, set the Last-Modified header as the current time. */
    /* Because we don't know! -- PHP just generated it! */

    if (filename == NULL)
        strncpy(lastmodified, date, sizeof(lastmodified));
    else
        filelastmodifytime(filename, lastmodified, sizeof(lastmodified));

    char response[2048];
    size_t wrote = snprintf(response, sizeof(response),
                            http_base_response, status, date,
                            lastmodified, len, type);

    /* PHP may have additional headers it wants to send, too! So cut off the \r\n\r\n*/
    if (terminate)
        wrote = wrote - 2;

    send(fd, response, wrote, 0);
}

/* From the headers of PHP/CGI, get the status. If no status, we return NULL and you should */
/* send a 200 OK! */
char *get_status(char *headers, char *mut)
{
    if (!strin(headers, "Status:"))
        return NULL;

    char *begin = strstr(headers, "Status:") + (sizeof("Status:"));
    char *end = strstr(begin, "\r\n");
    size_t len = (size_t)end - (size_t)begin;

    memcpy(mut, begin, len);
    return mut;
}
