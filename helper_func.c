#include "helper_func.h"
#include "err.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include "constants.h"

communication_type check_communication_type(const char* input)
{
    if (strcmp(input, "tcp") == 0)
        return TCP;
    else if (strcmp(input, "udp") == 0)
        return UDP;
    else if (strcmp(input, "udpr") == 0)
        return UDPR;
    else
        fatal("given protocol type is not tcp nor udp nor udpr\n");
}

void init_socket_fd(int *socket_fd, communication_type type)
{
    if (type == TCP)
        *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    else
    {
        printf("Creating UDP socket\n");
        *socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (*socket_fd < 0)
    {
        syserr("cannot create a socket");
    }
}

void set_timeout_for_client_socket(int client_fd, int max_wait)
{
    // Set timeouts for the client socket so that we could prevent one 
    // client connecting and no sending anything thus blocking our server
    struct timeval time_o = {.tv_sec = max_wait, .tv_usec = 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &time_o, sizeof time_o);
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &time_o, sizeof time_o);
}

int readn_error_handler(ssize_t read_length, size_t data_size)
{
    if (read_length < 0)
    {
        if (errno == EAGAIN) 
        {
            make_error_msg(__FUNCTION__, " - readn timeout");
            return ERROR;
        } 
        else 
        {
            make_error_msg(__FUNCTION__, " - readn < 0");
            return ERROR;
        }
    }
    else if (read_length == 0) 
    {
        make_error_msg(__FUNCTION__, " - connection closed read_len == 0");
        return ERROR;
    }
    else if ((size_t) read_length < data_size) 
    {
        make_error_msg(__FUNCTION__, " - read nbr of bytes less than provided data size");
        return ERROR;
    }

    return SUCCESS;
}

void make_error_msg(const char *func_name, const char *msg)
{
    static char text[200];

    memset(text, 0, sizeof(text));
    strcpy(text, func_name);
    strcat(text, msg);
    error(text);
}

void print_data_to_stdout(char *buff, uint64_t package_id, uint32_t buff_len)
{
    printf("[packet: %" PRIu64 "]:\n%.*s\n", package_id, (int)buff_len, buff);
    // printf("[packet: %" PRIu64 "]:\n", package_id);
}