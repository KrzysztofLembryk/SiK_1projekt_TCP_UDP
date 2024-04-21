#ifndef CLIENT_TESTS_LIB_H
#define CLIENT_TESTS_LIB_H

#include <stdbool.h>
#include <netinet/in.h>
#include "my_vec.h"

void TCP_UDP_client_tests(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, bool is_TCP);


#endif