#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

#define HOSTING_PORT 8080
#define DIRECTORY "html"
#define TMP "/tmp/chttpd/"
#define SERVER_NAME "Custom C HTTP Server"

#define CGI true

#define MAX_GET_LEN 2048
#define MAX_POST_LEN GIGABYTE
#define MAX_URI 64
#define MAX_HEADERS 2048
#define MAX_FILESIZE GIGABYTE

#define FORBIDDEN_FILE ".private"

static const unsigned long KILOBYTE = 1024;
static const unsigned long MEGABYTE = 1048576;
static const unsigned long GIGABYTE = 1073741824;

static const char default_page[] = "/index.html";
static const char not_found_page[] = DIRECTORY "/404.html";
static const char forbidden_page[] = DIRECTORY "/403.html";


#endif