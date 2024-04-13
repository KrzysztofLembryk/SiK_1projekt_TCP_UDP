CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu17
TARGETS = ppcb_client server 

all: $(TARGETS)

server: server.o err.o common.o helper_func.o server_TCP_lib.o \
packet_structures.o

ppcb_client: ppcb_client.o my_vec.o err.o common.o \
packet_structures.o helper_func.o

common.o: common.c err.h common.h
data_handler_lib.o: data_handler_lib.c data_handler_lib.h my_vec.h
err.o: err.c err.h
helper_func.o: helper_func.c helper_func.h err.h
my_vec.o: my_vec.c err.h my_vec.h
packet_structures.o: packet_structures.c packet_structures.h err.h
ppcb_client.o: ppcb_client.c err.h common.h packet_structures.h \
 protconst.h helper_func.h my_vec.h
server.o: server.c err.h common.h helper_func.h protconst.h \
 server_TCP_lib.h packet_structures.h
server_TCP_lib.o: server_TCP_lib.c err.h common.h protconst.h \
 helper_func.h server_TCP_lib.h packet_structures.h

clean:
	rm -f $(TARGETS) *.o