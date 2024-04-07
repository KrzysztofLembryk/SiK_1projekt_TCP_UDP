#ifndef HELPER_FUNC_H
#define HELPER_FUNC_H

typedef enum server_type {TCP, UDP} server_type;

server_type check_type_of_server(const char* input);

void init_socket_fd(int *socket_fd, server_type type);

#endif