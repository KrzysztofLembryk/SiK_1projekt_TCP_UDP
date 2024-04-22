CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu17
TARGETS = ppcb_client server 

all: $(TARGETS)

server: server.o err.o common.o helper_func.o server_TCP_lib.o \
packet_structures.o server_UDP_lib.o

ppcb_client: ppcb_client.o err.o common.o helper_func.o client_TCP_lib.o \
packet_structures.o my_vec.o client_UDP_lib.o client_UDPR_lib.o \
client_tests_lib.o 

client_TCP_lib.o: client_TCP_lib.c err.h common.h protconst.h \
 helper_func.h client_TCP_lib.h packet_structures.h constants.h my_vec.h
client_tests_lib.o: client_tests_lib.c client_tests_lib.h my_vec.h \
 helper_func.h packet_structures.h constants.h client_TCP_lib.h common.h \
 protconst.h
client_UDP_lib.o: client_UDP_lib.c client_UDP_lib.h my_vec.h \
 helper_func.h packet_structures.h constants.h common.h err.h protconst.h
client_UDPR_lib.o: client_UDPR_lib.c client_UDP_lib.h my_vec.h \
 helper_func.h packet_structures.h constants.h common.h err.h protconst.h \
 client_UDPR_lib.h
common.o: common.c err.h common.h helper_func.h
err.o: err.c err.h
helper_func.o: helper_func.c helper_func.h constants.h err.h
my_vec.o: my_vec.c err.h my_vec.h
packet_structures.o: packet_structures.c packet_structures.h constants.h \
 err.h helper_func.h
ppcb_client.o: ppcb_client.c err.h common.h protconst.h helper_func.h \
 client_TCP_lib.h packet_structures.h constants.h my_vec.h \
 client_UDP_lib.h client_UDPR_lib.h client_tests_lib.h
server.o: server.c err.h common.h helper_func.h protconst.h \
 server_TCP_lib.h packet_structures.h constants.h server_UDP_lib.h
server_TCP_lib.o: server_TCP_lib.c err.h common.h protconst.h \
 helper_func.h server_TCP_lib.h packet_structures.h constants.h
server_UDP_lib.o: server_UDP_lib.c constants.h common.h \
 packet_structures.h helper_func.h err.h protconst.h

clean:
	rm -f $(TARGETS) *.o