#include "data_handler_lib.h"

my_vec_t *read_stdin()
{
    my_vec_t* my_vec = my_vec_init();
    my_vec_read_stdin(my_vec);
    return my_vec;
}