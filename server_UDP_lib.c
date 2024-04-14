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

int check_if_CONN(char *buff, ssize_t read_bytes, CONN *conn)
{
    if (cast_buff_to(conn, sizeof(*conn), buff, (size_t)read_bytes) != SUCCESS)
        return -1;

    ntoh_CONN(conn);

    if (conn->package_type_id != CONN_ID)
    {
        make_error_msg(__FUNCTION__, " - package type id is not CONN");
        return -1;
    }
    if (conn->protocol_id != UDP_PROTOCOL || conn->protocol_id != UDPR_PROTOCOL)
    {
        make_error_msg(__FUNCTION__, " - protocol is not udp or udpr");
        return -1;
    }

    print_CONN(conn);

    return 0;
}

void UDP_server_handler(int socket_fd, struct sockaddr_in *server_address)
{
    printf("UDPserver is listening on port %" PRIu16 "\n",
           ntohs(server_address->sin_port));

    static char buff[BUFFOR_SIZE];

    while (true)
    {
        memset(buff, 0, sizeof(buff));

        struct sockaddr_in client_address;
        socklen_t client_address_len = (socklen_t)sizeof(client_address);

        // UDP gets data as datagrams that are stored in queue, so after we do
        // recvfrom, we read whole datagram from queue, so if we dont have
        // enough space in buffor part of data is lost. Thus first we will read
        // whole datagram into buffer, then cast it on our structures i.e. CONN.
        ssize_t read_bytes = recvfrom(socket_fd, buff, BUFFOR_SIZE,
                                      DEFAULT_FLAG,
                                      (struct sockaddr *)&client_address,
                                      (socklen_t *)&client_address_len);
        if (read_bytes < 0)
        {
            make_error_msg(__FUNCTION__, " - read_bytes < 0");
            // SEND CONNRJCT or sth
            continue;
        }

        CONN conn;

        if (check_if_CONN(buff, read_bytes, &conn) != SUCCESS)
        {
            // send CONNRJT
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
            make_error_msg(__FUNCTION__, " - unknown protocol type")
            break;
        }
    }
}
