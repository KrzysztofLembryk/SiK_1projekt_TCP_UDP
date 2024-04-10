#ifndef MY_VEC_H
#define MY_VEC_H

#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <inttypes.h>

// my_vec_t struct is a wrapper around char array, that takes care of memory
// management, and provides API for safe putting and getting elements from array
// --> buff_size - allocated memory for buffer (in bytes since its char buffer)
// --> occupied_size - how many bytes in buffer is already taken
// --> first_free_space_idx - first available idx to put new element
typedef struct 
{
    char *buff;
    uint64_t buff_size;
    uint64_t occupied_size;     
    uint64_t first_free_space_idx;

} my_vec_t;

my_vec_t *my_vec_init();

void my_vec_destruct(my_vec_t *my_vec);

void my_vec_push_back(my_vec_t *my_vec, char c);

void my_vec_get(my_vec_t *my_vec, uint64_t idx);

void my_vec_read_stdin(my_vec_t *my_vec);


#endif