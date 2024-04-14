#include <unistd.h>
#include <sys/socket.h>
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "common.h"
#include "packet_structures.h"
#include "helper_func.h"
#include "err.h"

// Buffor is 65000 since we need space for 64kB of data and also for packages
// headers
#define BUFFOR_SIZE 65000
#define DEFAULT_FLAG 0
#define SUCCESS 0

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
        make_error_msg(__FUNCTION__, " - read_bytes < 0");
        return -1;
    }
    if (read_bytes == 0)
    {
        make_error_msg(__FUNCTION__, " - read_bytes == 0");
        return -1;
    }
    return read_bytes;
}

int check_if_CONN(char *buff, ssize_t read_bytes, CONN *conn)
{
    if (cast_buff_to(conn, sizeof(*conn), buff, (size_t)read_bytes) != SUCCESS)
        return -1;

    ntoh_CONN(conn);
    print_CONN(conn);

    if (conn->package_type_id != CONN_ID)
    {
        make_error_msg(__FUNCTION__, " - package type id is not CONN");
        return -1;
    }
    if (conn->protocol_id != UDP_PROTOCOL && conn->protocol_id != UDPR_PROTOCOL)
    {
        make_error_msg(__FUNCTION__, " - protocol is not udp nor udpr");
        return -1;
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
        return -1;
    }
    else if (sent_length != sizeof(conrjt))
    {
        make_error_msg(__FUNCTION__, " - sent_len not equal to size of data we wanted to send");
        return -1;
    }
    return 0;
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
        return -1;
    }
    else if (sent_length != sizeof(conacc))
    {
        make_error_msg(__FUNCTION__, " - sent_len not equal to size of data we wanted to send");
        return -1;
    }
    return 0;
}

void UDP_data_receive_handler(int socket_fd, )
{
}

void UPDR_data_receive_handler()
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
