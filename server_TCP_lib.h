#ifndef SERVER_TCP_LIB_H
#define SERVER_TCP_LIB_H


#include <inttypes.h>
#include <netinet/in.h>
#include "packet_structures.h"
#include "stdbool.h"


int TCP_wait_for_client(int socket_fd, int *c_fd, struct sockaddr_in *client_address);


int TCP_conn_init_helper(CONN *conn, int client_fd);


int TCP_handle_conn_init(CONN *conn, int client_fd);



int TCP_get_DATA_metainfo(int client_fd, DATA_INFO_t *data_metainfo, 
                            uint64_t session_id, uint64_t curr_packet_id);


int TCP_send_packet(void *packet, size_t packet_size, int client_fd);



int TCP_read_data_to_buf(int client_fd, char *buf, 
                                        uint32_t nbr_of_bytes_in_packet);


void TCP_print_data_to_stdout(char *buff, uint64_t package_id, uint32_t buff_len);


void TCP_server_handler(int socket_fd, struct sockaddr_in *server_address, int queue_len);

#endif