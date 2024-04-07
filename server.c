#include <stdio.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <string.h>
// includes sockaddr:
#include <netinet/in.h>
// includes htonl etc.:
#include <arpa/inet.h>
// includes htobe64 etc.:
#include <endian.h>
#include <stdbool.h>

#include "err.h"
#include "common.h"
#include "packet_structures.h"

#define QUEUE_LEN 5

typedef enum server_type {TCP, UDP} server_type;

server_type check_type_of_server(const char* input)
{
    if (strcmp(input, "tcp") == 0)
    {
        return TCP;
    }
    else if (strcmp(input, "udp") == 0)
    {
        return UDP;
    }
    else
        fatal("given protocol type is not tcp nor udp\n");
}

void init_socket_fd(int *socket_fd, server_type type)
{
    if (type == TCP)
        *socket_fd = socket(AF_INET, SOCK_STREAM, 0);
    else
        *socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (*socket_fd < 0)
    {
        syserr("cannot create a socket");
    }
}

void TCP_handler(int socket_fd, struct sockaddr_in *server_address)
{
    // Since its TCP server we switch its socket to listening
    if (listen(socket_fd, QUEUE_LEN) < 0) 
        syserr("TCP-listen-error\n");
    
    printf("address before getsockname %" PRId32 "\n", server_address->sin_addr.s_addr);

    //  
    socklen_t lenght = (socklen_t) sizeof (*server_address);
    if (getsockname(socket_fd, (struct sockaddr *) server_address, &lenght) < 0)
        syserr("getsockname");


    printf("TCPserver-parent is listening on port %" PRIu16 "\n", 
        ntohs(server_address->sin_port));
    
    while(true)
    {
        // We wait for client that wants to connect with us on accept function
        struct sockaddr_in client_address;
        int client_fd = accept(socket_fd, (struct sockaddr *) &client_address,
                               &((socklen_t){sizeof(client_address)}));
        if (client_fd < 0) 
            syserr("TCPserver-accept");
        
    }
}

void UDP_handler(int socket_fd)
{

}

int main(int argc, char *argv[])
{
    // Server takes two parameters: 
    // 1) protocole type (tcp, udp)
    // 2) port number on which it listens
    if (argc != 3)
    {
        fatal("usage of %s: <protocol type> <port number>\n", argv[0]);
    }

    server_type type_of_server = check_type_of_server(argv[1]);

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
        TCP_handler(socket_fd, &server_address);
        break; 
    case UDP:

        break;
    default:
        break;
    }

    return 0;
}



