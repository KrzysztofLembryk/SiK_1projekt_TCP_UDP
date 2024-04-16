#ifndef CONSTANTS_H
#define CONSTANTS_H

// -----TCP SERVER QUEUE LEN-----

#define QUEUE_LEN 5

// -----PROTOCOLS_IDs-----

#define TCP_PROTOCOL 1
#define UDP_PROTOCOL 2
#define UDPR_PROTOCOL 3

// -----PACKET_STRUCTURES_IDs-----

#define CONN_ID 1
#define CONACC_ID 2
#define CONRJT_ID 3
#define DATA_ID 4
#define ACC_ID 5
#define RJT_ID 6
#define RCVD_ID 7

// -----BUFFORS SIZES-----

// #define SEND_BUFF_SIZE 32000
#define SEND_BUFF_SIZE 10
#define RECEIVE_BUFFOR_SIZE 65000

// -----RETURN VALUES and ERROR VALUES-----

#define SUCCESS 0
#define ERROR -1
#define WRONG_SESSION_ID -2
#define WRONG_PACKAGE_TYPE_ID -3
#define WRONG_PACKAGE_ID -4
#define WRONG_PACKAGE_SIZE -5
#define TIMEOUT_ERROR -6

// -----SOCKET COMMUNICATION FLAGS-----

#define DEFAULT_FLAG 0
#define SEND_FLAGS 0
#define RECEIVE_FLAGS 0

#endif