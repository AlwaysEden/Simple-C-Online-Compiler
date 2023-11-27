C = gcc
CFLAGS = -Wall
TARGETS = server client

all: $(TARGETS)

server: occ_server.c
	$(CC) $(CFLAGS) -o server occ_server.c -pthread

client: occ_client.c
	$(CC) $(CFLAGS) -o client occ_client.c

clean:
	rm -f $(TARGETS)

