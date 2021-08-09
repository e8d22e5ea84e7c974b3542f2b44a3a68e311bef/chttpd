#ifndef DATA_H
#define DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sarray.h"


/* Simple container for a non-zero terminated string/block of memory. */
struct data_t
{
    void *data;
    size_t len;
};

typedef struct data_t data_t;

void data_t_free(struct data_t *data);
struct data_t *data_t_init(void *data, size_t len);
struct data_t *data_t_dup(const void *data, size_t len);


#endif