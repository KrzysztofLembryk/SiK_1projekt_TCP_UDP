#include <unistd.h>
#include <stdio.h>
#include "client_tests_lib.h"
#include "helper_func.h"
#include "packet_structures.h"
#include "client_TCP_lib.h"
#include "common.h"
#include "protconst.h"

#define WRONG_CONN 0
#define WRONG_DATA 1

int TCP_send_package(int socket_fd, void *package, size_t pacakte_size)
{
    ssize_t written_length = writen(socket_fd, package, pacakte_size);

    if (written_length < 0)
    {
        make_error_msg(__FUNCTION__, " - writen < 0");
        return ERROR;
    }
    else if ((size_t)written_length != pacakte_size)
    {
        make_error_msg(__FUNCTION__, " - write len not eq sizeof CONN");
        return ERROR;
    }
    return SUCCESS;
}

void send_WRONG_CONN(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, unsigned int session_id, bool is_TCP)
{
    const int BAD_SESSION_ID __attribute__((unused)) = 0;
    const int WRONG_PROTOCOL __attribute__((unused)) = 1;
    const int WRONG_PACKAGE_TYPE __attribute__((unused)) = 2;
    const int CONNECT_AND_WAIT __attribute__((unused)) = 3;
    const int CONNECT_SEND_WAIT __attribute__((unused)) = 4;
    int i = 0;
    CONN conn;

    while (true)
    {
        switch (i)
        {
        case BAD_SESSION_ID:
            if (is_TCP)
            {
                init_CONN(&conn, 2137, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                // second send to check if connection was closed by server
                DATA data;
                init_DATA(&data, 2136, 0, vec->occupied_size, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size);
            }
            break;
        case WRONG_PROTOCOL:
            if (is_TCP)
            {
                init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
                // second send to check if connection was closed by server
                TCP_client_send_CONN(socket_fd, &conn);
            }
            break;
        case WRONG_PACKAGE_TYPE:
            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);

                conn.package_type_id = DATA_ID;

                TCP_client_send_CONN(socket_fd, &conn);
                // second send to check if connection was closed by server
                TCP_client_send_CONN(socket_fd, &conn);
            }
            break;
        case CONNECT_AND_WAIT:
            if (is_TCP)
            {
                sleep(MAX_WAIT + 1);
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
            }
            break;
        case CONNECT_SEND_WAIT:
            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
                sleep(MAX_WAIT + 1);
            }
            break;
        default:
            return;
        }

        if (connect(socket_fd, (struct sockaddr *)server_address,
                    (socklen_t)sizeof(*server_address)) < 0)
        {
            make_error_msg(__FUNCTION__, " - cannot connect to the server");
            return;
        }

        i++;
    }
}

void TCP_UDP_client_tests(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, unsigned int session_id, bool is_TCP)
{
    int i = 0;
    while (true)
    {
        // Connect to the server.
        if (connect(socket_fd, (struct sockaddr *)server_address,
                    (socklen_t)sizeof(*server_address)) < 0)
        {
            make_error_msg(__FUNCTION__, " - cannot connect to the server");
            return;
        }
        switch (i)
        {
        case WRONG_CONN:
            send_WRONG_CONN(socket_fd, server_address, vec, session_id, is_TCP);
            break;
        case WRONG_DATA:
            break;
        default:
            return;
        }
        i++;
        sleep(2);
    }
}
