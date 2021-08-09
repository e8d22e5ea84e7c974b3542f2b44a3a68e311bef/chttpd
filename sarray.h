#ifndef SARRAY_H
#define SARRAY_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define NC(x) null_check(x)
#define DEFAULT_CAPACITY 25

/* sarray encourages expensive splits. Consider using strchr, strstr, and strrchr. */

struct sarray
{
    char **data;
    size_t size;
    size_t capacity; 
};

struct carray
{
    char *data;
    size_t size;
    size_t capacity;
};

typedef struct carray carray;
typedef struct sarray sarray;

void sfree(void *ptr);

carray *carray_init();
void carray_add(carray *array, const char *data);
void carray_free(carray *array);


sarray *sarray_init();
void sarray_add(sarray *array, const char *item);
void sarray_free(sarray *array);
sarray *sarray_slice(sarray *array, size_t indices);

sarray *split_string(const char *string, const char *delimiter);

char *sarray_get(sarray *array, size_t index);
char *sarray_end(sarray *array);
char *sarray_begin(sarray *array);

char *null_check(char *string);
void sarray_print(sarray *array);

bool sarray_in(sarray *array, char *item);

#endif