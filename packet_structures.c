#include "packet_structures.h"
#include <arpa/inet.h>
#include <endian.h>
#include <string.h>
#include <stdio.h>
#include "err.h"
#include "helper_func.h"


// -----INIT FUNCTIONS-----

void init_CONN(CONN *conn, uint64_t session_id, uint8_t protocol_id, 
                uint64_t nbr_of_bytes)
{
    conn->package_type_id = CONN_ID;
    conn->session_id = htobe64(session_id);
    conn->protocol_id = protocol_id;
    conn->nbr_of_bytes_to_be_sent = htobe64(nbr_of_bytes);
}

void init_CONACC(CONACC *conacc, uint64_t session_id)
{
    conacc->package_type_id = CONACC_ID;
    conacc->session_id = htobe64(session_id);
}

void init_CONRJT(CONRJT *conrjt, uint64_t session_id)
{
    conrjt->package_type_id = CONRJT_ID;
    conrjt->session_id = htobe64(session_id);
}

int init_DATA(DATA *data, uint64_t session_id, uint64_t package_id, 
                uint32_t nbr_of_bytes, char *bytes_to_send)
{
    data->package_type_id = DATA_ID;
    data->session_id = htobe64(session_id);
    data->package_id = htobe64(package_id);
    data->nbr_of_bytes_in_packet = htobe32(nbr_of_bytes);

    if (nbr_of_bytes > SEND_BUFF_SIZE)
    {
        error("init_DATA - given nbr of bytes is greater than BUFF SIZE!\n");
        return -1;
    }

    strncpy(data->seq_of_bytes, bytes_to_send, nbr_of_bytes);

    return 0;
}

void init_ACC(ACC *acc, uint64_t session_id, uint64_t package_id)
{
    acc->package_type_id = ACC_ID;
    acc->session_id = htobe64(session_id);
    acc->package_id = htobe64(package_id);
}

void init_RJT(RJT *rjt, uint64_t session_id, uint64_t package_id)
{
    rjt->package_type_id = RJT_ID;
    rjt->session_id = htobe64(session_id);
    rjt->package_id = htobe64(package_id);
}

void init_RCVD(RCVD *rcvd, uint64_t session_id)
{
    rcvd->package_type_id = RCVD_ID;
    rcvd->session_id = htobe64(session_id);
}

// -----PRINT FUNCTIONS-----

void print_CONN(CONN *conn)
{
    printf("[CONN package]:\n");
    printf("package type: %d\n", conn->package_type_id);
    printf("session id: %" PRIu64 "\n", conn->session_id);
    printf("protocol id: %d\n", conn->protocol_id);
    printf("nbr of bytes to send: %" PRIu64 "\n", conn->nbr_of_bytes_to_be_sent);
}

void print_CONACC(CONACC *conacc)
{
    printf("[CONACC package]:\n");
    printf("package type: %d\n", conacc->package_type_id);
    printf("session id: %" PRIu64 "\n", conacc->session_id);
}

void print_CONRJT(CONRJT *conrjt)
{
    printf("[CONRJT package]:\n");
    printf("package type: %d\n", conrjt->package_type_id);
    printf("session id: %" PRIu64 "\n", conrjt->session_id);
}

void print_DATA_INFO(DATA *data)
{
    printf("[DATA package]:\n");
    printf("package type: %d\n", data->package_type_id);
    printf("package id: %" PRIu64 "\n", data->package_id);
    printf("nbr of bytes to send: %" PRIu32 "\n", data->nbr_of_bytes_in_packet);
}

void print_ACC(ACC *acc)
{
    printf("[ACC package]:\n");
    printf("package type: %d\n", acc->package_type_id);
    printf("session id: %" PRIu64 "\n", acc->session_id);
    printf("package id: %" PRIu64 "\n", acc->package_id);
}

void print_RJT(RJT *rjt)
{
    printf("[RJT package]:\n");
    printf("package type: %d\n", rjt->package_type_id);
    printf("session id: %" PRIu64 "\n", rjt->session_id);
    printf("package id: %" PRIu64 "\n", rjt->package_id);
}

void print_RCVD(RCVD *rcvd)
{
    printf("[RCVD package]:\n");
    printf("package type: %d\n", rcvd->package_type_id);
    printf("session id: %" PRIu64 "\n", rcvd->session_id);
}

// -----NTOH FUNCTIONS-----

void ntoh_CONN(CONN *conn)
{
    conn->session_id = be64toh(conn->session_id);
    conn->nbr_of_bytes_to_be_sent = be64toh(conn->nbr_of_bytes_to_be_sent);
}

void ntoh_CONACC(CONACC *conacc)
{
    conacc->session_id = be64toh(conacc->session_id);
}

void ntoh_DATA_INFO(DATA_INFO_t *d_info)
{
    d_info->session_id = be64toh(d_info->session_id);
    d_info->package_id = be64toh(d_info->package_id);
    d_info->nbr_of_bytes_in_packet = be32toh(d_info->nbr_of_bytes_in_packet);
}

// -----CAST FUNCTIONS-----

// Function casts bytes_in_buffer size data stored in buffer, to given void *ptr
// it also checks if ptr_size == bytes_in_buff if not it returns -1 
// it also handles exception considering DATA_INFO structure, which can have 
// size smaller than nbr of bytes in buff 
int cast_buff_to(void *ptr, size_t ptr_size, char *buff, size_t bytes_in_buff)
{
    // First byte in each package is package_type_id, only DATA package can have
    // greater nbr of bytes in buffer than sizeof(DATA), since additional bytes
    // are real data that was sent.
    if (bytes_in_buff != ptr_size && buff[0] != DATA_ID)
    {
        make_error_msg(__FUNCTION__, " - recv package size not equal to given packet size");

        return -1;
    }

    memcpy(ptr, buff, ptr_size);

    return 0;
}