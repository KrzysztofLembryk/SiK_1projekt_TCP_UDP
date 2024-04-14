#include <sys/socket.h>
#include "client_UDP_lib.h"
#include "packet_structures.h"
#include "common.h"
#include "err.h"

#define SEND_FLAGS 0

void UDP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, uint64_t session_id)
{
    CONN conn;

    // init_CONN(&conn, session_id, UDP_PROTOCOL, vec->occupied_size);
    init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);

    socklen_t address_length = (socklen_t)sizeof(*server_address);
    ssize_t sent_length = sendto(socket_fd, &conn, sizeof(conn), SEND_FLAGS,
                                    (struct sockaddr *)server_address, address_length);

    if (sent_length < 0)
    {
        syserr("sendto");
    }
    else if (sent_length != sizeof(conn))
    {
        fatal("incomplete sending");
    }
}