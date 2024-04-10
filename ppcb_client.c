// #include "data_handler_lib.h"
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
#include <time.h>
#include <stdlib.h>

#include "err.h"
#include "common.h"
#include "packet_structures.h"
#include "protconst.h"
#include "helper_func.h"
#include "my_vec.h"

my_vec_t *read_stdin()
{
    my_vec_t* my_vec = my_vec_init();
    my_vec_read_stdin(my_vec);

    return my_vec;
}

void TCP_client_handler(int socket_fd, struct sockaddr_in *server_address, my_vec_t *vec, unsigned int session_id)
{
    // Connect to the server.
    if (connect(socket_fd, (struct sockaddr *) server_address,
                (socklen_t) sizeof(*server_address)) < 0) 
    {
        syserr("cannot connect to the server");
    }

    CONN conn;
    init_CONN(&conn, session_id, TCP_PROTOCOL, vec->occupied_size);

    ssize_t written_length = writen(socket_fd, &conn, sizeof(conn));

    if (written_length < 0) 
        syserr("writen");
    else if ((size_t) written_length != sizeof(conn)) 
        fatal("incomplete writen");

    CONACC conacc;
    ssize_t read_length = readn(socket_fd, &conacc, sizeof(conacc));

    if (readn_error_handler(read_length, sizeof (conacc)) != 0)
        syserr("readn");
    
    ntoh_CONACC(&conacc);

    printf("Ive got CONACC\n");
    printf("package type id: %d\n", conacc.package_type_id);
    printf("session id: %" PRIu64 "\n", conacc.session_id);
}

int main(int argc, char *argv[])
{
    // if (argc > 4 || argc < 3) 
    //     fatal("usage: %s <protocol type> (<host> <port>) or <server address:port>", argv[0]);

    if (argc != 4) 
        fatal("usage: %s <protocol type> (<host> <port>) or <server address:port>", argv[0]);

    srand(time(NULL));   // Initialization, should only be called once.
    unsigned int session_id = rand();      

    my_vec_t *vec = read_stdin();
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
        TCP_client_handler(socket_fd, &server_address, vec, session_id);
        break; 
    case UDP:

        break;
    default:
        break;
    }

    return 0;
}