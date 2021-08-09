#ifndef SERVER_H
#define SERVER_H

#include <stddef.h>
#include <wchar.h>
#include <locale.h>

#include <pthread.h>
#include <sys/socket.h>
#include <sys/signal.h>

#include <libgen.h>
#include <arpa/inet.h>
#include "networking.h"

#include "config.h"
#include "text.h"
#include "sarray.h"

#include "http.h"

#define clean_up_client_handler() do {  \
    free(top);                          \
    free(headers);                      \
    sarray_free(request_fields);        \
    sarray_free(files);                 \
    sarray_free(uri);                   \
    data_t_free(response);              \
    sfree(nameofdir);                   \
    sfree(post_data);                   \
    free_headers(hdrs); } while (0);    \

struct httpd
{
    char *path;
    int port;
    int fd;
    struct sockaddr_in *saddr;
};

typedef struct httpd httpd;

struct hclient
{
    httpd *server;
    char address[16];
    uint16_t port;
    int fd;
};

typedef struct hclient hclient;



hclient *hclient_init(httpd *server, const char *address, int port, int fd);
httpd *httpd_create(const char *path, int port);
int httpd_host(httpd *server);
void *httpd_handle(void *_client);
void httpd_close(httpd *server);


#endif