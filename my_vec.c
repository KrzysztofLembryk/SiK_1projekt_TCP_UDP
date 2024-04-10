#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "err.h"
#include "my_vec.h"

#define INIT_VEC_SIZE 4096
#define BUFF_SIZE 1024

// -----HELPER FUNCTIONS-----

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

    // printf("-----MAKING REALLOCATION-----\n");
    // printf("Old buffsize -> %lu, New buffsize -> %lu\n", my_vec->buff_size, new_size);

    my_vec->buff_size = new_size;

    strncpy(my_vec->buff, temp, my_vec->occupied_size);
    free(temp);
}

// Function copies characters in buff starting at first free place in vec.
// It handles reallocation when not enough space for all characters in buff
// and also handles null terminated characters, since each buff is ended with 
// termination character thus we want to copy buff_size - 1 characters to vec.
void push_back_str_to_vec(my_vec_t *vec, char *buff, size_t buff_size)
{
    if (vec->occupied_size + buff_size >= vec->buff_size)
        realocate_my_vec(vec);
    
    strncpy(vec->buff + vec->first_free_space_idx, buff, buff_size);

    vec->first_free_space_idx += (buff_size);
    vec->occupied_size += (buff_size);
}

// -----MY_VEC LIB IMPLEMENTATION-----

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

void my_vec_push_back(my_vec_t *my_vec, char c)
{
    if (my_vec->first_free_space_idx >= my_vec->buff_size)
        realocate_my_vec(my_vec);

    my_vec->buff[my_vec->first_free_space_idx] = c;
    my_vec->first_free_space_idx += 1;
    my_vec->occupied_size += 1;
}

char my_vec_get(my_vec_t *my_vec, uint64_t idx)
{
    if (idx < my_vec->first_free_space_idx)
        return my_vec->buff[idx];
    
    error("my_vec_get - idx out of scope, returning termination symbol\n");
    return '\0';
}

void my_vec_print(my_vec_t *my_vec)
{
    for (size_t i = 0; i < my_vec->first_free_space_idx; i++)
        printf("%c", my_vec->buff[i]);
    printf("\n");
}

// This function reads stdin to read_buffer of constant size, and then it copies
// data to reasizeable vec. Since it uses read() to read from STDIN, function 
// after successfully reading all data also adds \0 to the end of vec.
void my_vec_read_stdin(my_vec_t *my_vec)
{
    static char read_buff[BUFF_SIZE];
    size_t buff_size = BUFF_SIZE;
    ssize_t nbr_of_bytes_read = 0;

    memset(read_buff, 0, sizeof(read_buff));

    do
    {
        nbr_of_bytes_read = read(STDIN_FILENO, read_buff, buff_size);

        if (nbr_of_bytes_read == 0) 
        {
            break;
        }
        else if (nbr_of_bytes_read < 0)
            fatal("my_vec_read_stdin - read func returned < 0\n");
        
        push_back_str_to_vec(my_vec, read_buff, nbr_of_bytes_read);
        printf("%zu characters were read.\n",nbr_of_bytes_read);
        my_vec_print(my_vec);
        memset(read_buff, 0, sizeof(read_buff));

    } while(nbr_of_bytes_read != 0);

    printf("\n");
    my_vec_push_back(my_vec, '\0');
    // printf("my_vec:\n%s\n", my_vec->buff);
    // printf("nbr of bytes in vec: %" PRIu64 "\n", my_vec->occupied_size);
}