#include <unistd.h>
#include <sys/socket.h>
// // includes sockaddr:
#include <netinet/in.h>
#include <time.h>
#include <stdlib.h>

#include "err.h"
#include "common.h"
#include "protconst.h"
#include "helper_func.h"
#include "client_TCP_lib.h"
#include "client_UDP_lib.h"
#include "client_UDPR_lib.h"
#include "client_tests_lib.h"

my_vec_t *read_stdin()
{
    my_vec_t* my_vec = my_vec_init();
    my_vec_read_stdin(my_vec);

    return my_vec;
}

int main(int argc, char *argv[])
{
    // if (argc > 4 || argc < 3) 
    //     fatal("usage: %s <protocol type> (<host> <port>) or <server address:port>", argv[0]);

    if (argc != 4) 
        fatal("usage: %s <protocol type> (<host> <port>) or <server address:port>", argv[0]);

    srand(time(NULL));   

    uint32_t session_id_significant = rand();      
    uint32_t session_id_less_significant = rand();      
    uint64_t session_id = (uint64_t) session_id_significant << 32 | session_id_less_significant;

    communication_type type_of_comm = check_communication_type(argv[1]);
    const char *host = argv[2];
    uint16_t port = port_from_str_to_ul(argv[3]);
    struct sockaddr_in server_address = get_server_address(host, port);

    printf("connecting to host: %s, port: %d\n", host, port);

    int socket_fd;
    init_socket_fd(&socket_fd, type_of_comm);

    // we set timeout for our socket, since server might never respond, so after
    // MAX_WAIT seconds we will return error
    struct timeval time_o = {.tv_sec = MAX_WAIT, .tv_usec = 0};
    setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &time_o, sizeof time_o);


    // We read stdin so late since before reading it errors might occur 
    // regarding creating socket/checking comm type etc. So we would need to 
    // deallocate our vector after each error, but now since we read input at
    // the end, allocation happens only after all previous operations were 
    // successful
    my_vec_t *vec = read_stdin();

    switch (type_of_comm)
    {
        case TCP:
            if (DO_TESTS)
            {
                printf("DOING TCP TESTING!!!!\n");
                TCP_UDP_client_tests(socket_fd, &server_address, vec, session_id, true);
            }
            else
            {
                TCP_client_handler(socket_fd, &server_address, vec, session_id);
            }
            break; 
        case UDP:
            if (DO_TESTS)
            {
                TCP_UDP_client_tests(socket_fd, &server_address, vec, session_id, false);
            }
            else
            {
                UDP_client_handler(socket_fd, &server_address, vec, session_id);
            }
            break;
        case UDPR:
            UDPR_client_handler(socket_fd, &server_address, vec, session_id);
            break;
        default:
            break;
    }

    close(socket_fd);
    my_vec_destruct(vec);

    return 0;
}