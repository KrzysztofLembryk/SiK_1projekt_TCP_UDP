#ifndef PACKET_STRUCTURES_H
#define PACKET_STRUCTURES_H

#include <inttypes.h>
#include <stddef.h>
#include <sys/types.h>

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


#endif