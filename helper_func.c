#include "helper_func.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include "constants.h"
#include "err.h"

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

int sendto_wrapper(int socket_fd, struct sockaddr_in *server_address,
                   socklen_t server_address_len,
                   void *data, size_t data_size, const char *function_name)
{
    ssize_t sent_length = sendto(socket_fd, data, data_size,
                                 SEND_FLAGS,
                                 (struct sockaddr *)server_address,
                                 server_address_len);
    if (sent_length < 0)
    {
        make_error_msg(function_name, " - sent len < 0");
        return ERROR;
    }
    else if ((size_t)sent_length != data_size)
    {
        make_error_msg(function_name, " - sent_len not equal to size of data we wanted to send");
        return ERROR;
    }
    return SUCCESS;
}

int wait_for_server_response(int socket_fd, char *response_buffer, size_t buff_size, ssize_t *received_length, unsigned long *real_server_s_addr, unsigned short server_port)
{
    static bool first_wait = true;

    while (true)
    {
        memset(response_buffer, 0, buff_size);

        struct sockaddr_in receive_address;
        socklen_t server_address_len = (socklen_t)sizeof(receive_address);

        *received_length = recvfrom(socket_fd, response_buffer, buff_size, RECEIVE_FLAGS, (struct sockaddr *)&receive_address,
                                        (socklen_t *)&server_address_len);

        if (*received_length <= 0)
        {
            if (errno == EAGAIN)
            {
                make_error_msg(__FUNCTION__, " - timeout");
                return TIMEOUT_ERROR;
            }
            else
            {
                make_error_msg(__FUNCTION__, " - recvfrom <= 0");
                return ERROR;
            }
        }
        if (first_wait)
        {
            printf("FIRST WAIT, server addres: %u\n", receive_address.sin_addr.s_addr);
            first_wait = false;
            *real_server_s_addr = receive_address.sin_addr.s_addr;
        }
        if (receive_address.sin_addr.s_addr != *real_server_s_addr ||
        receive_address.sin_port != server_port)
        {
            printf("---------------------------------------------------\n");
            printf("received addr s_addr: %u, correct addr s_addr: %lu\n", receive_address.sin_addr.s_addr, *real_server_s_addr);
            printf("received addre port: %hu, correct addr port: %hu\n", receive_address.sin_port, server_port);
            printf("---------------------------------------------------\n");

            // If we got packet not from our server we ignore it
            make_error_msg(__FUNCTION__, " - got msg not from my server, ignoring it");
            continue;
        }

        return SUCCESS;
    }

    return SUCCESS;
}