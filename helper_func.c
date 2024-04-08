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

void set_timeout_for_client_socket(int client_fd)
{
    // Set timeouts for the client socket so that we could prevent one 
    // client connecting and no sending anything thus blocking our server
    struct timeval time_o = {.tv_sec = MAX_WAIT, .tv_usec = 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &time_o, sizeof time_o);
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &time_o, sizeof time_o);
}

int readn_error_handler(ssize_t read_length, size_t data_size)
{
    if (read_length < 0)
    {
        if (errno == EAGAIN) 
        {
            error("readn - timeout\n"); 
            return -1;
        } 
        else 
        {
            error("readn");
            return -1;
        }
    }
    else if (read_length == 0) 
    {
        error("readn - connection closed read_len == 0\n");
        return -1;
    }
    else if ((size_t) read_length < data_size) 
    {
        error("readn - connection closed without providing full data structure\n");
        return -1;
    }

    return 0;
}