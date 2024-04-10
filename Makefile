CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu17
TARGETS = ppcb_client ppcb_server 

all: $(TARGETS)

ppcb_server: server.o common.o err.o helper_func.o packet_structures.o

ppcb_client: ppcb_client.o data_handler_lib.o my_vec.o err.o common.o \
packet_structures.o helper_func.o

common.o: common.c err.h common.h
data_handler_lib.o: data_handler_lib.c data_handler_lib.h my_vec.h
err.o: err.c err.h
helper_func.o: helper_func.c helper_func.h err.h
my_vec.o: my_vec.c err.h my_vec.h
packet_structures.o: packet_structures.c packet_structures.h
ppcb_client.o: ppcb_client.c data_handler_lib.h my_vec.h err.h common.h \
 packet_structures.h protconst.h helper_func.h
server.o: server.c err.h common.h packet_structures.h protconst.h \
 helper_func.h

clean:
	rm -f $(TARGETS) *.o