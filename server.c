#include <stdio.h>
// for close():
#include <unistd.h>
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
#include <errno.h>

#include "err.h"
#include "common.h"
#include "packet_structures.h"
#include "protconst.h"
#include "helper_func.h"

#define BUFFOR_SIZE 64000
#define QUEUE_LEN 5

struct sockaddr_in TCP_wait_for_client(int socket_fd, int *c_fd)
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

// Function handles initialization of connection with client. It waits for data
// of type CONN, reads it, and checks whether read data has package_type equal
// to CONN_ID if not, it returns -2. It also checks whether read data has 
// correct protocol equal to TCP_PROTOCOL
int TCP_conn_init_helper(CONN *conn, int client_fd)
{
    ssize_t read_length = readn(client_fd, conn, sizeof (*conn));

    if (readn_error_handler(read_length, sizeof (*conn)) != 0)
        return -1;

    if (conn->package_type_id != CONN_ID)
    {
        error("connection closed - wrong package_type_id\n");
        return -2;
    }
    if (conn->protocol_id != TCP_PROTOCOL)
    {
        error("Wrong protocol\n");
        return -2;
    }
    return 0;
}

int TCP_handle_conn_init(CONN *conn, int client_fd)
{
    int init_ret_val = TCP_conn_init_helper(conn, client_fd);

    return init_ret_val;
}

int TCP_send_CONACC_to_client(int client_fd, CONACC *conacc)
{
    ssize_t written_length = writen(client_fd, conacc, sizeof (*conacc));
    if (written_length < 0 )
    {
        error("TCP-send_CONACC-writen returned < 0\n");
        return -1;
    }
    if ((size_t) written_length < sizeof (*conacc)) 
    {
        error("TCP-send_CONACC_to_client-writen-wrote less than wanted size\n");
        return -1;
    }
    else 
    {
        printf("CONACC reply sent\n");
        return 0;
    }
}

// Function reads only metadata about upcoming data (it also converts
// data_metainfo from network to host byte order), meaning only:
// - uint8_t package_type_id;
// - uint64_t session_id;
// - uint64_t package_id;
// - uint32_t nbr_of_bytes_in_packet; 
// without real data that is being sent, so that we can quickly check if data 
// parameters are correct (i.e if we get consecutive package_id) without wasting
// time and reading all data even though its incorrect
int TCP_get_DATA_metainfo(int client_fd, DATA_INFO_t *data_metainfo, 
                            uint64_t session_id, uint64_t prev_packet_id, 
                            bool *first_packet)
{
    ssize_t read_length = readn(client_fd, data_metainfo, 
                                                    sizeof (*data_metainfo));

    if (readn_error_handler(read_length, sizeof (*data_metainfo)) != 0)
        return -1;

    ntoh_DATA_INFO(data_metainfo);

    if(data_metainfo->package_type_id != DATA_ID)
    {
        error("TCP_get_DATA_metainfo-wrong package type id\n");
        return -1;
    }
    if (data_metainfo->session_id != session_id)
    {
        error("TCP_get_DATA_metainfo-wrong session id\n");
        return -1;
    }
    if (*first_packet)
    {
        *first_packet = false;
        // prev_packet_id = 0 here since its first packet
        if (data_metainfo->package_id != prev_packet_id)
        {
            error("TCP_get_DATA_metainfo-wrong first packet doesnt have package id = 0\n");
            return -1;
        }
    }
    else if (data_metainfo->package_id != prev_packet_id + 1)
    {
        error("TCP_get_DATA_metainfo-wrong not consecutive packet id\n");
        return -1; 
    }

    return 0;
}

int TCP_send_RJT(int client_fd, RJT *rjt)
{
    ssize_t written_length = writen(client_fd, rjt, sizeof (*rjt));
    if (written_length < 0 )
    {
        error("TCP-send_RJT-writen returned < 0\n");
        return -1;
    }
    if ((size_t) written_length < sizeof (*rjt)) 
    {
        error("TCP-send_RJT_to_client-writen-wrote less than wanted size\n");
        return -1;
    }
    else 
    {
        printf("RJT reply sent\n");
        return 0;
    }
}

int TCP_send_RCVD(int client_fd, RCVD *rcvd)
{
    ssize_t written_length = writen(client_fd, rcvd, sizeof (*rcvd));
    if (written_length < 0 )
    {
        error("TCP-send_RCVD-writen returned < 0\n");
        return -1;
    }
    if ((size_t) written_length < sizeof (*rcvd)) 
    {
        error("TCP-send_RCVD_to_client-writen-wrote less than wanted size\n");
        return -1;
    }
    else 
    {
        printf("RCVD reply sent\n");
        return 0;
    }
}

void TCP_read_data_to_buf(int client_fd, char *buf, 
                                        uint32_t nbr_of_bytes_in_packet)
{
    ssize_t len = readn(client_fd, buf, nbr_of_bytes_in_packet);
    if (len < 0)
        error("readn");
}

void TCP_print_data_to_stdout(char *buff, uint64_t package_id, uint32_t buff_len)
{
    printf("[packet: %" PRIu64 "]-->%.*s\n", package_id, (int)buff_len, buff);
}

void TCP_server_handler(int socket_fd, struct sockaddr_in *server_address)
{
    // Since its TCP server we switch its socket to listening
    if (listen(socket_fd, QUEUE_LEN) < 0) 
        syserr("TCP-listen-error\n");
    
    socklen_t length = (socklen_t) sizeof (*server_address);
    if (getsockname(socket_fd, (struct sockaddr *) server_address, &length) < 0)
        syserr("getsockname");

    printf("TCPserver-parent is listening on port %" PRIu16 "\n", 
        ntohs(server_address->sin_port));
    
    while(true)
    {
        int client_fd = -1;
        struct sockaddr_in client_address = TCP_wait_for_client(socket_fd, 
                                                            &client_fd);

        // We need to set time for our client in order to prevent client from 
        // connecting and not sending anything thus blocking our server
        set_timeout_for_client_socket(client_fd, MAX_WAIT);

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

        ntoh_CONN(&conn);

        printf("Ive got CONN PACKET:\n");
        printf("package type id: %d\n", conn.package_type_id);
        printf("session id: %" PRIu64 "\n", conn.session_id);
        printf("protocol id: %d\n", conn.protocol_id);
        printf("nbr of bytes to receive: %" PRIu64 "\n", conn.nbr_of_bytes_to_be_sent);

        CONACC conacc;
        init_CONACC(&conacc, conn.session_id);
        int conacc_ret_val = TCP_send_CONACC_to_client(client_fd, &conacc);

        if (conacc_ret_val != 0)
        {
            close(client_fd);
            continue;
        }

        // Since its static it will be initialised only once, not every time
        // that loop gets here
        static char buff[BUFFOR_SIZE];
        uint64_t total_nbr_of_bytes_to_be_sent = conn.nbr_of_bytes_to_be_sent;
        uint64_t nbr_of_bytes_received = 0;
        uint64_t prev_packet_id = 0;
        bool first_packet = true;
        bool wrong_packet_err = false;

        // We read as long as we don't get declared nbr of bytes of data or
        // there is some error
        while (nbr_of_bytes_received < total_nbr_of_bytes_to_be_sent)
        {
            DATA_INFO_t data_metainfo; 

            if (TCP_get_DATA_metainfo(client_fd, &data_metainfo, 
                conn.session_id, prev_packet_id, &first_packet) != 0)
            {
                // If received packet was incorrect we send RJT to client and 
                // close connection
                wrong_packet_err = true;
                RJT rjt;

                init_RJT(&rjt, conn.session_id, data_metainfo.package_id);

                TCP_send_RJT(client_fd, &rjt);

                close(client_fd);
                break;
            }

            // If sent info about data is correct we read data to buffer and 
            // then we print received data to stodout
            nbr_of_bytes_received += data_metainfo.nbr_of_bytes_in_packet;

            if (nbr_of_bytes_received > total_nbr_of_bytes_to_be_sent) 
            {
                error("Client sent more data than declared");
            }

            memset(buff, 0, sizeof(buff));
            TCP_read_data_to_buf(client_fd, buff, data_metainfo.nbr_of_bytes_in_packet);
            TCP_print_data_to_stdout(buff, data_metainfo.package_id, data_metainfo.nbr_of_bytes_in_packet);
        }

        if (wrong_packet_err)
            continue;

        // Communication was succesful thus we send RCVD to client
        RCVD rcvd;
        init_RCVD(&rcvd, conn.session_id);
        TCP_send_RCVD(client_fd, &rcvd);
        close(client_fd);
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
        TCP_server_handler(socket_fd, &server_address);
        break; 
    case UDP:

        break;
    default:
        break;
    }

    return 0;
}



