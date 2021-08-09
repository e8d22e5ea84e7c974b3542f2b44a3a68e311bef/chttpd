#include "data.h"


/* data_t_init - initialize a data_t struct WITHOUT copying the pointer to heap memory. */
struct data_t *data_t_init(void *data, size_t len)
{
    struct data_t *data_object = (struct data_t*) malloc(sizeof(struct data_t));
    
    data_object->data = data;
    data_object->len = len;

    return data_object;
}


/* Copy a block of memory into heap memory and return a data structure. */
struct data_t *data_t_dup(const void *data, size_t len)
{
    void *heap = (void*) malloc(len);
    memcpy(heap, data, len);

    return data_t_init(heap, len);
}

/* data_t_free - the function to call after you're done with a struct data_t. THIS WILL FREE THE DATA POINTER!!! */
void data_t_free(struct data_t *data)
{
    sfree(data->data);
    sfree(data);
}


