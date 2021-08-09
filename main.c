#include <stdio.h>

#include "config.h"
#include "server.h"

int main(int argc, char **argv, char **envp)
{
    if (!getuid())
    {
        fprintf(stderr, "DO NOT RUN AS ROOT. ARE YOU STUPID?\n");
        return -1;
    }

    system("mkdir " TMP " >& /dev/null"); // Create temporary directory
    
    const int port = (argv[1] == NULL) ? HOSTING_PORT : atoi(argv[1]);
    const char *directory = (argv[2] == NULL) ? DIRECTORY : argv[2];

    if (port > 65535)
    {
        fprintf(stderr, "The port (%d) is invalid. Please choose a number 80-65535.\n", port);
        return 1;
    }

    if (!ifdir(directory))
    {
        fprintf(stderr, "The hosting directory (%s) does not exist.\n", directory);
        return 2;
    }

    printf("Starting the HTTP server...\n");
    printf("Hosting %s/ at 127.0.0.1:%d...\n", directory, port);
    
    httpd *server = httpd_create(directory, port);
    if (server == NULL)
    {
        fprintf(stderr, "Exiting due to fatal error...\n");
        return 3;
    }

    httpd_host(server);
    httpd_close(server);

    return 0;
}