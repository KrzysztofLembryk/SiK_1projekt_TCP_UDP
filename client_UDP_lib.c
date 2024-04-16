#include <sys/socket.h>
#include <endian.h>
#include <errno.h>
#include <unistd.h>
#include "client_UDP_lib.h"
#include "packet_structures.h"
#include "common.h"
#include "err.h"
#include "helper_func.h"
#include "constants.h"
#include "protconst.h"

#define RESPONSE_BUFF_SIZE 200

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

int wait_for_server_response(int socket_fd, char *response_buffer, size_t buff_size, ssize_t *received_length)
{
    printf("waiting for server repsonse\n");
    memset(response_buffer, 0, buff_size);

    struct sockaddr_in receive_address;
    socklen_t server_address_len = (socklen_t)sizeof(receive_address);

    *received_length = recvfrom(socket_fd, response_buffer, buff_size, RECEIVE_FLAGS, (struct sockaddr *)&receive_address,
                                       (socklen_t *)&server_address_len);

    if (received_length < 0)
    {
        if (errno == EAGAIN)
        {
            make_error_msg(__FUNCTION__, " - timeout");
            return TIMEOUT_ERROR;
        }
        else
        {
            make_error_msg(__FUNCTION__, " - recvfrom < 0");
            return ERROR;
        }
    }
    return SUCCESS;
}

int UDP_client_send_DATA(int socket_fd, struct sockaddr_in *server_address,
                         socklen_t server_address_len, my_vec_t *vec, uint64_t session_id)
{
    uint32_t bytes_left = vec->occupied_size;
    uint32_t bytes_sent = 0;
    uint64_t start_cpy_pos = 0;
    uint64_t curr_package_id = 0;
    char buff[SEND_BUFF_SIZE + 1];
    DATA data;

    while (bytes_sent != vec->occupied_size)
    {
        memset(buff, 0, SEND_BUFF_SIZE + 1);
        // sleep(11);
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
            printf("WTF - udp client send data\n");
            return ERROR;
        }
    }
    return SUCCESS;
}

void UDP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, communication_type comm_type)
{
    CONN conn;

    init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
    // init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);

    socklen_t server_address_len = (socklen_t)sizeof(*server_address);

    printf("Sending conn package \n");
    sendto_wrapper(socket_fd, server_address, server_address_len,
                   &conn, sizeof(conn), __FUNCTION__);

    // Now we wait for server response - whether conacc or conrjt
    static char response_buffer[RESPONSE_BUFF_SIZE];
    ssize_t received_length;
    int wait_ret_val = wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, &received_length);

    if (wait_ret_val == ERROR)
    {
        return;
    }
    else if (wait_ret_val == TIMEOUT_ERROR)
        return;

    printf("Got response, now casting it\n");

    CONACC conacc;

    printf("sizeof conacc: %zu, received bytes: %zu\n", sizeof(conacc), (size_t)received_length);
    cast_buff_to(&conacc, sizeof(conacc), response_buffer, (size_t)received_length);

    // we should also check if session id is correct, to find out whether
    // correct server sent us conacc
    if (conacc.package_type_id != CONACC_ID)
    {
        make_error_msg(__FUNCTION__, " - rcvd package type id is not CONACC");
        return;
    }

    printf("Sending data\n");
    if (UDP_client_send_DATA(socket_fd, server_address, server_address_len, vec, session_id) != SUCCESS)
    {
        return;
    }

    // Now we wait for rcvd
    printf("Waiting for verver rcvd\n");
    wait_ret_val = wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, &received_length);

    if (wait_ret_val != SUCCESS)
    {
        return;
    }

    RCVD rcvd;
    cast_buff_to(&rcvd, sizeof(rcvd), response_buffer, (size_t)received_length);

    if (rcvd.package_type_id != RCVD_ID)
    {
        make_error_msg(__FUNCTION__, " - rcvd package type id is not RCVD");
        return;
    }
}

// - Function tries to establish connection with server by sending CONN and 
// waiting to receive CONACC.
// - If any error occured function returns ERROR, otherwise if connection was
// established returns SUCCESS
int UDPR_client_init_connection(int socket_fd, char *response_buffer,
                                struct sockaddr_in *server_address, 
                                socklen_t server_address_len,
                                uint64_t session_id,
                                ssize_t *received_length,
                                CONACC *conacc)
{
    CONN conn;
    init_CONN(&conn, session_id, UDPR_PROTOCOL, vec->occupied_size);

    printf("Sending conn package \n");
    if (sendto_wrapper(socket_fd, server_address, server_address_len,
                   &conn, sizeof(conn), __FUNCTION__) != SUCCESS)
    {
        // char msg[100];
        // strcpy(msg, __FUNCTION__);
        // strcat(msg, " - sendto error  <= 0");
        // fatal(msg);
        make_error_msg(__FUNCTION__, " - sendto error <= 0");
        return ERROR;
    }

    if (wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, received_length) != SUCCESS)
    {
        return ERROR;
    }

    printf("sizeof conacc: %zu, received bytes: %zu\n", sizeof(*conacc), (size_t)received_length);
    cast_buff_to(&conacc, sizeof(*conacc), response_buffer, (size_t)received_length);
    ntoh_CONACC(conacc);

    if (conacc->package_type_id != CONACC_ID || conacc->session_id != session_id)
    {
        return ERROR;
    }

    return SUCCESS;
}

int UDPR_client_send_DATA(int socket_fd, struct sockaddr_in *server_address,
                          socklen_t server_address_len, my_vec_t *vec, uint64_t session_id)
{
    return SUCCESS;
}

void UDPR_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, communication_type comm_type)
{
    socklen_t server_address_len = (socklen_t)sizeof(*server_address);
    int nbr_of_retransmits = 0;
    static char response_buffer[RESPONSE_BUFF_SIZE];
    ssize_t received_length;
    int init_ret_val;

    printf("Got response, now casting it\n");
    CONACC conacc;
    // INITIALIZATION OF CONNECTION
    do
    {
        memset(response_buffer, 0, RESPONSE_BUFF_SIZE);

        init_ret_val = UDPR_client_init_connection(socket_fd, response_buffer, server_address, server_address_len, session_id, 
        &received_length, &conacc);

        if (init_ret_val != SUCCESS)
        {
            nbr_of_retransmits++;
            if (nbr_of_retransmits > MAX_RETRANSMITS)
            {
                make_error_msg(__FUNCTION__, " - nbr of retransmits greater than MAX nbr of retransmits");
                return;
            }
            continue;
        }
    } while (init_ret_val != SUCCESS);
    
    // Connection with server was established succesfully

    

    printf("Sending data\n");
    switch (comm_type)
    {
    case UDP:
        if (UDP_client_send_DATA(socket_fd, server_address, server_address_len, vec, session_id) != SUCCESS)
        {
            return;
        }
        break;
    case UDPR:
        break;
    default:
        break;
    }

    // Now we wait for rcvd
    printf("Waiting for verver rcvd\n");
    received_length = wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE);

    if (received_length == ERROR)
    {
        return;
    }
    else if (received_length == TIMEOUT_ERROR)
        return;

    RCVD rcvd;
    cast_buff_to(&rcvd, sizeof(rcvd), response_buffer, (size_t)received_length);

    if (rcvd.package_type_id != RCVD_ID)
    {
        make_error_msg(__FUNCTION__, " - rcvd package type id is not RCVD");
        return;
    }
}
