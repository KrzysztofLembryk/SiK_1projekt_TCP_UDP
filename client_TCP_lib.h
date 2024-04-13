#ifndef CLIENT_TCP_LIb_H
#define CLIENT_TCP_LIb_H

#include "packet_structures.h"
#include "my_vec.h"
#include <inttypes.h>

int TCP_client_send_CONN(int socket_fd, CONN *conn);


int TCP_client_send_DATA(int socket_fd, my_vec_t *vec, uint64_t session_id);


void TCP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, unsigned int session_id);




#endif