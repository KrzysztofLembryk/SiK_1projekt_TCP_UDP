#include <stdio.h>
// for close():
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <string.h>
#include <arpa/inet.h>

#include "err.h"
#include "common.h"
#include "helper_func.h"
#include "protconst.h"
#include "server_TCP_lib.h"
#include "server_UDP_lib.h"


#define QUEUE_LEN 5


int main(int argc, char *argv[])
{
    // Server takes two parameters: 
    // 1) protocole type (tcp, udp)
    // 2) port number on which it listens
    if (argc != 3)
    {
        fatal("usage of %s: <protocol type> <port number>\n", argv[0]);
    }

    communication_type type_of_server = check_communication_type(argv[1]);

    // We read port, and change it from str to uint16
    uint16_t port = port_from_str_to_ul(argv[2]);
    printf("port: %" PRIu16 "\n", port);

    // We create socket on which we will be listening
    // socket(int domain, int type, int protocol)
    // ## domain - family of protocols that will be used for communication:
    // --> AF_INET - IPv4
    // --> AF_INET6 - IPv6
    // ## type - type of connection:
    // --> SOCK_STREAM - TCP
    // --> SOCK_DGRAM - UDP
    // ## protocol - specifies protocol, default protocol = 0 is used
    int socket_fd;
    init_socket_fd(&socket_fd, type_of_server);

    // Now we create socket address to which we will bind the socket. We have 
    // to do it since newly created socket has no address thus cannot be seen 
    // other processes (clients)
    struct sockaddr_in server_address;

    // We created socket with IPv4 protocol so we need to choose the same one
    // here for address
    server_address.sin_family = AF_INET;

    // Numbers need to be in network byte order so we convert them by htonl/s.
    // Since we are server we want to listen on all available interfaces
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); 

    // We need to give port which we are using, for our address 
    server_address.sin_port = htons(port);

    // Now we need to bind created address to our socket.
    if (bind(socket_fd, (struct sockaddr *) (&server_address),
                             (socklen_t) sizeof server_address) < 0)
    {
        syserr("binding socket with address unsuccesful");
    }

    // Depending on type of server we need to change how our server behaves.
    // For instance TCP server opens socket in listening mode, whereas UDP 
    // server does not
    switch (type_of_server)
    {
    case TCP:
        TCP_server_handler(socket_fd, &server_address, QUEUE_LEN);
        break; 
    case UDP:
        UDP_server_handler(socket_fd, &server_address);
        break;
    default:
        break;
    }

    return 0;
}



