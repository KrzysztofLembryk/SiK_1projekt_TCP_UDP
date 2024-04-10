#ifndef HELPER_FUNC_H
#define HELPER_FUNC_H

#include <stddef.h>
#include <sys/types.h>

typedef enum communication_type {TCP, UDP, UDPR} communication_type;

communication_type check_communication_type(const char* input);

void init_socket_fd(int *socket_fd, communication_type type);

void set_timeout_for_client_socket(int client_fd, int max_wait);

int readn_error_handler(ssize_t read_length, size_t data_size);

#endif