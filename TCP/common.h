#ifndef MIM_COMMON_H
#define MIM_COMMON_H

uint16_t port_from_str_to_ul(char const *string);
size_t   read_size(char const *string);

struct sockaddr_in get_server_address(char const *host, uint16_t port);

#endif
