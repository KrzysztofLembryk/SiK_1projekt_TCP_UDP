#include <sys/socket.h>
#include <endian.h>
#include <errno.h>
#include <stdbool.h>
#include <unistd.h>

#include "client_UDP_lib.h"
#include "packet_structures.h"
#include "common.h"
#include "err.h"
#include "helper_func.h"
#include "constants.h"
#include "protconst.h"

int UDP_client_CONN_handler(int socket_fd,
                            struct sockaddr_in *server_address,
                            socklen_t server_address_len,
                            uint64_t session_id,
                            uint64_t occupied_size)
{
    CONN conn;

    init_CONN(&conn, session_id, UDP_PROTOCOL, occupied_size);

    if (sendto_wrapper(socket_fd, server_address, server_address_len,
                       &conn, sizeof(conn), __FUNCTION__) != SUCCESS)
    {
        return ERROR;
    }
    return SUCCESS;
}

int UDP_client_CONACC_handler(int socket_fd,
                              char *response_buffer,
                              uint64_t session_id,
                              unsigned long *real_server_s_addr,
                              unsigned short server_port)
{
    memset(response_buffer, 0, RESPONSE_BUFF_SIZE);

    ssize_t received_length;

    if (wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, &received_length, real_server_s_addr, server_port) != SUCCESS)
    {
        return ERROR;
    }

    CONACC conacc;

    printf("sizeof conacc: %zu, received bytes: %zu\n", sizeof(conacc), (size_t)received_length);
    cast_buff_to(&conacc, sizeof(conacc), response_buffer, (size_t)received_length);
    ntoh_CONACC(&conacc);

    // we should also check if session id is correct, to find out whether
    // correct server sent us conacc
    if (conacc.package_type_id != CONACC_ID)
    {
        make_error_msg(__FUNCTION__, " - rcvd package type id is not CONACC");
        return ERROR;
    }
    if (conacc.session_id != session_id)
    {
        make_error_msg(__FUNCTION__, " - received CONACC has wrong session id");
        return ERROR;
    }
    if (received_length != sizeof(conacc))
    {
        make_error_msg(__FUNCTION__, " - first two values of conacc were correct, but size of received message is not equal to size of CONACC packet");
        return ERROR;
    }
    return SUCCESS;
}

int UDP_client_send_DATA(int socket_fd,
                         struct sockaddr_in *server_address,
                         socklen_t server_address_len,
                         my_vec_t *vec,
                         uint64_t session_id)
{
    uint32_t bytes_left = vec->occupied_size;
    uint32_t bytes_sent = 0;
    uint64_t start_cpy_pos = 0;
    uint64_t curr_package_id = 0;
    static char buff[SEND_BUFF_SIZE + 1];
    DATA data;

    while (bytes_sent != vec->occupied_size)
    {
        memset(buff, 0, SEND_BUFF_SIZE + 1);

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

        if (sendto_wrapper(socket_fd, server_address, server_address_len,
                           &data, sizeof(DATA_INFO_t) + be32toh(data.nbr_of_bytes_in_packet), __FUNCTION__) != SUCCESS)
        {
            return ERROR;
        }
    }
    return SUCCESS;
}

int UDP_client_RCVD_handler(int socket_fd,
                            char *response_buffer,
                            uint64_t session_id,
                            unsigned long *real_server_s_addr,
                            unsigned short server_port)
{
    memset(response_buffer, 0, RESPONSE_BUFF_SIZE);

    ssize_t received_length;

    if (wait_for_server_response(socket_fd, response_buffer,
                                 RESPONSE_BUFF_SIZE, &received_length,
                                 real_server_s_addr, server_port) != SUCCESS)
    {
        return ERROR;
    }

    RCVD rcvd;
    cast_buff_to(&rcvd, sizeof(rcvd), response_buffer, (size_t)received_length);
    ntoh_RCVD(&rcvd);

    if (rcvd.package_type_id != RCVD_ID)
    {
        make_error_msg(__FUNCTION__, " - received package type id is not RCVD");
        return ERROR;
    }
    if (rcvd.session_id != session_id)
    {
        make_error_msg(__FUNCTION__, " - received package type is RCVD but with wrong session id");
        return ERROR;
    }
    if (received_length != sizeof(rcvd))
    {

        make_error_msg(__FUNCTION__, " - first two values of RCVD were correct, but size of received message is not equal to size of RCVD packet");
        return ERROR;
    }
    return SUCCESS;
}

void UDP_client_handler(int socket_fd,
                        struct sockaddr_in *server_address,
                        my_vec_t *vec,
                        uint64_t session_id)
{
    unsigned long real_server_s_addr = 0;
    static char response_buffer[RESPONSE_BUFF_SIZE];
    socklen_t server_address_len = (socklen_t)sizeof(*server_address);

    if (UDP_client_CONN_handler(socket_fd, server_address, server_address_len, session_id, vec->occupied_size) != SUCCESS)
    {
        return;
    }

    // Now we wait for server response - whether conacc or conrjt, there might be a possibility that different server will send us message, we need to ignore it thus loop will be needed

    if (UDP_client_CONACC_handler(socket_fd, response_buffer, session_id,
                                  &real_server_s_addr, server_address->sin_port) != SUCCESS)
    {
        return;
    }

    if (UDP_client_send_DATA(socket_fd, server_address, server_address_len, vec, session_id) != SUCCESS)
    {
        return;
    }

    // Now we wait for rcvd
    if (UDP_client_RCVD_handler(socket_fd, response_buffer, session_id, &real_server_s_addr, server_address->sin_port) != SUCCESS)
        return;
}
