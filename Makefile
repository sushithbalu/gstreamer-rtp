
CC = gcc
CFLAGS = -Wall -g 
LDFLAGS = `pkg-config --cflags --libs gstreamer-1.0`

SER = rtp_server.c
CLI = rtp_client.c

all:	client server

client:
	$(CC) $(CFLAGS)  $(CLI) $(LDFLAGS) -o client

server:
	$(CC) $(CFLAGS)  $(SER) $(LDFLAGS) -o server

clean:
	rm -rf *.o server client
