#include <sys/socket.h>
#include "client_UDP_lib.h"
#include "packet_structures.h"
#include "common.h"
#include "err.h"
#include "helper_func.h"

#define SEND_FLAGS 0
#define RECEIVE_FLAGS 0
#define SUCCESS 0
#define ERROR -1
#define WRONG_SESSION_ID -2
#define TIMEOUT_ERROR -3


int sendto_wrapper(int socket_fd, struct sockaddr_in *server_address,
                socklen_t *server_address_len, uint64_t session_id, 
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
    else if (sent_length != data_size)
    {
        make_error_msg(function_name, " - sent_len not equal to size of data we wanted to send");
        return ERROR;
    }
    return SUCCESS;
}

int wait_for_server_response(int socket_fd, char *response_buffer)
{
    memset(response_buffer, 0, sizeof(response_buffer)); 

    struct sockaddr_in receive_address;
    ssocklen_t server_address_len = (socklen_t)sizeof(receive_address);

    ssize_t received_length = recvfrom(socket_fd, response_buffer, sizeof(response_buffer), RECEIVE_FLAGS, (struct sockaddr *)&receive_address, 
    &server_address_len);

    if (received_length < 0)
    {
       make_error_msg(__FUNCTION__, " - recvfrom < 0"); 
       return ERROR;
    }
    return received_length;
}

int UDP_client_send_DATA(int socket_fd, struct sockaddr_in *server_address,
                socklen_t *server_address_len, my_vec_t *vec, uint64_t session_id)
{
    uint32_t bytes_left = vec->occupied_size;
    uint32_t bytes_sent = 0;
    uint64_t start_cpy_pos = 0;
    uint64_t curr_package_id = 0;
    char buff[SEND_BUFF_SIZE + 1];
    DATA data;

    while (bytes_sent != vec->occupied_size)
    {
        memset(buff, 0, sizeof(buff));
        
        if (bytes_left < SEND_BUFF_SIZE)
        {
            strncpy(buff, vec->buff + start_cpy_pos, bytes_left);

            if (init_DATA(&data, session_id, curr_package_id, 
                                                bytes_left, buff) != SUCCESS)
            {
                return ERROR;
            }

            bytes_sent += bytes_left;
            bytes_left -= bytes_left;
        }
        else
        {
            strncpy(buff, vec->buff + start_cpy_pos, SEND_BUFF_SIZE);
            if (init_DATA(&data, session_id, curr_package_id, 
                                            SEND_BUFF_SIZE, buff) != SUCCESS)
            {
                return ERROR;
            }

            bytes_sent += SEND_BUFF_SIZE;
            bytes_left -= SEND_BUFF_SIZE;
            start_cpy_pos += SEND_BUFF_SIZE;
        }

        curr_package_id++;

        if (sendto_wrapper(socket_fd, server_address, server_address_len, session_id, &data, sizeof(data), __FUNCTION__)  != SUCCESS)
        {
            return ERROR;
        }
    }
    return SUCCESS;
}

void UDP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id)
{
    CONN conn;

    init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
    // init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);

    socklen_t server_address_len = (socklen_t)sizeof(*server_address);

    sendto_wrapper(socket_fd, server_address, &server_address_len, session_id, 
    &conn, sizeof(conn), __FUNCTION__);

    // Now we wait for server response - whether conacc or conrjt
    static char response_buffer[200];

    ssize_t received_length = wait_for_server_response(socket_fd, response_buffer);

    if (received_length != SUCCESS)
    {
        syserr("recvfrom");
    }

    CONACC conacc;
    cast_buff_to(&conacc, sizeof(conacc), response_buffer, (size_t)received_length);

    if (conacc.package_type_id != CONACC_ID)
    {
        make_error_msg(__FUNCTION__, " - rcvd package type id is not CONACC");
        return;
    }

    if (UDP_client_send_DATA(socket_fd, vec, session_id) != SUCCES)
        return;
    
    // Now we wait for rcvd
    received_length = wait_for_server_response(socket_fd, response_buffer);

    if (received_length != SUCCESS)
    {
        syserr("recvfrom");
    }

    RCVD rcvd;
    cast_buff_to(&rcvd, sizeof(rcvd), response_buffer, (size_t)received_length);

    if (rcvd.package_type_id != RCVD_ID)
    {
        make_error_msg(__FUNCTION__, " - rcvd package type id is not RCVD");
        return;
    }
}
