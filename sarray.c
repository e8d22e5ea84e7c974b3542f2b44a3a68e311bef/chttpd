#include "sarray.h"

/* Initialize string array with data. If data is NULL, don't add it. */
sarray *sarray_init()
{
    sarray *array = (sarray*) malloc(sizeof(sarray));
    
    array->capacity = DEFAULT_CAPACITY;
    array->data = (char**) calloc(array->capacity, sizeof(char*));
    array->size = 0;

    return array;
}

/* Add a char ** and its length to an sarray. */
void sarray_array(sarray *array, char **data, size_t size)
{
    for (size_t i = 0; i < size; i++)
    {
        sarray_add(array, data[i]);
    }
}

/* Append src to dest sarray. */
void sarray_sarray(sarray *dest, sarray *src)
{
    sarray_array(dest, src->data, src->size);
}


/* Add a string onto the array. */
void sarray_add(sarray *array, const char *item)
{
    if (array->size == array->capacity)
    {
        array->capacity = array->capacity * 2;
        array->data = (char**) realloc(array->data, sizeof(char*) * array->capacity);
    }

    array->data[array->size] = strdup(item);
    array->size++;
}

/* Slice the first indices off as specificed by that variable. If you go over the size, it returns NULL. */
/* This returns a new pointer to an allocated struct. You must free the original one!!! */
sarray *sarray_slice(sarray *array, size_t indices)
{
    if (indices > array->size)
        return NULL;
    
    sarray *new_array = sarray_init();
    sarray_array(new_array, array->data+indices, array->size-indices);
    return new_array;
}

/* sarray_free - Free the array and its allocated items. */
void sarray_free(sarray *array)
{
    if (array == NULL)
        return;

    for (size_t i = 0; i < array->size; i++)
    {
        sfree(array->data[i]);
    }

    if (array->data != NULL)
        sfree(array->data);
    
    sfree(array);
}

/* Split a string by a delimiter, returning an sarray. */
/* If no such delimiter was found, NULL is returned. */
sarray *split_string(const char *string, const char *delimiter)
{
    char *heap = strdup(string);
    char *token = strtok(heap, delimiter);

    if (token == NULL)
    {
        free(heap);
        return NULL;
    }

    sarray *vector = sarray_init();

    /* Ugly one-liner. */
    for (; token != NULL; sarray_add(vector, token), token = strtok(NULL, delimiter));

    free(heap);
    return vector;
}

/* Get the string at the specific index. Returns NULL if the index does not exist or was deleted. */
char *sarray_get(sarray *array, size_t index)
{
    if (index > (array->size - 1))
        return NULL;

    return array->data[index];
}

/* Obtain the end of a string array. */
char *sarray_end(sarray *array)
{
    return sarray_get(array, array->size-1);
}

/* (redundant) Obtain the beginning of a string array. */
char *sarray_begin(sarray *array)
{
    return sarray_get(array, 0);
}

/* Safely return a string that may be NULL. */
char *null_check(char *string)
{
    return (string == NULL) ? "(null)" : string; 
}


/* Print a string array. */
void sarray_print(sarray *array)
{
    printf("=== Array ===\n");
    for (size_t i = 0; i < array->size; i++)
        printf("[%d]: %s\n", i, sarray_get(array, i));
}


bool sarray_in(sarray *array, char *item)
{
    if (array == NULL || item == NULL)
        return false;


    for (size_t i = 0; i < array->size; i++)
    {  
        char *element = sarray_get(array, i);
        if (element == NULL)
            continue;

        if (!strcmp(element, item))
            return true;
    }

    return false;
}

/* carray */

/* A dynamically increasing string/character array. */
carray *carray_init()
{
    carray *array = (carray*) malloc(sizeof(carray));
    
    array->capacity = 1024;
    array->data = (char*) calloc(array->capacity, 1);
    array->size = 0;

    return array;
}

/* Append a substring to the string. */
/* carray is string-based, so you need a null-terminator on data. If it's not there, add it! */
/* Want a carray for binary data? Use struct data_t! */
void carray_add(carray *array, const char *data)
{
    size_t length = strlen(data);

    /* If the length goes over the capacity. It's cheaper to just have a capacity. */
    /* Instead of allocating every single time we append to the string. */

    if ((array->size + length) > array->capacity)
    {
        array->capacity += length;
        array->capacity = array->capacity * 2;
        array->data = (char*) realloc(array->data, array->capacity);
    }

    strcat(array->data, data);
}

/* Free the dynamic string. */
void carray_free(carray *array)
{
    if (array == NULL)
        return;

    free(array->data);
    free(array);
}


/* Free a pointer, but don't crash and burn over a NULL pointer! */
void sfree(void *ptr)
{
    if (ptr == NULL)
        return;

    free(ptr);
}
