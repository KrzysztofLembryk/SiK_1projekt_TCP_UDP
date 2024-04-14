#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "err.h"

#define BUFFER_SIZE 20

/**
 * We read port that we need to know to establish connection with server. Under
 * this port sought server on given IP waits.
 */
static uint16_t port_from_str_to_ul(char const *string)
{
    char *endptr;
    unsigned long port = strtoul(string, &endptr, 10);

    if ((port == ULONG_MAX && errno == ERANGE) ||
        *endptr != 0 ||
        port == 0 ||
        port > UINT16_MAX)
    {
        fatal("%s is not a valid port number", string);
    }

    return (uint16_t)port;
}

static struct sockaddr_in get_server_address(char const *host, uint16_t port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET; // IPv4
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    struct addrinfo *address_result;
    int errcode = getaddrinfo(host, NULL, &hints, &address_result);

    if (errcode != 0)
    {
        fatal("getaddrinfo: %s", gai_strerror(errcode));
    }

    struct sockaddr_in send_address;
    send_address.sin_family = AF_INET; // IPv4
    send_address.sin_addr.s_addr =     // IP address
        ((struct sockaddr_in *)(address_result->ai_addr))->sin_addr.s_addr;
    send_address.sin_port = htons(port); // port from the command line

    freeaddrinfo(address_result);

    return send_address;
}

int main(int argc, char *argv[])
{
    if (argc < 4)
    {
        fatal("usage: %s <host> <port> <n> <k>...\n# n- nbr of packages to send\n# k- size of each package (in bytes)", argv[0]);
    }

    char const *host = argv[1];
    uint16_t port = port_from_str_to_ul(argv[2]);

    struct sockaddr_in server_address = get_server_address(host, port);
    char const *server_ip = inet_ntoa(server_address.sin_addr);
    uint16_t server_port = ntohs(server_address.sin_port);

    // Create a socket.
    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_fd < 0)
    {
        syserr("cannot create a socket");
    }

    int n = atoi(argv[3]);
    int k = atoi(argv[4]);

    for (int i = 0; i < n; i++)
    {
        // Send a message.
        size_t message_length = k;
        // if (message_length >= BUFFER_SIZE)
        // {
        //     fatal("parameters must be less than %zu characters long", (size_t)BUFFER_SIZE);
        // }
        // int message = 

        int send_flags = 0;
        socklen_t address_length = (socklen_t)sizeof(server_address);
        ssize_t sent_length = sendto(socket_fd, argv[i], message_length, send_flags,
                                     (struct sockaddr *)&server_address, address_length);

        if (sent_length < 0)
        {
            syserr("sendto");
        }
        else if (sent_length != (ssize_t)message_length)
        {
            fatal("incomplete sending");
        }

        printf("sent to %s:%" PRIu16 ": '%s'\n", server_ip, server_port, argv[i]);

        // Receive a message. Buffer should not be allocated on the stack.
        static char buffer[BUFFER_SIZE];
        memset(buffer, 0, sizeof(buffer)); // Clean the buffer.

        size_t max_length = sizeof(buffer);
        int receive_flags = 0;
        struct sockaddr_in receive_address;
        address_length = (socklen_t)sizeof(receive_address);
        ssize_t received_length = recvfrom(socket_fd, buffer, max_length, receive_flags,
                                           (struct sockaddr *)&receive_address, &address_length);
        if (received_length < 0)
        {
            syserr("recvfrom");
        }

        printf("received %zd bytes from %s:%u: '%.*s'\n",
               received_length, server_ip, server_port, (int)received_length, buffer);
    }

    close(socket_fd);
    return 0;
}
