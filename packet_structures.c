#include "packet_structures.h"
#include <arpa/inet.h>
#include <endian.h>

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

void init_DATA(DATA *data, uint64_t session_id, uint64_t package_id, 
                uint32_t nbr_of_bytes, char *bytes_to_send)
{
    data->package_type_id = DATA_ID;
    data->session_id = htobe64(session_id);
    data->package_id = htobe64(package_id);
    data->nbr_of_bytes_in_packet = htobe32(nbr_of_bytes);
    data->seq_of_bytes = bytes_to_send;
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
