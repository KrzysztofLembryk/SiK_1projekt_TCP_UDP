#include "data_handler_lib.h"
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

#include "err.h"
#include "common.h"
#include "packet_structures.h"
#include "protconst.h"
#include "helper_func.h"


int main()
{
    my_vec_t *vec = read_stdin();
    return 0;
}