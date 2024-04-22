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

int make_new_socket(communication_type type_of_comm)
{
    int socket_fd;
    init_socket_fd(&socket_fd, type_of_comm);

    // we set timeout for our socket, since server might never respond, so after
    // MAX_WAIT seconds we will return error
    struct timeval time_o = {.tv_sec = MAX_WAIT, .tv_usec = 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &time_o, sizeof time_o);

    return socket_fd;
}

void TCP_send_WRONG_CONN(int init_socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, bool is_TCP)
{
    printf("-----INFO ABOUT WRONG CONN TESTS - TCP-----\n");
    printf("Tests send CONN package to server with wrong parameters or with wait more than MAX_WAIT = 4\nFor each test we open new socket, but we dont close any in order not to provoke broken pipe error in server\nThere is special tests just for this\n");
    printf("-------------------------------------------\n\n");

    const int BAD_SESSION_ID __attribute__((unused)) = 0;
    const int WRONG_PROTOCOL __attribute__((unused)) = 1;
    const int WRONG_PACKAGE_TYPE __attribute__((unused)) = 2;
    const int CONNECT_AND_WAIT __attribute__((unused)) = 3;
    const int CONNECT_SEND_WAIT __attribute__((unused)) = 4;
    const int BROKEN_PIPE __attribute__((unused)) = 5;
    int i = 0;
    static CONN conn;
    int socket_fd = init_socket_fd;

    if (connect(socket_fd, (struct sockaddr *)server_address,
                (socklen_t)sizeof(*server_address)) < 0)
    {
        make_error_msg(__FUNCTION__, " - cannot connect to the server");
        return;
    }

    while (true)
    {
        switch (i)
        {
        case BAD_SESSION_ID:
            printf("-----BAD SESSION ID-----\n");
            printf("First we send CONN with session id, then we send DATA with wrong session id\n");
            printf("------------------------\n");
            fflush(stdout);

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
            printf("-----WRONG PROTOCOL-----\n");
            printf("We send CONN with wrong protocol type\n");
            printf("------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
            }
            break;
        case WRONG_PACKAGE_TYPE:
            printf("-----WRONG PACKAGE TYPE-----\n");
            printf("We send conn with wrong package type id\n");
            printf("----------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);

                conn.package_type_id = DATA_ID;

                TCP_client_send_CONN(socket_fd, &conn);
            }
            break;
        case CONNECT_AND_WAIT:
            printf("-----CONNECT AND WAIT MAX_WAIT-----\n");
            printf("We connect to server and then before sending CONN we wait MAX_WAIT + 1 seconds\n");
            printf("-----------------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                sleep(MAX_WAIT + 1);
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
            }
            break;
        case CONNECT_SEND_WAIT:
            printf("-----CONNECT SEND WAIT MAX_WAIT-----\n");
            printf("We connecto to server, send CONN and then wait MAX_WAIT + 1 seconds without sending anything\n");
            printf("------------------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
                sleep(MAX_WAIT + 1);
            }
            break;
        case BROKEN_PIPE:
            printf("-----BROKEN PIPE test-----\n");
            printf("We connect to server, send CONN and send it again so that server needs to respond with RJT then we immediately close socket\n");
            printf("--------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);
                TCP_client_send_CONN(socket_fd, &conn);
                close(socket_fd);
            }

            break;
        default:
            return;
        }

        printf("\n");
        i++;
        fflush(stdout);
        sleep(2);
        // printf("connecting to server\n\n");

        socket_fd = make_new_socket(TCP);

        if (connect(socket_fd, (struct sockaddr *)server_address,
                    (socklen_t)sizeof(*server_address)) < 0)
        {
            make_error_msg(__FUNCTION__, " - cannot connect to the server");
        }
    }
}

void TCP_send_WRONG_DATA(int init_socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, bool is_TCP)
{
    printf("-----INFO ABOUT WRONG DATA TESTS - TCP-----\n");
    printf("Tests send correct CONN package to server and then incorrect DATA packet\nAt least 10byte input file is needed\n");
    printf("-------------------------------------\n");
    const int BAD_SESSION_ID __attribute__((unused)) = 1;
    const int WRONG_PACKAGE_TYPE __attribute__((unused)) = 2;
    const int CONNECT_AND_WAIT __attribute__((unused)) = 3;
    const int SECOND_DATA_PACKAGE_WRONG_ID_SMALLER __attribute__((unused)) = 4;
    const int SECOND_DATA_PACKAGE_WRONG_ID_GREATER __attribute__((unused)) = 5;
    const int WRONG_DECLARED_SIZE_IN_CONN_too_much __attribute__((unused)) = 6;
    const int WRONG_DECLARED_SIZE_IN_CONN_too_little __attribute__((unused)) = 7;
    const int WRONG_DECLARED_SIZE_IN_DATA __attribute__((unused)) = 8;
    int i = 1;
    CONN conn;
    int socket_fd = init_socket_fd;
 
    while (i <= 8)
    {
        // printf("connecting to server\n\n");
        fflush(stdout);
        // close(socket_fd);

        if (is_TCP)
            socket_fd = make_new_socket(TCP);
        else
            socket_fd = make_new_socket(UDP);

        if (connect(socket_fd, (struct sockaddr *)server_address,
                    (socklen_t)sizeof(*server_address)) < 0)
        {
            make_error_msg(__FUNCTION__, " - cannot connect to the server");
        }

        switch (i)
        {
        case BAD_SESSION_ID:
            printf("-----BAD SESSION ID of second DATA PACKAGE-----\n");
            printf("We send correct conn, and correct first data packet, but second data packet is send with wrong session id\nCONN packet parameter that stores size of data to send is set to two times of read file size\n");
            printf("-----------------------------------------------\n\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL,
                          2 * vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                // second send to check if connection was closed by server
                DATA data;
                init_DATA(&data, session_id, 0, vec->occupied_size, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size);

                uint64_t id = 2137;
                init_DATA(&data, id, 1, vec->occupied_size, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size);
            }
            break;
        case WRONG_PACKAGE_TYPE:
            printf("-----WRONG PACKAGE TYPE OF DATA-----\n");
            printf("We send data packet with wrong package type\n");
            printf("------------------------------------\n");
            fflush(stdout);
            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;

                init_DATA(&data, session_id, 0, vec->occupied_size, vec->buff);

                data.package_type_id = CONN_ID;

                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size);
            }
            break;
        case CONNECT_AND_WAIT:
            printf("-----CONNECT SEND DATA WAIT MAX_WAIT-----\n");
            printf("We send DATA and then wait for MAX_WAIT + 1 s\n");
            printf("-----------------------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;

                init_DATA(&data, session_id, 0, 6, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + 6);
                sleep(MAX_WAIT + 1);
            }
            break;
        case SECOND_DATA_PACKAGE_WRONG_ID_SMALLER:
            printf("-----SECOND DATA PACKAGE WRONG ID - SMALLER-----\n");
            printf("We send first DATA packet with correct package id, but second with wrong\n");
            printf("------------------------------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;
                char msg[] = "smaller";
                init_DATA(&data, session_id, 0, 7, msg);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + 7);

                init_DATA(&data, session_id, 0, vec->occupied_size - 7, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size - 7);
            }
            break;
        case SECOND_DATA_PACKAGE_WRONG_ID_GREATER:
            printf("-----SECOND DATA PACKAGE WRONG ID - GREATER-----\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;
                char msg[] = "greater";
                init_DATA(&data, session_id, 0, 7, msg);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + 7);

                init_DATA(&data, session_id, 3, vec->occupied_size - 7, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size - 7);
            }
            break;
        case WRONG_DECLARED_SIZE_IN_CONN_too_much:
            printf("-----TOO MUCH DECLARED DATA SIZE IN CONN-----\n");
            printf("We send CONN package with file nbr of bytes to send equal to size + 20\nThen we send read file in two packets\n");
            printf("---------------------------------------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size + 20);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;

                init_DATA(&data, session_id, 0, 5, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + 5);

                init_DATA(&data, session_id, 1, vec->occupied_size, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size);
            }
            break;
        case WRONG_DECLARED_SIZE_IN_CONN_too_little:
            printf("-----TOO LITTLE DECLARED DATA SIZE IN CONN-----\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;

                init_DATA(&data, session_id, 0, 5, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + 5);

                init_DATA(&data, session_id, 1, vec->occupied_size, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + vec->occupied_size);
            }
            break;
        case WRONG_DECLARED_SIZE_IN_DATA:
            printf("-----WRONG DECLARED SIZE IN DATA-----\n");
            printf("so we send our msg and some junk\n");
            printf("-------------------------------------\n");
            fflush(stdout);

            if (is_TCP)
            {
                init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
                TCP_client_send_CONN(socket_fd, &conn);

                DATA data;

                init_DATA(&data, session_id, 0, 5, vec->buff);
                TCP_send_package(socket_fd, &data, sizeof(DATA_INFO_t) + 26);
                sleep(2);
            }
            break;
        default:
            return;
        }
        printf("\n");
        i++;
        fflush(stdout);
        sleep(2);
    }
}

void UDP_send_WRONG_CONN(int init_socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id)
{
    printf("-----INFO ABOUT WRONG CONN TESTS - UDP-----\n");
    printf("Tests send CONN package to server with wrong parameters or with wait more than MAX_WAIT = 4\nFor each test we open new socket, but we dont close any in order not to provoke broken pipe error in server\nThere is special tests just for this\n");
    printf("-------------------------------------------\n\n");

    const int BAD_SESSION_ID __attribute__((unused)) = 0;
    const int WRONG_PROTOCOL __attribute__((unused)) = 1;
    const int WRONG_PACKAGE_TYPE __attribute__((unused)) = 2;
    const int CONNECT_SEND_WAIT __attribute__((unused)) = 3;
    const int BROKEN_PIPE __attribute__((unused)) = 4;
    int i = 0;
    static CONN conn;
    int socket_fd = init_socket_fd;
    socklen_t server_address_len = (socklen_t)sizeof(*server_address);


    while (true)
    {
        switch (i)
        {
        case BAD_SESSION_ID:
            printf("-----BAD SESSION ID-----\n");
            printf("First we send CONN with session id, then we send DATA with wrong session id\n");
            printf("------------------------\n");
            fflush(stdout);

            init_CONN(&conn, 2137, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
                       &conn, sizeof(conn), __FUNCTION__);

            // second send to check if connection was closed by server
            DATA data;
            init_DATA(&data, 2136, 0, vec->occupied_size, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
                       &data, sizeof(data), __FUNCTION__);
            break;
        case WRONG_PROTOCOL:
            printf("-----WRONG PROTOCOL-----\n");
            printf("We send CONN with wrong protocol type\n");
            printf("------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
                    &conn, sizeof(conn), __FUNCTION__);
            
            break;
        case WRONG_PACKAGE_TYPE:
            printf("-----WRONG PACKAGE TYPE-----\n");
            printf("We send conn with wrong package type id\n");
            printf("----------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);

            conn.package_type_id = DATA_ID;
            sendto_wrapper(socket_fd, server_address, server_address_len,
                &conn, sizeof(conn), __FUNCTION__);

            
            break;
        case CONNECT_SEND_WAIT:
            printf("-----CONNECT SEND WAIT MAX_WAIT-----\n");
            printf("We connecto to server, send CONN and then wait MAX_WAIT + 1 seconds without sending anything\n");
            printf("------------------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
                &conn, sizeof(conn), __FUNCTION__);
            sleep(MAX_WAIT + 1);

            break;
        case BROKEN_PIPE:
            printf("-----BROKEN PIPE test-----\n");
            printf("We connect to server, send CONN and send it again so that server needs to respond with RJT then we immediately close socket\n");
            printf("--------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
                &conn, sizeof(conn), __FUNCTION__);
            sendto_wrapper(socket_fd, server_address, server_address_len,
                &conn, sizeof(conn), __FUNCTION__);
            close(socket_fd);
            
            break;
        default:
            return;
        }
        printf("\n");
        i++;
        fflush(stdout);
        sleep(2);
        // printf("connecting to server\n\n");

        socket_fd = make_new_socket(UDP);

        if (connect(socket_fd, (struct sockaddr *)server_address,
                    (socklen_t)sizeof(*server_address)) < 0)
        {
            make_error_msg(__FUNCTION__, " - cannot connect to the server");
        }
    }
}


void UDP_send_WRONG_DATA(int init_socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id)
{
    printf("-----INFO ABOUT WRONG DATA TESTS - UDP-----\n");
    printf("Tests send correct CONN package to server and then incorrect DATA packet\nAt least 10byte input file is needed\n");
    printf("-------------------------------------------\n\n");
    const int BAD_SESSION_ID __attribute__((unused)) = 1;
    const int WRONG_PACKAGE_TYPE __attribute__((unused)) = 2;
    const int CONNECT_AND_WAIT __attribute__((unused)) = 3;
    const int SECOND_DATA_PACKAGE_WRONG_ID_SMALLER __attribute__((unused)) = 4;
    const int SECOND_DATA_PACKAGE_WRONG_ID_GREATER __attribute__((unused)) = 5;
    const int WRONG_DECLARED_SIZE_IN_CONN_too_much __attribute__((unused)) = 6;
    const int WRONG_DECLARED_SIZE_IN_CONN_too_little __attribute__((unused)) = 7;
    const int WRONG_DECLARED_SIZE_IN_DATA __attribute__((unused)) = 8;
    int i = 1;
    static CONN conn;
    static DATA data;
    int socket_fd = init_socket_fd;
    socklen_t server_address_len = (socklen_t)sizeof(*server_address);

    while (true)
    {
        // printf("connecting to server\n\n");
        // fflush(stdout);
        // close(socket_fd);

        socket_fd = make_new_socket(UDP);

        switch (i)
        {
        case BAD_SESSION_ID:
            printf("-----BAD SESSION ID of second DATA PACKAGE-----\n");
            printf("We send correct conn, and correct first data packet, but second data packet is send with wrong session id\nCONN packet parameter that stores size of data to send is set to two times of read file size\n");
            printf("-----------------------------------------------\n\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL,
                        2 * vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);

            // second send to check if connection was closed by server
            init_DATA(&data, session_id, 0, vec->occupied_size, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size, __FUNCTION__);

            uint64_t id = 2137;
            init_DATA(&data, id, 1, vec->occupied_size, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size, __FUNCTION__);

            break;
        case WRONG_PACKAGE_TYPE:
            printf("-----WRONG PACKAGE TYPE OF DATA-----\n");
            printf("We send data packet with wrong package type\n");
            printf("------------------------------------\n");
            fflush(stdout);
            
            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);

            init_DATA(&data, session_id, 0, vec->occupied_size, vec->buff);

            data.package_type_id = CONN_ID;

            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size, __FUNCTION__);

            break;
        case CONNECT_AND_WAIT:
            printf("-----CONNECT SEND DATA WAIT MAX_WAIT-----\n");
            printf("We send DATA and then wait for MAX_WAIT + 1 s\n");
            printf("-----------------------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);

            init_DATA(&data, session_id, 0, 6, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + 6, __FUNCTION__);
            sleep(MAX_WAIT + 1);

            break;
        case SECOND_DATA_PACKAGE_WRONG_ID_SMALLER:
            printf("-----SECOND DATA PACKAGE WRONG ID - SMALLER-----\n");
            printf("We send first DATA packet with correct package id, but second with wrong\n");
            printf("------------------------------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);

            char msg1[] = "smaller";
            init_DATA(&data, session_id, 0, 7, msg1);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + 7, __FUNCTION__);

            init_DATA(&data, session_id, 0, vec->occupied_size - 7, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size - 7, __FUNCTION__);

            break;
        case SECOND_DATA_PACKAGE_WRONG_ID_GREATER:
            printf("-----SECOND DATA PACKAGE WRONG ID - GREATER-----\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);

            char msg2[] = "greater";

            init_DATA(&data, session_id, 0, 7, msg2);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + 7, __FUNCTION__);

            init_DATA(&data, session_id, 3, vec->occupied_size - 7, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size - 7, __FUNCTION__);

            break;
        case WRONG_DECLARED_SIZE_IN_CONN_too_much:
            printf("-----TOO MUCH DECLARED DATA SIZE IN CONN-----\n");
            printf("We send CONN package with file nbr of bytes to send equal to size + 20\nThen we send read file in two packets\nAfter sending our packets we immediately end connection and create new socket to send package as new CLIENT - server should print error since it will get package from someone else than us\n");
            printf("---------------------------------------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size + 20);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);


            init_DATA(&data, session_id, 0, 5, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + 5, __FUNCTION__);

            init_DATA(&data, session_id, 1, vec->occupied_size, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size, __FUNCTION__);

            socket_fd = make_new_socket(UDP);
            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);
            sleep(MAX_WAIT);

            break;
        case WRONG_DECLARED_SIZE_IN_CONN_too_little:
            printf("-----TOO LITTLE DECLARED DATA SIZE IN CONN-----\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);

            init_DATA(&data, session_id, 0, 5, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + 5, __FUNCTION__);

            init_DATA(&data, session_id, 1, vec->occupied_size, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + vec->occupied_size, __FUNCTION__);
            sleep(MAX_WAIT);

            break;
        case WRONG_DECLARED_SIZE_IN_DATA:
            printf("-----WRONG DECLARED SIZE IN DATA-----\n");
            printf("so we send our msg and some junk\n");
            printf("-------------------------------------\n");
            fflush(stdout);

            init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &conn, sizeof(conn), __FUNCTION__);


            init_DATA(&data, session_id, 0, 5, vec->buff);
            sendto_wrapper(socket_fd, server_address, server_address_len,
            &data, sizeof(DATA_INFO_t) + 26, __FUNCTION__);

            break;
        default:
            return;
        }
        printf("\n");
        i++;
        fflush(stdout);
        sleep(2);
    }
}



void TCP_UDP_client_tests(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id, bool is_TCP)
{
    // // Connect to the server.
    // if (connect(socket_fd, (struct sockaddr *)server_address,
    //                 (socklen_t)sizeof(*server_address)) < 0)
    // {
    //     make_error_msg(__FUNCTION__, " - cannot connect to the server");
    //     return;
    // }
    printf("---BETWEEN EACH CONNECTION there is sleep(2)---\n");
    int i = 0;

    while (true)
    {
        switch (i)
        {
        case WRONG_CONN:
            printf("\n||||||||||||||||||||||||||\n");
            printf("-----WRONG CONN TESTS-----\n");
            printf("||||||||||||||||||||||||||\n\n");
            if (is_TCP)
            {
                TCP_send_WRONG_CONN(socket_fd, server_address, vec, session_id, is_TCP);
            }
            else
            {
                UDP_send_WRONG_CONN(socket_fd, server_address, vec, session_id);
            }
            break;
        case WRONG_DATA:
            printf("\n||||||||||||||||||||||||||\n");
            printf("-----WRONG DATA TESTS-----\n");
            printf("||||||||||||||||||||||||||\n\n");
            if (is_TCP)
            {
                TCP_send_WRONG_DATA(socket_fd, server_address, vec, session_id, is_TCP);
            }
            else
            {
                UDP_send_WRONG_DATA(socket_fd, server_address, vec, session_id);
            }
            break;
        default:
            printf("----------TESTS ENDED----------\n");
            return;
        }
        i++;
        sleep(2);
    }
}
