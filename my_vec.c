#include <stdlib.h>
#include <stdint.h>
#include "err.h"
#include "my_vec.h"

#define INIT_BUFFER_SIZE 5000

my_vec_t *my_vec_init()
{
    my_vec_t *vec = (my_vec_t*)malloc(sizeof my_vec_t);

    if (vec == NULL)
        fatal("my_vec_init - malloc didnt allocate\n");

    vec->buff = calloc(INIT_BUFFER_SIZE, sizeof char);

    if (vec->buff == NULL)
        fatal("my_vec_init - calloc didnt allocate\n");

    vec->buff_size = INIT_BUFFER_SIZE;
    vec->occupied_size = 0;
    vec->first_free_space_idx = 0;

    return vec;
}

void my_vec_destruct(my_vec_t *my_vec)
{
    free(my_vec->buff);
    free(my_vec);
}

void my_vec_push_back(my_vec_t *my_vec, char c)
{
    if (my_vec->first_free_space_idx >= my_vec->buff_size)
    { 
        uint64_t new_size;

        if (my_vec->buff_size < UINT64_MAX / 2)
            new_size = 2 * my_vec->buff_size;
        else
            new_size = UINT64_MAX;

        char *temp = my_vec->buff;

        my_vec->buff = calloc(new_size, sizeof char);

        if (vec->buff == NULL)
            fatal("my_vec_push_back - calloc didnt allocate\n");

        my_vec->buff_size = new_size;

        strncpy(my_vec->buff, temp, my_vec->occupied_size);
        free(temp);
    }

    my_vec->buff[my_vec->first_free_space_idx] = c;
    my_vec->first_free_space_idx += 1;
    my_vec->occupied_size += 1;
}


char my_vec_get(my_vec_t *my_vec, uint64_t idx)
{
    if (idx < my_vec->first_free_space_idx)
        return my_vec->buff[idx];
    
    error("my_vec_get - idx out of scope, returning NULL\n");
    return NULL;
}

