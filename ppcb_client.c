#include "data_handler_lib.h"
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

#include "err.h"
#include "common.h"
#include "packet_structures.h"
#include "protconst.h"
#include "helper_func.h"

void TCP_client_handler(int socket_fd, struct sockaddr_in *server_address)
{

}

int main(int argc, char *argv[])
{
    // if (argc > 4 || argc < 3) 
    //     fatal("usage: %s <protocol type> (<host> <port>) or <server address:port>", argv[0]);

    if (argc != 4) 
        fatal("usage: %s <protocol type> (<host> <port>) or <server address:port>", argv[0]);

    communication_type type_of_comm = check_communication_type(argv[1]);
    const char *host = argv[2];
    uint16_t port = port_from_str_to_ul(argv[3]);
    struct sockaddr_in server_address = get_server_address(host, port);

    printf("connecting to host: %s, port: %d\n", host, port);
    printf("server_address.sin_addr.s_addr: %d\n", server_address.sin_addr.s_addr);

    int socket_fd;
    init_socket_fd(&socket_fd, type_of_comm);

    switch (type_of_comm)
    {
    case TCP:
        TCP_client_handler(socket_fd, &server_address);
        break; 
    case UDP:

        break;
    default:
        break;
    }

    my_vec_t *vec = read_stdin();
    return 0;
}