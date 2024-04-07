#include "helper_func.h"
#include "err.h"
#include <sys/socket.h>
#include <string.h>

server_type check_type_of_server(const char* input)
{
    if (strcmp(input, "tcp") == 0)
    {
        return TCP;
    }
    else if (strcmp(input, "udp") == 0)
    {
        return UDP;
    }
    else
        fatal("given protocol type is not tcp nor udp\n");
}

void init_socket_fd(int *socket_fd, server_type type)
{
    if (type == TCP)
        *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    else
        *socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (*socket_fd < 0)
    {
        syserr("cannot create a socket");
    }
}