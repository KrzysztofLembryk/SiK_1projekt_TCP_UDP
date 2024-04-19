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

    if (*received_length < 0)
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

int UDP_client_CONN_handler(int socket_fd, struct sockaddr_in *server_address, socklen_t server_address_len, uint64_t session_id, uint64_t occupied_size)
{
    CONN conn;

    init_CONN(&conn, session_id, UDP_PROTOCOL, occupied_size);

    printf("Sending conn package \n");
    if (sendto_wrapper(socket_fd, server_address, server_address_len,
                   &conn, sizeof(conn), __FUNCTION__) != SUCCESS)
    {
        return ERROR;
    }
    return SUCCESS;
}


int UDP_client_CONACC_handler(int socket_fd, char *response_buffer, 
    uint64_t session_id)
{
    memset(response_buffer, 0, RESPONSE_BUFF_SIZE);

    ssize_t received_length;
    int wait_ret_val = wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, &received_length);

    if (wait_ret_val != SUCCESS)
    {
        return ERROR;
    }

    printf("Got response, now casting it\n");

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


int UDP_client_send_DATA(int socket_fd, struct sockaddr_in *server_address,
                         socklen_t server_address_len, my_vec_t *vec, uint64_t session_id)
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
            printf("- udp client send data\n");
            return ERROR;
        }
    }
    return SUCCESS;
}

int UDP_client_RCVD_handler(int socket_fd, char *response_buffer, 
    uint64_t session_id)
{
    memset(response_buffer, 0, RESPONSE_BUFF_SIZE);

    printf("Waiting for server rcvd\n");
    ssize_t received_length;
    int wait_ret_val = wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, &received_length);

    if (wait_ret_val != SUCCESS)
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


void UDP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id)
{
    static char response_buffer[RESPONSE_BUFF_SIZE];
    socklen_t server_address_len = (socklen_t)sizeof(*server_address);

    if (UDP_client_CONN_handler(socket_fd, server_address, server_address_len, session_id, vec->occupied_size) != SUCCESS)
    {
        return;
    }

    // Now we wait for server response - whether conacc or conrjt, there might be a possibility that different server will send us message, we need to ignore it thus loop will be needed
    int conacc_ret_val = UDP_client_CONACC_handler(socket_fd, response_buffer, session_id);

    if (conacc_ret_val != SUCCESS)
        return;

    printf("Sending data\n");
    if (UDP_client_send_DATA(socket_fd, server_address, server_address_len, vec, session_id) != SUCCESS)
    {
        return;
    }

    // Now we wait for rcvd
    if (UDP_client_RCVD_handler(socket_fd, response_buffer, session_id) != SUCCESS)
        return;
}


// ----------UDPR CLIENT FUNCTIONS----------

// - Function tries to establish connection with server by sending CONN and 
// waiting to receive CONACC.
// - If any error occured function returns ERROR, otherwise if connection was
// established returns SUCCESS
int UDPR_client_init_connection(int socket_fd, char *response_buffer,
                                struct sockaddr_in *server_address, 
                                socklen_t server_address_len,
                                uint64_t session_id,
                                uint64_t occupied_size)
{
    ssize_t received_length;
    CONN conn;
    init_CONN(&conn, session_id, UDPR_PROTOCOL, occupied_size);
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

    if (wait_for_server_response(socket_fd, response_buffer, RESPONSE_BUFF_SIZE, &received_length) != SUCCESS)
    {
        return ERROR;
    }

    CONACC conacc;

    printf("sizeof conacc: %zu, received bytes: %zu\n", sizeof(conacc), 
        (size_t)received_length);

    cast_buff_to(&conacc, sizeof(conacc), response_buffer, 
                                                (size_t)received_length);
    ntoh_CONACC(&conacc);

    if (conacc->package_type_id != CONACC_ID || conacc->session_id != session_id)
    {
        return ERROR;
    }
    printf("UDPR client success in connecting to server\n");
    return SUCCESS;
}

int UDPR_client_handle_RCVD(int socket_fd, char *response_buff, 
    ssize_t *received_length, uint64_t curr_package_id, uint64_t session_id)
{
    RCVD rcvd;
    // We wait for server response
    do
    {
        memset(response_buff, 0, RESPONSE_BUFF_SIZE);
        int wait_ret_val = wait_for_server_response(socket_fd, response_buff, RESPONSE_BUFF_SIZE, received_length);

        if (wait_ret_val == TIMEOUT_ERROR)
        {
            // We wait for rcvd, if we didnt get it in MAX_WAIT we get error
            // since we cannot do the retransmission anymore since we already
            // sent all data
            make_error_msg(__FUNCTION__, " - timeout");
            return ERROR;
        }
        else if (wait_ret_val == ERROR)
        {
            // Recvfrom was < 0 
            return ERROR;
        }
        // there will be another case: (but not impl yet)
        // else if (wait_ret_val == WRONG_SERVER_ADDRESS)
        // continue;

        cast_buff_to(&rcvd, sizeof(rcvd), response_buff, *received_length);
        ntoh_RCVD(&rcvd);

        // We could get ACC package instead of RCVD, if so we ignore it if its
        // correct ACC package, if not, we get error and end connection since 
        // such behaviour is against protocol
        if (rcvd.package_type_id == ACC_ID)
        {
            if (rcvd.session_id == session_id)
            {
                ACC acc;

                cast_buff_to(&acc, sizeof(acc), response_buff, *received_length); 
                ntoh_ACC(&acc);

                if (acc.package_id > curr_package_id)
                {
                    make_error_msg(__FUNCTION__, " - received ACC packet but with package_id greater than curr_package_id");
                    return ERROR;
                }
                continue;
            }
            else
            {
                make_error_msg(__FUNCTION__, " - received ACC packet with wrong session id");
                return ERROR;
            }
        }
        if (rcvd.package_type_id != RCVD_ID)
        {
            make_error_msg(__FUNCTION__, " - wrong packet type");
            return ERROR;
        }
        if (rcvd.session_id != session_id)
        {
            make_error_msg(__FUNCTION__, " - wrong session id in received RCVD packet");
            return ERROR;
        }
        if (*received_length != sizeof(rcvd))
        {
            make_error_msg(__FUNCTION__, " - first two values of rcvd were correct, but size of received message is not equal to size of RCVD packet");
            return ERROR;
        }

        return SUCCESS;
    } while (true);

    return SUCCESS;
}

int UDPR_client_handle_ACC(int socket_fd, char *response_buff, ACC *acc, ssize_t *received_length, int *nbr_of_retransmits, uint64_t curr_package_id, uint64_t session_id)
{
    // We wait for server response
    do
    {
        memset(response_buff, 0, RESPONSE_BUFF_SIZE);
        int wait_ret_val = wait_for_server_response(socket_fd, response_buff, RESPONSE_BUFF_SIZE, received_length);

        if (wait_ret_val == TIMEOUT_ERROR)
        {
            (*nbr_of_retransmits)++;
            if ((*nbr_of_retransmits) > MAX_RETRANSMITS)
            {
                make_error_msg(__FUNCTION__, " - nbr of retransmits greater than MAX nbr of retransmits");
                return ERROR;
            }
            return RETRANSMISSION;
        }
        else if (wait_ret_val == ERROR)
        {
            // Recvfrom was < 0 
            return ERROR;
        }

        cast_buff_to(acc, sizeof(*acc), response_buff, *received_length);
        if (acc->package_type_id == CONACC_ID)
        {
            // No matter how many conacc we get, we ignore all of them since we
            // established connection with server, thus we wait for ACC
            // (even though some conacc might have wrong session id, we ignore 
            // them nonetheless)
            continue;
        }

        ntoh_ACC(acc);

        if (acc->package_type_id != ACC_ID)
            return ERROR;
        if (acc->session_id != session_id)
            return ERROR;
        if (acc->package_id < curr_package_id)
        {
            // We could get good ACC but with old package_id thus we ignore it
            // and wait for next ACC with good package_id 
            continue;
        }
        if (acc->package_id > curr_package_id)
            return ERROR;

        return SUCCESS;
    } while (true);

    return SUCCESS;
}

int UDPR_client_send_DATA(int socket_fd, struct sockaddr_in *server_address,
                          socklen_t server_address_len, my_vec_t *vec, uint64_t session_id, int *nbr_of_retransmits)
{
    uint32_t bytes_left = vec->occupied_size;
    uint32_t bytes_sent = 0;
    uint64_t start_cpy_pos = 0;
    uint64_t shift = 0;
    uint64_t curr_package_id = 0;
    ssize_t received_length = 0;
    static char buff[SEND_BUFF_SIZE + 1];
    static char response_buff[RESPONSE_BUFF_SIZE];
    DATA data;

    while (bytes_sent != vec->occupied_size)
    {
        memset(buff, 0, SEND_BUFF_SIZE + 1);
        memset(response_buff, 0, RESPONSE_BUFF_SIZE);
        // sleep(11);
        if (bytes_left < SEND_BUFF_SIZE)
        {
            strncpy(buff, vec->buff + start_cpy_pos, bytes_left);

            if (init_DATA(&data, session_id, curr_package_id,
                          bytes_left, buff) != SUCCESS)
            {
                return ERROR;
            }

            // bytes_sent += bytes_left;
            // bytes_left -= bytes_left;
            shift = bytes_left;
        }
        else
        {
            strncpy(buff, vec->buff + start_cpy_pos, SEND_BUFF_SIZE);
            if (init_DATA(&data, session_id, curr_package_id,
                          SEND_BUFF_SIZE, buff) != SUCCESS)
            {
                return ERROR;
            }

            // bytes_sent += SEND_BUFF_SIZE;
            // bytes_left -= SEND_BUFF_SIZE;
            shift = SEND_BUFF_SIZE;
        }

        // Sending Data
        if (sendto_wrapper(socket_fd, server_address, server_address_len,
                           &data, sizeof(DATA_INFO_t) + be32toh(data.nbr_of_bytes_in_packet), __FUNCTION__) != SUCCESS)
        {
            make_error_msg(__FUNCTION__, " - sendto_wrapper error");
            return ERROR;
        }

        ACC acc;
        int acc_ret_val = UDPR_client_handle_ACC(socket_fd, response_buff, 
        &acc, &received_length, nbr_of_retransmits, curr_package_id, session_id);

        if (acc_ret_val == ERROR)
            return ERROR;
        else if (acc_ret_val == RETRANSMISSION)
            continue;

        // Otherwise we got correct ACC thus we can increase curr_package_id
        // and start_cpt_pos to be able to send next data package
        curr_package_id++;
        bytes_sent += shift;
        bytes_left -= shift;
        start_cpy_pos += shift;
    }
    return SUCCESS;
}

void UDPR_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id)
{
    socklen_t server_address_len = (socklen_t)sizeof(*server_address);
    int nbr_of_retransmits = 0;
    static char response_buffer[RESPONSE_BUFF_SIZE];
    ssize_t received_length;
    int init_ret_val;
    uint64_t last_package_idx = 0;

    // INITIALIZATION OF CONNECTION
    do
    {
        memset(response_buffer, 0, RESPONSE_BUFF_SIZE);

        init_ret_val = UDPR_client_init_connection(socket_fd, response_buffer, server_address, server_address_len, session_id, vec->occupied_size);

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
    
    // Connection with server was established succesfully, now we will be 
    // sending our data
    printf("Sending data\n"); 
    sleep(11);
    printf("Po sleep\n");
    if (UDPR_client_send_DATA(socket_fd, server_address, server_address_len, vec, session_id, &nbr_of_retransmits) != SUCCESS)
    {
        return;
    }

    // Now we wait for rcvd, we should ignore any acc we got
    if (UDPR_client_handle_RCVD(socket_fd, response_buffer, received_length, last_package_idx, session_id) != SUCCESS)
    {
        make_error_msg(__FUNCTION__, " - client did not receive RCVD msg from server");
        return;
    }
}
