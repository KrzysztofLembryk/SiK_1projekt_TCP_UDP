#include <stdio.h>
#include <inttypes.h>

#include "err.h"
#include "common.h"

int main(int argc, char *argv[])
{
    // Server takes two parameters: 
    // 1) protocole type (tcp, udp)
    // 2) port number on which it listens
    if (argc != 3)
    {
        fatal("usage of %s: <protocol type> <port number>\n", argv[0]);
    }

    uint16_t port = port_from_str_to_ul(argv[2]);
    printf("port: %" PRIu16 "\n", port);

    return 0;
}



