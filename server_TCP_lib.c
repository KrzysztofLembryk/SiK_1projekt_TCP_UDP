#include <stdio.h>
// for close():
#include <unistd.h>
#include <sys/socket.h>
// #include <inttypes.h>
#include <string.h>
// includes sockaddr:
#include <netinet/in.h>
// includes htonl etc.:
#include <arpa/inet.h>
// includes htobe64 etc.:
#include <endian.h>
#include <errno.h>

#include "err.h"
#include "common.h"
#include "protconst.h"
#include "helper_func.h"
#include "server_TCP_lib.h"
#include "constants.h"

// - Function waits for new client on accept
// - Function sets sockaddr_in *client_address to client who was accepted
// - Function sets int *c_fd to returned client descriptor by accept
// - On success function returns SUCCESS, otherwise ERROR
int TCP_wait_for_client(int socket_fd, int *c_fd, 
                        struct sockaddr_in *client_address)
{
    socklen_t server_addr_len = (socklen_t)sizeof(*client_address); 

    // We wait for client that wants to connect with us on accept function
    int client_fd = accept(socket_fd, (struct sockaddr *) client_address,
                            &server_addr_len);

    if (client_fd < 0) 
    {
        make_error_msg(__FUNCTION__, " - client_fd < 0");
        return ERROR;
    }
    
    char const *client_ip = inet_ntoa(client_address->sin_addr);
    uint16_t client_port = ntohs(client_address->sin_port);

    printf("\n####\naccepted connection from %s:%" PRIu16 "\n", client_ip, client_port);

    *c_fd = client_fd;
    return SUCCESS;
}

// - Function handles initialization of connection with client. 
// - It waits for CONN packet for MAX_WAIT seconds
// - It checks if received CONN packet has correct package_type_id, protocol_id
// - It checks if nbr of read bytes is equal to sizeof(CONN)
// - It returns ERROR if sth is wrong, otherwise SUCCESS
int TCP_handle_conn_init(CONN *conn, int client_fd)
{
    ssize_t read_length = readn(client_fd, conn, sizeof (*conn));

    if (readn_error_handler(read_length, sizeof (*conn)) != SUCCESS)
        return ERROR;

    if (conn->package_type_id != CONN_ID)
    {
        make_error_msg(__FUNCTION__, " - connection closed - wrong package_type_id");
        return ERROR;
    }
    if (conn->protocol_id != TCP_PROTOCOL)
    {
        make_error_msg(__FUNCTION__, " - wrong protocol type");
        return ERROR;
    }
    if (read_length != sizeof(*conn))
    {
        make_error_msg(__FUNCTION__, " - nbr of bytes read not equal to sizeof CONN");
        return ERROR;
    }
    return SUCCESS;
}

// - Function reads only metadata from received DATA packet meaning only:
// - uint8_t package_type_id; uint64_t session_id; 
// uint64_t package_id; uint32_t nbr_of_bytes_in_packet; 
// - Function checks if values of above parameters of DATA packet are correct
// - Function returns ERROR if any of above parameters is incorrect, otherwise 
// SUCCESS
// - Function changes network byte order to host order of read data_info packet
int TCP_get_DATA_metainfo(int client_fd, DATA_INFO_t *data_metainfo, 
                            uint64_t session_id, uint64_t curr_packet_id)
{
    ssize_t read_length = readn(client_fd, data_metainfo, 
                                                    sizeof (*data_metainfo));

    if (readn_error_handler(read_length, sizeof (*data_metainfo)) != SUCCESS)
        return ERROR;

    ntoh_DATA_INFO(data_metainfo);

    if(data_metainfo->package_type_id != DATA_ID)
    {
        make_error_msg(__FUNCTION__, " - wrong package type id");
        return ERROR;
    }
    if (data_metainfo->session_id != session_id)
    {
        make_error_msg(__FUNCTION__, " - wrong session id");
        return ERROR;
    }
    if (data_metainfo->package_id != curr_packet_id)
    {
        make_error_msg(__FUNCTION__, " - not consecutive packet id");
        return ERROR; 
    }

    return SUCCESS;
}

// - Function sends given packet of size packet_size to client_fd using writen
// - Function returns ERROR when writen returns <= 0 (also handles EPIPE) or 
// when writen size is not equal packet_size, otherwise it returns SUCCESS
int TCP_send_packet(void *packet, size_t packet_size, int client_fd)
{
    printf("before writen in send RJT\n");
    ssize_t written_length = writen(client_fd, packet, packet_size);
    printf("After writen in send RJT\n");

    if (written_length < 0 )
    {
        if (errno == EPIPE)
            make_error_msg(__FUNCTION__, " - writen < 0 --> SIGPIPE signal in write, client closed reading end of socket before server could send msg");
        else
            make_error_msg(__FUNCTION__, " - writen returned < 0");
        return ERROR;
    }
    if ((size_t) written_length < packet_size) 
    {
        make_error_msg(__FUNCTION__, " - writen-wrote less than wanted size");
        return ERROR;
    }
    if (written_length == 0)
    {
        make_error_msg(__FUNCTION__, " - writen len == 0");
        return ERROR;
    }

    return SUCCESS;
}

void handle_RJT_sending(int client_fd, CONN *conn, uint64_t curr_packet_id)
{
    static RJT rjt;

    init_RJT(&rjt, conn->session_id, curr_packet_id);

    TCP_send_packet(&rjt, sizeof(rjt), client_fd);

    close(client_fd);
}

// - Function reads nbr_of_bytes_in_packet to buf using readn
// - Function checks if nbr of read bytes is equal to nbr_of_bytes_in_packet
// - If its not it returns ERROR, it also returns ERROR when read_bytes < 0
// otherwise it returns SUCCESS
int TCP_read_data_to_buf(int client_fd, char *buf, 
                                        uint32_t nbr_of_bytes_in_packet)
{
    ssize_t read_bytes = readn(client_fd, buf, nbr_of_bytes_in_packet);
    if (read_bytes < 0)
    {
        make_error_msg(__FUNCTION__, " - readn < 0");
        return ERROR;
    }
    if (read_bytes != nbr_of_bytes_in_packet)
    {
        make_error_msg(__FUNCTION__, " - nbr of bytes read is not equal to nbr of bytes that should be in DATA packet");
        return ERROR;
    }
    return SUCCESS;
}

void TCP_print_data_to_stdout(char *buff, uint64_t package_id, uint32_t buff_len)
{
    printf("[packet: %" PRIu64 "]:\n%.*s\n", package_id, (int)buff_len, buff);
    // printf("[packet: %" PRIu64 "]:\n", package_id);
}

// - Function handles TCP communication with clients
// - If client connects to our server and makes server wait for packet more
// than MAX_WAIT seconds, server ends connection with client
// - If client sends data with wrong meta info, or sends wrong type of packet
// not following established protocol, server ends connection
// - If communication was successful server sends RCVD packet and ends 
// connection
// - Server closes clients' file descriptors after ending connections
void TCP_server_handler(int socket_fd, struct sockaddr_in *server_address, int queue_len)
{
    // Since its TCP server we switch its socket to listening
    if (listen(socket_fd, queue_len) < 0) 
        syserr("TCP-listen-error\n");
    
    socklen_t length = (socklen_t) sizeof (*server_address);
    if (getsockname(socket_fd, (struct sockaddr *) server_address, &length) < 0)
        syserr("getsockname");

    printf("TCPserver is listening on port %" PRIu16 "\n", 
        ntohs(server_address->sin_port));
    
    while(true)
    {
        int client_fd = -1;
        struct sockaddr_in client_address;

        // If accepting client was not successful we continue and wait again
        // since we don't want to stop server from working
        if (TCP_wait_for_client(socket_fd, &client_fd, &client_address) != SUCCESS)
        {
            continue;
        }

        // We need to set time for our client in order to prevent client from 
        // connecting and not sending anything thus blocking our server
        set_timeout_for_client_socket(client_fd, MAX_WAIT);

        // Now we want to receive CONN packet with package_type = CONN_ID and
        // protocol_id = TCP_PROTOCOL to establish connection with client
        CONN conn;
        conn.package_type_id = 77;

        // If dont receive correct CONN packet we end connection with client
        // and move on.
        if (TCP_handle_conn_init(&conn, client_fd) != SUCCESS)
        {
            close(client_fd);
            continue;
        }  

        ntoh_CONN(&conn);
        print_CONN(&conn);

        // We send CONACC to client to tell him we accepted his connection
        CONACC conacc;

        init_CONACC(&conacc, conn.session_id);

        if (TCP_send_packet(&conacc, sizeof(conacc), client_fd) != SUCCESS)
        {
            close(client_fd);
            continue;
        }

        // Since buff is static it will be initialised only once, not every time
        // that loop gets here
        static char buff[RECEIVE_BUFFOR_SIZE];
        uint64_t total_nbr_of_bytes_to_be_sent = conn.nbr_of_bytes_to_be_sent;
        uint64_t nbr_of_bytes_received = 0;
        uint64_t curr_packet_id = 0;
        bool wrong_packet_err = false;

        // We read as long as we don't get declared nbr of bytes of data or
        // there is some error
        while (nbr_of_bytes_received < total_nbr_of_bytes_to_be_sent)
        {
            static DATA_INFO_t data_metainfo; 

            // If received packet was incorrect we send RJT to client and 
            // close connection
            if (TCP_get_DATA_metainfo(client_fd, &data_metainfo, 
                conn.session_id, curr_packet_id) != SUCCESS)
            {
                wrong_packet_err = true;

                handle_RJT_sending(client_fd, &conn, curr_packet_id);

                break;
            }

            // If sent info about data is correct we read data to buffer and 
            // then we print received data to stodout
            nbr_of_bytes_received += data_metainfo.nbr_of_bytes_in_packet;

            if (nbr_of_bytes_received > total_nbr_of_bytes_to_be_sent) 
            {
                wrong_packet_err = true;

                handle_RJT_sending(client_fd, &conn, curr_packet_id);
                make_error_msg(__FUNCTION__, "- Client sent more data than declared");

                break;
            }

            curr_packet_id++;
            memset(buff, 0, sizeof(buff));

            if (TCP_read_data_to_buf(client_fd, buff, data_metainfo.nbr_of_bytes_in_packet) != SUCCESS)
            {
                wrong_packet_err = true;

                handle_RJT_sending(client_fd, &conn, curr_packet_id);

                break;
            }
            TCP_print_data_to_stdout(buff, data_metainfo.package_id, data_metainfo.nbr_of_bytes_in_packet);
        }

        if (wrong_packet_err)
            continue;

        // Communication was succesful thus we send RCVD to client
        static RCVD rcvd;
        init_RCVD(&rcvd, conn.session_id);
        TCP_send_packet(&rcvd, sizeof(rcvd), client_fd);
        close(client_fd);
    }
}