CC     = gcc
CFLAGS = -Wall -Wextra -O2 -std=gnu17
TARGETS = client proc-server thread-server preforked-server reuse-server

all: $(TARGETS)

client: client.o err.o common.o

proc-server: proc-server.o err.o common.o

thread-server: thread-server.o err.o common.o
	$(CC) $^ -o $@ -lpthread

preforked-server: preforked-server.o err.o common.o

reuse-server: reuse-server.o err.o common.o

# To są zależności wygenerowane automatycznie za pomocą polecenia `gcc -MM *.c`.

err.o: err.c err.h
common.o: common.c err.h common.h
client.o: client.c err.h common.h
preforked-server.o: preforked-server.c err.h common.h
proc-server.o: proc-server.c err.h common.h
reuse-server.o: reuse-server.c err.h common.h
thread-server.o: thread-server.c err.h common.h

clean:
	rm -f $(TARGETS) *.o
