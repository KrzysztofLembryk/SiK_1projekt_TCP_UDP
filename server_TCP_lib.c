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


int TCP_wait_for_client(int socket_fd, int *c_fd, 
                        struct sockaddr_in *client_address)
{
    socklen_t server_addr_len = (socklen_t)sizeof(*client_address); 
    // We wait for client that wants to connect with us on accept function
    int client_fd = accept(socket_fd, (struct sockaddr *) client_address,
                            &server_addr_len);

    if (client_fd < 0) 
    {
        // close(socket_fd);
        make_error_msg(__FUNCTION__, " - client_fd < 0");
        return ERROR;
    }
    
    char const *client_ip = inet_ntoa(client_address->sin_addr);
    uint16_t client_port = ntohs(client_address->sin_port);

    printf("\n####\naccepted connection from %s:%" PRIu16 "\n", client_ip, client_port);

    *c_fd = client_fd;
    return SUCCESS;
}

// Function handles initialization of connection with client. It waits for data
// of type CONN, reads it, and checks whether read data has package_type equal
// to CONN_ID if not, it returns -2. It also checks whether read data has 
// correct protocol equal to TCP_PROTOCOL
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


int TCP_send_CONACC_to_client(int client_fd, CONACC *conacc)
{
    ssize_t written_length = writen(client_fd, conacc, sizeof (*conacc));
    if (written_length < 0 )
    {
        make_error_msg(__FUNCTION__, " - writen returned < 0");
        return ERROR;
    }
    if ((size_t) written_length < sizeof (*conacc)) 
    {
        make_error_msg(__FUNCTION__, " - writen-wrote less than wanted size");
        return ERROR;
    }
    else 
    {
        return SUCCESS;
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

int TCP_send_RJT(int client_fd, RJT *rjt)
{
    ssize_t written_length = writen(client_fd, rjt, sizeof (*rjt));
    if (written_length < 0 )
    {
        error("TCP-send_RJT-writen returned < 0\n");
        return ERROR;
    }
    if ((size_t) written_length < sizeof (*rjt)) 
    {
        error("TCP-send_RJT_to_client-writen-wrote less than wanted size\n");
        return ERROR;
    }
    else 
    {
        printf("RJT reply sent\n");
        return SUCCESS;
    }
}

int TCP_send_RCVD(int client_fd, RCVD *rcvd)
{
    ssize_t written_length = writen(client_fd, rcvd, sizeof (*rcvd));
    if (written_length < 0 )
    {
        error("TCP-send_RCVD-writen returned < 0\n");
        return ERROR;
    }
    if ((size_t) written_length < sizeof (*rcvd)) 
    {
        error("TCP-send_RCVD_to_client-writen-wrote less than wanted size\n");
        return ERROR;
    }
    else 
    {
        printf("RCVD reply sent\n");
        return SUCCESS;
    }
}

int TCP_read_data_to_buf(int client_fd, char *buf, 
                                        uint32_t nbr_of_bytes_in_packet)
{
    ssize_t len = readn(client_fd, buf, nbr_of_bytes_in_packet);
    if (len < 0)
    {
        make_error_msg(__FUNCTION__, " - readn < 0");
        return ERROR;
    }
    if (len != nbr_of_bytes_in_packet)
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

        // We didnt receive correct CONN packet thus we end connection with
        // client and move on 
        if (TCP_handle_conn_init(&conn, client_fd) != SUCCESS)
        {
            close(client_fd);
            continue;
        }  

        ntoh_CONN(&conn);
        print_CONN(&conn);

        CONACC conacc;
        init_CONACC(&conacc, conn.session_id);
        int conacc_ret_val = TCP_send_CONACC_to_client(client_fd, &conacc);

        if (conacc_ret_val != 0)
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
            DATA_INFO_t data_metainfo; 

            if (TCP_get_DATA_metainfo(client_fd, &data_metainfo, 
                conn.session_id, curr_packet_id) != SUCCESS)
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
                make_error_msg(__FUNCTION__, "- Client sent more data than declared");
                wrong_packet_err = true;
                RJT rjt;

                init_RJT(&rjt, conn.session_id, data_metainfo.package_id);

                TCP_send_RJT(client_fd, &rjt);

                close(client_fd);
                break;
            }

            curr_packet_id++;
            memset(buff, 0, sizeof(buff));
            if (TCP_read_data_to_buf(client_fd, buff, data_metainfo.nbr_of_bytes_in_packet) != SUCCESS)
            {
                wrong_packet_err = true;
                RJT rjt;

                init_RJT(&rjt, conn.session_id, data_metainfo.package_id);

                TCP_send_RJT(client_fd, &rjt);

                close(client_fd);
                break;
            }
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