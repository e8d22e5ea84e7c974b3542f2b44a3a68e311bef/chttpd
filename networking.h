#ifndef NETWORKING_H
#define NETWORKING_H

/* Do not change. */
#define FILE_BUFFER_SIZE 8192
#define RECVALL_BUFFER 8192

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <libgen.h>
#include <arpa/inet.h>

#include "http.h"
#include "config.h"
#include "data.h"
#include "sarray.h"
#include "text.h"

struct data_t *recvall(int fd);
void recvuntil(int from, FILE *to, size_t when);

bool file_exists(const char *filename);
size_t file_size(const char *filename);
bool ifdir(const char *path);

sarray *list_dir(const char *path);
carray *dir_table(const char *path, const char *dir);

int hsendfile(int fd, const char *response, const char *mime, const char *filename, bool body);

int hsendfileranged(int fd, const char *response, const char *mime, const char *filename, 
                    struct client_http_headers *hdrs, bool body);

int hsendtext(int fd, const char *response, const char *message, bool body);
int hsendphp(int fd, struct client_http_headers *hdrs, char *phpfile, struct data_t *block, 
            bool post, bool body);


char *get_ip_str(const struct sockaddr *sa, char *s, size_t maxlen);



#endif