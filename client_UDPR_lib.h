#ifndef CLIENT_UDPR_LIB_H
#define CLIENT_UDPR_LIB_H

#include <inttypes.h>
#include <sys/socket.h>
#include "my_vec.h"

void UDPR_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, unsigned long real_server_s_addr);

#endif