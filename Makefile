CC = gcc
CFLAGS = -c

all: main.o socket.o server.o  protocol.o
	$(CC) build/main.o build/socket.o build/server.o build/protocol.o -o build/server

debug: CFLAGS += -g
debug: all

main.o: src/main.c
	$(CC) src/main.c $(CFLAGS) -o build/main.o

server.o: src/server.c
	$(CC) src/server.c $(CFLAGS) -o build/server.o

socket.o: src/socket.c
	$(CC) src/socket.c $(CFLAGS) -o build/socket.o

protocol.o: src/protocol.c
	$(CC) src/protocol.c $(CFLAGS) -o build/protocol.o