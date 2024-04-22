#ifndef CLIENT_UDP_LIB_H
#define CLIENT_UDP_LIB_H

#include <netinet/in.h>
#include "my_vec.h"
#include "helper_func.h"

void UDP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id);


#endif