CC = gcc
CFLAGS = -I. -lpthread -Wall -Wextra

LIB = utils.o

all: client server

client: client.o $(LIB)
	$(CC) -o client client.o $(LIB) $(CFLAGS)

server: server.o $(LIB)
	$(CC) -o server server.o $(LIB) $(CFLAGS)

utils.o: utils.c utils.h
	$(CC) -c utils.c $(CFLAGS)

client.o: client.c utils.h
	$(CC) -c client.c $(CFLAGS)

server.o: server.c utils.h
	$(CC) -c server.c $(CFLAGS)

clean:
	rm *.o client server servers.txt