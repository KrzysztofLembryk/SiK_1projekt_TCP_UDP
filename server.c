#include <stdio.h>
// for close():
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>
// #include <string.h>
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

#define QUEUE_LEN 5
#define TCP_PROTOCOL 1
#define UDP_PROTOCOL 2
#define UDPR_PROTOCOL 3

struct sockaddr_in wait_for_client(int socket_fd, int *c_fd)
{
    // We wait for client that wants to connect with us on accept function
    struct sockaddr_in client_address;
    int client_fd = accept(socket_fd, (struct sockaddr *) &client_address,
                            &((socklen_t){sizeof(client_address)}));

    if (client_fd < 0) 
        syserr("TCPserver-accept");
    
    char const *client_ip = inet_ntoa(client_address.sin_addr);
    uint16_t client_port = ntohs(client_address.sin_port);

    printf("accepted connection from %s:%" PRIu16 "\n", client_ip, client_port);

    *c_fd = client_fd;
    return client_address;
}

void set_timeout_for_client_socket(int client_fd)
{
    // Set timeouts for the client socket so that we could prevent one 
    // client connecting and no sending anything thus blocking our server
    struct timeval time_o = {.tv_sec = MAX_WAIT, .tv_usec = 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &time_o, sizeof time_o);
    setsockopt(client_fd, SOL_SOCKET, SO_SNDTIMEO, &time_o, sizeof time_o);
}

// Function handles initialization of connection with client. It waits for data
// of type CONN, reads it, and checks whether read data has package_type equal
// to CONN_ID if not, it returns -2. It also checks whether read data has 
// correct protocol equal to TCP_PROTOCOL
int TCP_conn_init_helper(CONN *conn, int client_fd)
{
    ssize_t read_lenght = readn(client_fd, conn, sizeof (*conn));

    if (read_lenght < 0) {
        if (errno == EAGAIN) 
        {
            printf("timeout\n"); 
            return -1;
        } 
        else 
        {
            error("readn");
        }
    }
    else if (read_lenght == 0) 
    {
        printf("connection closed read_len == 0\n");
        return -1;
    }
    else if ((size_t) read_lenght < sizeof (*conn)) 
    {
        printf("connection closed without providing full data structure\n");
        return -1;
    }

    if (conn->package_type_id != CONN_ID)
    {
        printf("connection closed - wrong package_type_id\n");
        return -2;
    }
    else
    {
        if (conn->protocol_id != TCP_PROTOCOL)
        {
            printf("Wrong protocol\n");
            return -2;
        }
        return 0;
    }
}

int TCP_handle_conn_init(CONN *conn, int client_fd)
{
    int init_ret_val = TCP_conn_init_helper(conn, client_fd);

    if (init_ret_val == -1)
    {
        printf("some error in nbr of read bytes closing connection \n");
        // NIE WIEM CZY TRZEBA TERAZ COS KLIENTOWI WYSLAC
    }
    else if(init_ret_val == -2)
    {
        printf("Wrong package_type_id, closing connection\n");
    }
    return init_ret_val;
}

int TCP_send_CONACC_to_client(int client_fd, CONACC *conacc)
{
    ssize_t written_length = writen(client_fd, conacc, sizeof (*conacc));
    if ((size_t) written_length < sizeof (*conacc)) 
    {
        error("TCP-send_CONACC_to_client-writen-wrote less than wanted size\n");
        return -1;
    }
    else 
    {
        printf("reply sent\n");
        return 0;
    }
}

void TCP_handler(int socket_fd, struct sockaddr_in *server_address)
{
    // Since its TCP server we switch its socket to listening
    if (listen(socket_fd, QUEUE_LEN) < 0) 
        syserr("TCP-listen-error\n");
    
    printf("address before getsockname %" PRId32 "\n", server_address->sin_addr.s_addr);

    socklen_t length = (socklen_t) sizeof (*server_address);
    if (getsockname(socket_fd, (struct sockaddr *) server_address, &length) < 0)
        syserr("getsockname");


    printf("TCPserver-parent is listening on port %" PRIu16 "\n", 
        ntohs(server_address->sin_port));
    
    while(true)
    {
        int client_fd = -1;
        struct sockaddr_in client_address = wait_for_client(socket_fd, 
                                                            &client_fd);

        // We need to set time for our client in order to prevent client from 
        // connecting and not sending anything thus blocking our server
        set_timeout_for_client_socket(client_fd);

        // Now we want to receive CONN packet with package_type = CONN_ID and
        // protocol_id = TCP_PROTOCOL to establish connection with client
        CONN conn;
        conn.package_type_id = 0;
        int init_ret_val = TCP_handle_conn_init(&conn, client_fd);
        
        // We didnt receive correct CONN packet thus we end connection with
        // client and move on 
        if (init_ret_val != 0)
        {
            close(client_fd);
            continue;
        }  

        CONACC conacc;
        init_CONACC(&conacc, conn.session_id);
        int conacc_ret_val = TCP_send_CONACC_to_client(client_fd, &conacc);

        if (conacc_ret_val != 0)
        {
            close(client_fd);
            continue;
        }
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



