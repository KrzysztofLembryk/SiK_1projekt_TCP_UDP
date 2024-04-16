#include <unistd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "constants.h"
#include "common.h"
#include "packet_structures.h"
#include "helper_func.h"
#include "err.h"
#include "protconst.h"

// - Function reads maximally RECEIVE_BUFFOR_SIZE bytes to buff
// - Before reading function zeros buffer
// - If recvfrom read <= 0 bytes func returns -1, otherwise nbr of bytes read
// - Function sets variables *client_address and client_addr_len
int read_data_to_buffer(int socket_fd, char *buff, size_t buff_size,
                            struct sockaddr_in *client_address,
                            socklen_t *client_address_len,
                            ssize_t *read_bytes)
{
    memset(buff, 0, buff_size);

    // UDP gets data as datagrams that are stored in queue, so after we do
    // recvfrom, we read whole datagram from queue, so if we dont have
    // enough space in buffor part of data is lost. Thus first we will read
    // whole datagram into buffer, then cast it on our structures i.e. CONN.
    *read_bytes = recvfrom(socket_fd, buff, RECEIVE_BUFFOR_SIZE,
                                  DEFAULT_FLAG,
                                  (struct sockaddr *)client_address,
                                  (socklen_t *)client_address_len);

    if (*read_bytes < 0)
    {
        if (errno == EAGAIN) 
        {
            make_error_msg(__FUNCTION__, " - timeout\n"); 
            return TIMEOUT_ERROR;
        }
        else 
        {
            make_error_msg(__FUNCTION__, " - read_bytes < 0");
            return ERROR;
        }
    }

    printf("READ BYTES: %zu\n", (size_t)*read_bytes);

    if (*read_bytes == 0)
    {
        make_error_msg(__FUNCTION__, " - read_bytes == 0");
        return ERROR;
    }
    return SUCCESS;
}

int sendto_wrapper(int socket_fd, struct sockaddr_in *client_address,
                socklen_t client_address_len,  
                void *data, size_t data_size, const char *function_name)
{
    ssize_t sent_length = sendto(socket_fd, data, data_size,
                                 DEFAULT_FLAG,
                                 (struct sockaddr *)client_address,
                                 client_address_len);
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

int check_if_correct_CONN(char *buff, ssize_t read_bytes, CONN *conn)
{
    if (cast_buff_to(conn, sizeof(*conn), buff, (size_t)read_bytes) != SUCCESS)
        return ERROR;

    ntoh_CONN(conn);
    print_CONN(conn);

    if (conn->package_type_id != CONN_ID)
    {
        make_error_msg(__FUNCTION__, " - package type id is not CONN");
        return ERROR;
    }
    if (conn->protocol_id != UDP_PROTOCOL && conn->protocol_id != UDPR_PROTOCOL)
    {
        make_error_msg(__FUNCTION__, " - protocol is not udp nor udpr");
        return ERROR;
    }

    return SUCCESS;
}


int send_CONRJT(int socket_fd, struct sockaddr_in *client_address,
                socklen_t client_address_len, uint64_t session_id)
{
    printf("Sending CONRJT\n");
    CONRJT conrjt;

    init_CONRJT(&conrjt, session_id);

    int ret_val = sendto_wrapper(socket_fd, client_address, client_address_len, 
     &conrjt, sizeof(conrjt), __FUNCTION__);

    return ret_val;
}

int send_CONACC(int socket_fd, struct sockaddr_in *client_address,
                socklen_t client_address_len, uint64_t session_id)
{
    printf("Sending CONACC\n");
    CONACC conacc;

    init_CONACC(&conacc, session_id);

    int ret_val = sendto_wrapper(socket_fd, client_address, client_address_len, 
    &conacc, sizeof(conacc), __FUNCTION__);

    return ret_val;
}

int send_RJT(int socket_fd, struct sockaddr_in *client_address,
             socklen_t client_address_len, uint64_t session_id,
             uint64_t package_id)
{
    printf("Sending RJT\n");
    RJT rjt;

    init_RJT(&rjt, session_id, package_id);

    int ret_val =  sendto_wrapper(socket_fd, client_address, client_address_len, &rjt, sizeof(rjt), __FUNCTION__);

    return ret_val;
}

int send_RCVD(int socket_fd, struct sockaddr_in *client_address,
             socklen_t client_address_len, uint64_t session_id)
{
    printf("Sending RCVD\n");
    RCVD rcvd;

    init_RCVD(&rcvd, session_id);

    int ret_val =  sendto_wrapper(socket_fd, client_address, client_address_len, &rcvd, sizeof(rcvd), __FUNCTION__);

    return ret_val;
}

int send_ACC(int socket_fd, struct sockaddr_in *client_address,
             socklen_t client_address_len, uint64_t session_id, 
             uint64_t package_id)
{
    printf("Sending ACC\n");
    ACC acc;

    init_ACC(&acc, session_id, package_id);

    int ret_val =  sendto_wrapper(socket_fd, client_address, client_address_len, &acc, sizeof(acc), __FUNCTION__);

    return ret_val;
}

int check_if_correct_DATA_packet(char *buff,
                                 ssize_t read_bytes,
                                 CONN *conn,
                                 DATA_INFO_t *d_info,
                                 uint64_t curr_package_id)
{
    if (cast_buff_to(d_info, sizeof(*d_info), buff, (size_t)read_bytes) != SUCCESS)
        return ERROR;

    ntoh_DATA_INFO(d_info);
    printf("Printing info about received data:\n");
    print_DATA_INFO(d_info);

    if (d_info->session_id != conn->session_id)
    {
        make_error_msg(__FUNCTION__, " - received DATA package has wrong session id");
        return WRONG_SESSION_ID;
    }
    if (d_info->package_type_id != DATA_ID)
    {
        make_error_msg(__FUNCTION__, " - received pacakge_type is not DATA");
        return WRONG_PACKAGE_TYPE_ID;
    }
    if (d_info->package_id != curr_package_id)
    {
        make_error_msg(__FUNCTION__, " - received DATA package has wrong package id");
        return WRONG_PACKAGE_ID;
    }
    if (read_bytes - sizeof(*d_info) - d_info->nbr_of_bytes_in_packet != 0)
    {
        printf("Read bytes: %zu\n", (size_t)read_bytes);
        printf("sizeof dinfo: %zu\n", sizeof(*d_info));
        printf("nbr of bytes in data: %" PRIu32 "\n", d_info->nbr_of_bytes_in_packet);
        make_error_msg(__FUNCTION__, " - nbr of received bytes from client is not equal to declared nbr of bytes in DATA_INFO header");
        return WRONG_PACKAGE_SIZE;
    }

    return SUCCESS;
}

void UDP_data_receive(int socket_fd, char *buff, CONN *conn)
{
    const uint64_t bytes_to_receive = conn->nbr_of_bytes_to_be_sent;
    uint64_t bytes_recvd = 0;
    uint64_t curr_package_id = 0;
    static struct sockaddr_in client_address;
    static socklen_t client_address_len = (socklen_t)sizeof(client_address);

    while (bytes_recvd < bytes_to_receive)
    {
        printf("waiting for packet [%lu]\n", curr_package_id);
        ssize_t read_bytes; 

        int read_ret_val = read_data_to_buffer(socket_fd, buff, 
                                                 RECEIVE_BUFFOR_SIZE,
                                                 &client_address,
                                                 &client_address_len,
                                                 &read_bytes);

        if (read_ret_val == TIMEOUT_ERROR)
        {
            // If read bytes <= 0 we end connection since we have some error
            // it could be timeout
            return;
        }
        if (read_ret_val == ERROR)
        {
            return;
        }

        DATA_INFO_t data_info;
        int ret_val = check_if_correct_DATA_packet(buff, read_bytes, conn,
                                &data_info, curr_package_id);

        if (ret_val == WRONG_SESSION_ID) 
        {
            // This means that somebody else sent us some data since it has 
            // wrong session id (We assume that session id is unique), thus we
            // dont want to stop receiving data from our client so we wait for
            // another package, and send CONRJT to client who sent wrong one.
            send_CONRJT(socket_fd, &client_address, client_address_len,
                        conn->session_id);
            continue;
        }
        else if (ret_val != SUCCESS)
        {
            // Our client sent package with wrong data so we end connection
            send_RJT(socket_fd, &client_address, client_address_len,
                    conn->session_id, data_info.package_id);
            return;
        }

        curr_package_id++;
        bytes_recvd += data_info.nbr_of_bytes_in_packet;

        print_data_to_stdout(buff + sizeof(data_info), data_info.package_id, data_info.nbr_of_bytes_in_packet);

        printf("bytes recvd: %" PRIu64 ", bytes_to_receive: %" PRIu64 "\n", bytes_recvd, bytes_to_receive);
    }

    // if bytes_recvd == bytes_to_receive this means that we've got all declared
    // data and thus we need to send rcvd msg
    if (bytes_recvd == bytes_to_receive)
        send_RCVD(socket_fd, &client_address, client_address_len, conn->session_id);
    else
        send_RJT(socket_fd, &client_address, client_address_len, conn->session_id, curr_package_id);
}

int do_retransmission(int socket_fd, struct sockaddr_in *client_address, socklen_t client_address_len, CONN *conn, bool is_first_DATA_packet, int *nbr_of_retransmits, uint64_t curr_package_id)
{

    (*nbr_of_retransmits)++;
    if (nbr_of_retransmits > MAX_RETRANSMITS)
    {
        make_error_msg(__FUNCTION__, " - nbr of available retransmits have been reached");
        return ERROR;
    }
    // We got timeout error so we send CONACC again if we havent got 
    // first DATA packet
    if (is_first_DATA_packet)
    {
        send_CONACC(socket_fd, client_address, client_address_len, conn->session_id);
    }
    else
    {
        send_ACC(socket_fd, client_address, client_address_len, conn->session_id, curr_package_id);
    }
    return SUCCESS;
}

// Once we establish connection with client we need to ignore other CONN packets
// sent by him and also DATA packets with package_id less than curr_id, since we
// could be interrupted while talking to him and he might send a few the same 
// DATA packets using retransmission.
// in UDPR we need to know the address of client who sent us conn, thus we need
// to have two additional function parameters that remember it.
void UDPR_data_receive(int socket_fd, char *buff, CONN *conn, struct sockaddr_in *curr_client_addr, socklen_t curr_client_addr_len)
{
    const uint64_t bytes_to_receive = conn->nbr_of_bytes_to_be_sent;
    uint64_t bytes_recvd = 0;
    uint64_t curr_package_id = 0;
    int nbr_of_retransmits = 0;
    bool is_first_DATA_packet = true;
    static struct sockaddr_in client_address;
    static socklen_t client_address_len = (socklen_t)sizeof(client_address);

    while (bytes_recvd < bytes_to_receive)
    {
        printf("waiting for packet [%lu]\n", curr_package_id);
        ssize_t read_bytes; 

        int read_ret_val = read_data_to_buffer(socket_fd, buff, 
                                                 RECEIVE_BUFFOR_SIZE,
                                                 &client_address,
                                                 &client_address_len,
                                                 &read_bytes);

        // No matter whether timeout or we read <= bytes we do the retransmit
        if (read_ret_val == TIMEOUT_ERROR || read_ret_val == ERROR)
        {
            // if do retransmission is not eq SUCCESS this means that 
            // nbr_of_retransmits is greater than MAX_RETRANSMITS so we end 
            // connection. Since read_data_to_buff was unsuccessful 
            // client_address and client_address_len variables are uninitialized
            // thus we need to remember addres of client who sent CONN to us in
            // function parameter
            if (do_retransmission(socket_fd, curr_client_addr, curr_client_addr_len, conn, is_first_DATA_packet, 
            &nbr_of_retransmits, curr_package_id) != SUCCESS)
            {
                return;
            }
            continue;
        }

        // If we succeded getting data into buffer we now need to check this 
        // data
        DATA_INFO_t data_info;
        int ret_val = check_if_correct_DATA_packet(buff, read_bytes, conn,
                                &data_info, curr_package_id);

        if (ret_val == WRONG_SESSION_ID) 
        {
            // This means that somebody else sent us some data since it has 
            // wrong session id (We assume that session id is unique), thus we
            // dont want to stop receiving data from our client so we wait for
            // another package, and send CONRJT to client who sent wrong one.
            // We also dont do the retransmission since data from our client 
            // might wait for us in queue
            send_CONRJT(socket_fd, &client_address, client_address_len,
                        conn->session_id);
            continue;
        }
        else if (ret_val == WRONG_PACKAGE_ID)
        {
            // If we get data with correct session id (this means its from our 
            // client) and if data package has wrong id we check if this id is
            // less than current id we want, if it is we do the retransmission
            // if not we send RJT because data was send in wrong order.
            if (data_info.package_id < curr_package_id)
            {
                if (do_retransmission(socket_fd, client_address, client_address_len, conn, is_first_DATA_packet, 
                &nbr_of_retransmits, curr_package_id) != SUCCESS)
                {
                    return;
                }
            }
            else
            {
                // Our client sent package with wrong data, 
                send_RJT(socket_fd, &client_address, client_address_len,
                        conn->session_id, data_info.package_id);

                return;
            }
            continue; 
        }
        else if (ret_val != SUCCESS)
        {
            send_RJT(socket_fd, &client_address, client_address_len,
                    conn->session_id, data_info.package_id);

            return;
        }

        // We got correct DATA packet so we send ACC to client with its id
        send_ACC(socket_fd, client_address, client_address_len, conn->session_id, curr_package_id);

        is_first_DATA_packet = false;
        curr_package_id++;
        bytes_recvd += data_info.nbr_of_bytes_in_packet;

        print_data_to_stdout(buff + sizeof(data_info), data_info.package_id, data_info.nbr_of_bytes_in_packet);

        printf("bytes recvd: %" PRIu64 ", bytes_to_receive: %" PRIu64 "\n", bytes_recvd, bytes_to_receive);
    }

    if (bytes_recvd == bytes_to_receive)
        send_RCVD(socket_fd, &client_address, client_address_len, conn->session_id);
    else
        send_RJT(socket_fd, &client_address, client_address_len, conn->session_id, curr_package_id);
}

void UDP_server_handler(int socket_fd, struct sockaddr_in *server_address)
{
    printf("UDPserver is listening on port %" PRIu16 "\n",
           ntohs(server_address->sin_port));

    static char buff[RECEIVE_BUFFOR_SIZE];

    while (true)
    {
        // If the timeout is set to zero (the default) then the operation 
        // will never timeout
        struct timeval no_timeout = {.tv_sec = 0, .tv_usec = 0};
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &no_timeout, 
                                                        sizeof no_timeout);
        // We dont want to set timeout for our socket here since now we are 
        // waiting for new connection, thus we can wait a long time.
        struct sockaddr_in client_address;
        socklen_t client_address_len = (socklen_t)sizeof(client_address);
        ssize_t read_bytes;
        int read_ret_val = read_data_to_buffer(socket_fd, buff, 
                                                        RECEIVE_BUFFOR_SIZE,
                                                        &client_address,
                                                        &client_address_len,
                                                        &read_bytes);
        if (read_ret_val < 0)
            continue;

        CONN conn;

        if (check_if_correct_CONN(buff, read_bytes, &conn) != SUCCESS)
        {
            send_CONRJT(socket_fd, &client_address, client_address_len,
                        conn.session_id);
            continue;
        }

        if (send_CONACC(socket_fd, &client_address, client_address_len,
                        conn.session_id) != SUCCESS)
        {
            continue;
        }

        // Only after establishing new connection we set timeout for our socket
        // so that we won't wait eternity for msg from client, since he may not
        // send it
        struct timeval timeout = {.tv_sec = MAX_WAIT, .tv_usec = 0};
        setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof timeout);

        switch (conn.protocol_id)
        {
            case UDP_PROTOCOL:
                printf("Will be receiving data from client\n");
                UDP_data_receive(socket_fd, buff, &conn);
                break;
            case UDPR_PROTOCOL:
                printf("UDPR server will be receiving data from client\n");
                UDPR_data_receive(socket_fd, buff, &conn, client_address, client_address_len);
                break;
            default:
                make_error_msg(__FUNCTION__, " - unknown protocol type");
                break;
        }
    }
}
