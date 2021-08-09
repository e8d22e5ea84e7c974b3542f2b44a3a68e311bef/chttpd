#ifndef HTTP_H
#define HTTP_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <libgen.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>

#include "config.h"
#include "sarray.h"
#include "text.h"


void response_gen(int fd, const char *status, size_t len, const char *type, const char *filename,
                bool terminate);


void currenttime(char *result, size_t len);
void filelastmodifytime(const char *filename, char *result, size_t len);
void timegen(struct tm *info, char *mut, size_t len);

char *get_status(char *headers, char *mut);
int request_type(const char *path);

/* Request methods (to eliminate confusion). Most of these we don't have to support. */
enum request_types
{
    REQUEST_GET,
    REQUEST_POST,
    REQUEST_HEAD,
    REQUEST_UNKNOWN = -1
};


/* Useful container for the HTTP headers that will be sent in the client's request. */
struct client_http_headers
{
    char address[16];

    enum request_types method; // GET, POST, etc
    char *query;

    /* Content-Length*/
    size_t content_length; 
    
    /* Content-Type: mime,charset,boundary */
    char *content_type;
    char *ct_charset;
    char *ct_boundary; 

    /* Range */
    bool ranged;
    size_t range_begin;
    size_t range_end;

    /* Cookies sent from the client. */
    char *cookies;

    bool do_not_track;
    char *content_encoding;
    char *user_agent;

    char *path;
    char *filename;

    bool keep_alive;

    char *modified_since;

};

struct client_http_headers *process_headers(char *addr, char *path, char *query, char *headers);
void free_headers(struct client_http_headers *headers);

static const char php_post_command[] =
"(cat %s | "
"SCRIPT_FILENAME=\"%s\" "
"SCRIPT_NAME=\"%s\" "
"REQUEST_URI=\"%s\" "
"REQUEST_METHOD=POST "
"SERVER_NAME=\"" SERVER_NAME "\" "
"SERVER_PROTOCOL=HTTP/1.1 "
"CONTENT_LENGTH=%lu "
"CONTENT_TYPE=\"%s\" "
"REDIRECT_STATUS=200 "
"HTTP_COOKIE=\"%s\" "
"REMOTE_ADDR=%s "
"php-cgi) 2> /dev/null";

static const char php_get_command[] =
"QUERY_STRING=\"%s\" "
"SCRIPT_FILENAME=\"%s\" "
"SCRIPT_NAME=\"%s\" "
"REQUEST_URI=\"%s\" "
"REQUEST_METHOD=GET "
"SERVER_NAME=\"" SERVER_NAME "\" "
"SERVER_PROTOCOL=HTTP/1.1 "
"CONTENT_LENGTH=%lu "
"CONTENT_TYPE=\"%s\" "
"REDIRECT_STATUS=200 "
"HTTP_COOKIE=\"%s\" "
"REMOTE_ADDR=%s "
"php-cgi 2> /dev/null";

static const char http_base_response[] =
"HTTP/1.1 %s\r\n"
"Server: " SERVER_NAME "\r\n"
"Date: %s\r\n"
"Last-Modified: %s\r\n"
"Accept-Ranges: bytes\r\n"
"Content-Length: %lu\r\n"
"Content-Type: %s\r\n"
"Connection: Closed\r\n\r\n";

static const char http_ranged_response[] =
"HTTP/1.1 206 Partial Content\r\n"
"Server: " SERVER_NAME "\r\n"
"Accept-Ranges: none\r\n"
"Content-Length: %lu\r\n"
"Content-Range: bytes %lu-%lu/%lu\r\n"
"Content-Type: %s\r\n"
"Connection: Closed\r\n\r\n";


/* Successes */
static const char http_ok[] = "200 OK";
static const char http_partial[] = "206 Partial Content";

/* Information */
static const char http_notmod[] = "304 Not Modified";

/* Server Errors */
static const char http_error[] = "500 Internal Server Error";
static const char http_not_implemented[] = "501 Not Implemented";
static const char http_version[] = "505 HTTP Version Not Supported";

/* Errors */
static const char http_bad_request[] = "400 Bad Request";
static const char http_not_found[] = "404 Not Found";
static const char http_forbidden[] = "403 Forbidden";
static const char http_long[] = "414 URI Too Long";
static const char http_bad_range[] = "416 Range Not Satisfiable";
static const char http_not_allowed[] = "405 Method Not Allowed";
static const char http_timeout[] = "408 Request Timeout";
static const char http_nolength[] = "411 Length Required";
static const char http_payload[] = "413 Payload Too Large";
static const char http_ratelimit[] = "429 Too Many Requests";
static const char http_headerlong[] = "431 Request Header Fields Too Large";

#endif