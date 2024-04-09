CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu17
TARGETS = ppcb_client ppcb_server 

all: $(TARGETS)

ppcb_server: server.o common.o err.o helper_func.o packet_structures.o

ppcb_client: ppcb_client.o

common.o: common.c err.h common.h
err.o: err.c err.h
helper_func.o: helper_func.c helper_func.h err.h
packet_structures.o: packet_structures.c packet_structures.h
ppcb_client.o: ppcb_client.c
server.o: server.c err.h common.h packet_structures.h protconst.h \
 helper_func.h

clean:
	rm -f $(TARGETS) *.o