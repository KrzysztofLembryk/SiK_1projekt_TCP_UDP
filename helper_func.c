#include "helper_func.h"
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdbool.h>
#include <time.h>
#include "constants.h"
#include "err.h"

// - Function given char input checks whether it is equal to 'tcp', 'udp' or 
// 'udpr' if yes it return communication_type value: TCP, UDP or UDPR
// - Otherwise function invokes fatal()
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

// - Function depending on type value creates either TCP socket or UDP socket
// - Function sets *socket_fd to returned by socket() value
// - If socket() returned < 0 function invokes syserr()
void init_socket_fd(int *socket_fd, communication_type type)
{
    if (type == TCP)
        *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    else
    {
        *socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    }

    if (*socket_fd < 0)
    {
        syserr("cannot create a socket");
    }
}

// - Function sets timeout equl to max_wait for TCP client socket
void set_timeout_for_client_socket(int client_fd, int max_wait)
{
    // Set timeouts for the client socket so that we could prevent one 
    // client connecting and no sending anything thus blocking our server
    struct timeval time_o = {.tv_sec = max_wait, .tv_usec = 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &time_o, sizeof time_o);
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &time_o, sizeof time_o);
}

// - Function handles errors connected with usage of readn function and makes
// error msg accordingly to given error.
// - Function returns ERROR if error was encountered, SUCCESS otherwise
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

// - Function is wrapper for error() function that takes func_name and msg to
// be printed in stderr
void make_error_msg(const char *func_name, const char *msg)
{
    static char text[200];

    memset(text, 0, sizeof(text));
    strcpy(text, func_name);
    strcat(text, msg);
    error(text);
}

// - Function prints buff_len bytes from buff to stdout, function flushes stdout
// to make sure printf() prints wanted bytes
void print_data_to_stdout(char *buff, uint64_t package_id, uint32_t buff_len)
{
    printf("%.*s", (int)buff_len, buff);
    fflush(stdout);
}

// - Function sends data using sendto() and handles errors returned by it
// - If no errors functions returns SUCCESS, otherwise ERROR
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

// - Function uses recvfrom() to wait for data
// - Function handles errors returned by recvfrom
// - Function also handles case when data is being sent from someone with whom
// connection was not established and prints according error and ignores this 
// data (real_server_s_addr and server_port are needed for checking this)
// - If no errors occured returns SUCCESS, otherwise ERROR
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
            first_wait = false;
            *real_server_s_addr = receive_address.sin_addr.s_addr;
        }
        if (receive_address.sin_addr.s_addr != *real_server_s_addr ||
        receive_address.sin_port != server_port)
        {
            // If we got packet not from our server we ignore it
            make_error_msg(__FUNCTION__, " - got msg not from my server, ignoring it");
            continue;
        }

        return SUCCESS;
    }

    return SUCCESS;
}

void save_to_file(const char *f_name, clock_t start, clock_t end, 
    uint64_t nbr_of_bytes)
{
    FILE *file;
    file = fopen(f_name, "a");
    float seconds = (float)(end - start) / CLOCKS_PER_SEC;

    if (nbr_of_bytes / 1000 > 100)
    {
        fprintf(file, "%" PRIu64 "MB,", nbr_of_bytes / 1000000);
    }
    else
    {
        fprintf(file, "%" PRIu64 "KB,", nbr_of_bytes / 1000);
    }
    fprintf(file, "%f,%u\n", seconds, SEND_BUFF_SIZE);
    fflush(file);
    fclose(file);
}