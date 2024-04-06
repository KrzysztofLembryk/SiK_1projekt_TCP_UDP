#include <stdio.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <string.h>
// includes sockaddr:
#include <netinet/in.h>
// includes htonl etc.:
#include <arpa/inet.h>
#include <endian.h>
#include "err.h"
#include "common.h"


enum server_type {TCP, UDP};


int main(int argc, char *argv[])
{
    // Server takes two parameters: 
    // 1) protocole type (tcp, udp)
    // 2) port number on which it listens
    if (argc != 3)
    {
        fatal("usage of %s: <protocol type> <port number>\n", argv[0]);
    }

    enum server_type type_of_server;

    if (strcmp(argv[1], "tcp") == 0)
    {
        type_of_server = TCP;
    }
    else if (strcmp(argv[1], "udp") == 0)
    {
        type_of_server = UDP;
    }
    else
        fatal("given protocol type is not tcp nor udp\n");

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
    if (type_of_server == TCP)
        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    else
        socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0)
    {
        syserr("cannot create a socket");
    }





    return 0;
}



