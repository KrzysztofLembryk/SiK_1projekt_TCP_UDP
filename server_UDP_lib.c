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

#include "common.h"
#include "packet_structures.h"
#include "helper_func.h"
#include "err.h"

// Buffor is 65000 since we need space for 64kB of data and also for packages
// headers
#define BUFFOR_SIZE 65000
#define DEFAULT_FLAG 0
#define SUCCESS 0
#define ERROR -1
#define WRONG_SESSION_ID -2
#define TIMEOUT_ERROR -3

// - Function reads maximally BUFFOR_SIZE bytes to buff
// - Before reading function zeros buffer
// - If recvfrom read <= 0 bytes func returns -1, otherwise nbr of bytes read
// - Function sets variables *client_address and client_addr_len
ssize_t read_data_to_buffer(int socket_fd, char *buff,
                            struct sockaddr_in *client_address,
                            socklen_t *client_address_len)
{
    memset(buff, 0, sizeof(buff));

    // UDP gets data as datagrams that are stored in queue, so after we do
    // recvfrom, we read whole datagram from queue, so if we dont have
    // enough space in buffor part of data is lost. Thus first we will read
    // whole datagram into buffer, then cast it on our structures i.e. CONN.
    ssize_t read_bytes = recvfrom(socket_fd, buff, BUFFOR_SIZE,
                                  DEFAULT_FLAG,
                                  (struct sockaddr *)client_address,
                                  (socklen_t *)client_address_len);
    if (read_bytes < 0)
    {
        if (errno == EAGAIN) 
            make_error_msg(__FUNCTION__, " - timeout\n"); 
        else 
            make_error_msg(__FUNCTION__, " - read_bytes < 0");

        return ERROR;
    }
    if (read_bytes == 0)
    {
        make_error_msg(__FUNCTION__, " - read_bytes == 0");
        return ERROR;
    }
    return read_bytes;
}

int check_if_CONN(char *buff, ssize_t read_bytes, CONN *conn)
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

    return 0;
}

int send_CONRJT(int socket_fd, struct sockaddr_in *client_address,
                socklen_t client_address_len, uint64_t session_id)
{
    CONRJT conrjt;

    init_CONRJT(&conrjt, session_id);

    ssize_t sent_length = sendto(socket_fd, &conrjt, sizeof(conrjt),
                                 DEFAULT_FLAG,
                                 (struct sockaddr *)client_address,
                                 client_address_len);
    if (sent_length < 0)
    {
        make_error_msg(__FUNCTION__, " - sent len < 0");
        return ERROR;
    }
    else if (sent_length != sizeof(conrjt))
    {
        make_error_msg(__FUNCTION__, " - sent_len not equal to size of data we wanted to send");
        return ERROR;
    }

    return SUCCESS;
}

int send_CONACC(int socket_fd, struct sockaddr_in *client_address,
                socklen_t client_address_len, uint64_t session_id)
{

    CONACC conacc;

    init_CONACC(&conacc, session_id);

    ssize_t sent_length = sendto(socket_fd, &conacc, sizeof(conacc),
                                 DEFAULT_FLAG,
                                 (struct sockaddr *)client_address,
                                 client_address_len);
    if (sent_length < 0)
    {
        make_error_msg(__FUNCTION__, " - sent len < 0");
        return ERROR;
    }
    else if (sent_length != sizeof(conacc))
    {
        make_error_msg(__FUNCTION__, " - sent_len not equal to size of data we wanted to send");
        return ERROR;
    }
    return SUCCESS;
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
    print_DATA_INFO(d_info);

    if (d_info->session_id != conn->session_id)
    {
        make_error_msg(__FUNCTION__, " - received DATA package has wrong session id");
        return WRONG_SESSION_ID;
    }
    if (d_info->package_type_id != DATA_ID)
    {
        make_error_msg(__FUNCTION__, " - received pacakge_type is not DATA");
        return ERROR;
    }
    if (d_info->package_id != curr_package_id)
    {
        make_error_msg(__FUNCTION__, " - received DATA package has wrong package id");
        return ERROR;
    }
    if (read_bytes - sizeof(*d_info) - d_info->nbr_of_bytes_in_packet != 0)
    {
        make_error_msg(__FUNCTION__, " - nbr of received bytes from client is not equal to decalred nbr of bytes in DATA_INFO header");
        return ERROR;
    }

    return SUCCESS;
}

int UDP_data_receive(int socket_fd, char *buff, CONN *conn)
{
    const uint64_t bytes_to_receive = conn->nbr_of_bytes_to_be_sent;
    uint64_t bytes_recvd = 0;
    uint64_t curr_package_id = 0;
    static struct sockaddr_in client_address;
    static socklen_t client_address_len = (socklen_t)sizeof(client_address);

    while (bytes_recvd < bytes_to_receive)
    {
        ssize_t read_bytes = read_data_to_buffer(socket_fd, buff,
                                                 &client_address,
                                                 &client_address_len);

        if (read_bytes <= 0)
            continue;

        DATA_INFO_t data_info;
        int ret_val = check_if_correct_DATA_packet(buff, read_bytes, conn,
                                &data_info, curr_package_id);

        if (ret_val == WRONG_SESSION_ID) 
        {
            // This means that somebody else sent us some data since it has 
            // wrong session id (We assume that session id is unique), thus we
            // dont want to stop receiving data from our client so we wait for
            // another package, and send CONRJT to client who sent wrong one.
            // send CONRJT
            continue;
        }
        else if (ret_val == ERROR)
        {
            // Our client sent package with wrong data so we end connection
            // send RJT
            break;
        }

        curr_package_id++;
        bytes_recvd += data_info.nbr_of_bytes_in_packet;

        print_data_to_stdout(buff + sizeof(data_info), curr_package_id, data_info.nbr_of_bytes_in_packet);
    }
}

void UPDR_data_receive()
{
}

void UDP_server_handler(int socket_fd, struct sockaddr_in *server_address)
{
    printf("UDPserver is listening on port %" PRIu16 "\n",
           ntohs(server_address->sin_port));

    static char buff[BUFFOR_SIZE];

    while (true)
    {
        struct sockaddr_in client_address;
        socklen_t client_address_len = (socklen_t)sizeof(client_address);
        ssize_t read_bytes = read_data_to_buffer(socket_fd, buff,
                                                 &client_address,
                                                 &client_address_len);
        if (read_bytes <= 0)
            continue;

        CONN conn;

        if (check_if_CONN(buff, read_bytes, &conn) != SUCCESS)
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

        switch (conn.protocol_id)
        {
        case UDP_PROTOCOL:
            /* code */
            break;
        case UDPR_PROTOCOL:
            break;
        default:
            make_error_msg(__FUNCTION__, " - unknown protocol type");
            break;
        }
    }
}
