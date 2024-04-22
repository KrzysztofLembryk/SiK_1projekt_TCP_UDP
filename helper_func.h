#ifndef HELPER_FUNC_H
#define HELPER_FUNC_H

#include <stddef.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>

typedef enum communication_type {TCP, UDP, UDPR} communication_type;

communication_type check_communication_type(const char* input);

void init_socket_fd(int *socket_fd, communication_type type);

void set_timeout_for_client_socket(int client_fd, int max_wait);

int readn_error_handler(ssize_t read_length, size_t data_size);

void make_error_msg(const char *func_name, const char *msg);

void print_data_to_stdout(char *buff, uint64_t package_id, uint32_t buff_len);

int sendto_wrapper(int socket_fd, struct sockaddr_in *server_address,
                   socklen_t server_address_len,
                   void *data, size_t data_size, const char *function_name);

int wait_for_server_response(int socket_fd, char *response_buffer, size_t buff_size, ssize_t *received_length, unsigned long *real_server_s_addr, unsigned short server_port);

#endif