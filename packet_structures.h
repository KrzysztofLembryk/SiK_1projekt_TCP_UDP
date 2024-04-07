#ifndef PACKET_STRUCTURES_H
#define PACKET_STRUCTURES_H

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>

#define CONN_ID 1
#define CONACC_ID 2
#define CONRJT_ID 3
#define DATA_ID 4
#define ACC_ID 5
#define RJT_ID 6
#define RCVD_ID 7
// All sent numbers need to be in network byte order

// -----DATA STRUCTURES-----

// protocol_id:
// 1 - tcp
// 2 - udp
// 3 - udpr (with retransmission) 
typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
    uint8_t protocol_id;
    uint64_t len_of_bytes_seq;
} CONN;

typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
} CONACC;


typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
} CONRJT;

// package_id - we need this attribute to determine correct order of received 
//              packages
// nbr_of_bytes_to_be_sent - determines how many bytes will be stored in
//                           char *seq_of_bytes
typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
    uint64_t package_id;
    uint32_t nbr_of_bytes_to_be_sent; 
    char *seq_of_bytes;
} DATA;

typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
    uint64_t package_id;
} ACC;

typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
    uint64_t package_id;
} RJT;

typedef struct __attribute__((__packed__))
{
    uint8_t package_type_id;
    uint64_t session_id;
} RCVD;

// -----INIT FUNCTIONS-----

void init_CONN(CONN *conn, uint64_t session_id, uint8_t protocol_id, 
                uint64_t nbr_of_bytes);

void init_CONACC(CONACC *conacc, uint64_t session_id);

void init_CONRJT(CONRJT *conrjt, uint64_t session_id);

void init_DATA(DATA *data, uint64_t session_id, uint64_t package_id, 
                uint32_t nbr_of_bytes, char *bytes_to_send);

void init_ACC(ACC *acc, uint64_t session_id, uint64_t package_id);

void init_RJT(RJT *rjt, uint64_t session_id, uint64_t package_id);

void init_RCVD(RCVD *rcvd, uint64_t session_id);

#endif