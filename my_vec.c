#include <stdlib.h>
#include <stdint.h>
#include "err.h"
#include "my_vec.h"

#define INIT_VEC_SIZE 4096
#define BUFF_SIZE 9

my_vec_t *my_vec_init()
{
    my_vec_t *vec = (my_vec_t*)malloc(sizeof(my_vec_t));

    if (vec == NULL)
        fatal("my_vec_init - malloc didnt allocate\n");

    vec->buff = calloc(INIT_VEC_SIZE, sizeof(char));

    if (vec->buff == NULL)
        fatal("my_vec_init - calloc didnt allocate\n");

    vec->buff_size = INIT_VEC_SIZE;
    vec->occupied_size = 0;
    vec->first_free_space_idx = 0;

    return vec;
}

void my_vec_destruct(my_vec_t *my_vec)
{
    free(my_vec->buff);
    free(my_vec);
}

// Function reallocates vector
void realocate_my_vec(my_vec_t *my_vec)
{
    uint64_t new_size;

    if (my_vec->buff_size < UINT64_MAX / 2)
        new_size = 2 * my_vec->buff_size;
    else
        new_size = UINT64_MAX;

    char *temp = my_vec->buff;

    my_vec->buff = calloc(new_size, sizeof(char));

    if (my_vec->buff == NULL)
        fatal("my_vec_push_back - calloc didnt allocate\n");

    my_vec->buff_size = new_size;

    strncpy(my_vec->buff, temp, my_vec->occupied_size);
    free(temp);
    
}

void my_vec_push_back(my_vec_t *my_vec, char c)
{
    if (my_vec->first_free_space_idx >= my_vec->buff_size)
    { 
        realocate_my_vec(my_vec);
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
    return '\0';
}

// Function copies characters in buff starting at first free place in vec.
// It handles reallocation when not enough space for all characters in buff
// and also handles null terminated characters, since each buff is ended with 
// termination character thus we want to copy buff_size - 1 characters to vec.
void push_back_str_to_vec(my_vec_t *vec, char *buff, ssize_t buff_size)
{
    if (vec->occupied_size + buff_size >= vec->buff_size)
        realocate_my_vec(vec);
    
    strncpy(vec->buff + vec->first_free_space_idx, buff, buff_size + 1);
    vec->first_free_space_idx += (buff_size);
    vec->occupied_size += (buff_size);
}

// This function reads stdin to read_buffer of constant size, and then it copies
// data to reasizeable vec
void my_vec_read_stdin_with_buffor(my_vec_t *my_vec)
{
    static char read_buff[BUFF_SIZE];
    memset(read_buff, 0, sizeof(read_buff));
    size_t buff_size = BUFF_SIZE;
    ssize_t nbr_of_bytes_read = 0;

    while (fgets(read_buff, buff_size, stdin) != NULL) 
    {
        nbr_of_bytes_read = strlen(read_buff);
        push_back_str_to_vec(my_vec, read_buff, nbr_of_bytes_read);


        printf("%zu characters were read.\n",nbr_of_bytes_read);
        printf("You typed: %s",read_buff);
        if (read_buff[BUFF_SIZE - 1] == '\0' || read_buff[1] == '\0')
            printf("\n-----null terminator was added-----\n");

        memset(read_buff, 'p', sizeof(read_buff));
    }
    printf("\n");
    my_vec_push_back(my_vec, '\0');
    printf("my_vec: %s\n", my_vec->buff);
//    printf("Po petli\n"); 
}