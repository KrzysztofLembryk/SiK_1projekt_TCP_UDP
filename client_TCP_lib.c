// #include "data_handler_lib.h"
#include <unistd.h>
#include <sys/socket.h>
// includes sockaddr:
#include <netinet/in.h>
// includes htonl etc.:
#include <arpa/inet.h>
// includes htobe64 etc.:
#include <endian.h>
#include <stdbool.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>

#include "err.h"
#include "common.h"
// #include "packet_structures.h"
#include "protconst.h"
#include "helper_func.h"
// #include "my_vec.h"

#define SUCCESS 0

int TCP_client_send_CONN(int socket_fd, CONN *conn)
{
    ssize_t written_length = writen(socket_fd, conn, sizeof(*conn));

    if (written_length < 0) 
    {
        error("TCP_client_send_CONN - writen < 0");
        return -1;
    }
    else if ((size_t) written_length != sizeof(*conn)) 
    {
        error("TCP_client_send_CONN - incomplete writen");
        return -1;
    }
    return 0;
}

int send_data_wrapper(int socket_fd, DATA *data)
{
    ssize_t written_length = writen(socket_fd, data, sizeof(*data));

    if (written_length < 0) 
    {
        error("send_DATA - writen < 0 \n");
        return -1;
    }
    else if ((size_t) written_length != sizeof(*data)) 
    {
        error("send_DATA - incomplete writen\n");
        return -1;
    }

    return 0;
}

int TCP_client_send_DATA(int socket_fd, my_vec_t *vec, uint64_t session_id)
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
                return -1;
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
                return -1;
            }

            bytes_sent += SEND_BUFF_SIZE;
            bytes_left -= SEND_BUFF_SIZE;
            start_cpy_pos += SEND_BUFF_SIZE;
        }

        curr_package_id++;

        if (send_data_wrapper(socket_fd, &data) != SUCCESS)
            return -1;
    }
    return 0;
}

void TCP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, unsigned int session_id)
{
    // Connect to the server.
    if (connect(socket_fd, (struct sockaddr *) server_address,
                (socklen_t) sizeof(*server_address)) < 0) 
    {
        error("cannot connect to the server");
        return;
    }

    CONN conn;

    init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
    if  (TCP_client_send_CONN(socket_fd, &conn) != SUCCESS)
        return;

    ntoh_CONN(&conn);

    CONACC conacc;
    ssize_t read_length = readn(socket_fd, &conacc, sizeof(conacc));

    if (readn_error_handler(read_length, sizeof (conacc)) != SUCCESS)
        return;

    ntoh_CONACC(&conacc);

    sleep(5);
    printf("Sending data\n");

    if (TCP_client_send_DATA(socket_fd, vec, session_id) != SUCCESS)
        return;

    RCVD rcvd;
    read_length = readn(socket_fd, &rcvd, sizeof(rcvd));

    if (readn_error_handler(read_length, sizeof (rcvd)) != SUCCESS)
        return;

    // Session id might be different in received data since we didnt invoke
    // ntoh function for RCVD
    printf("RCVD id: %d, RJT id: %d\n", RCVD_ID, RJT_ID);
    print_RCVD(&rcvd);

    if (rcvd.package_type_id == RCVD_ID) 
        printf("received RCVD\n");
    else
        printf("received RJT\n");
}