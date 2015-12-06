CC = gcc
CFLAGS = -g -pedantic -Wall -std=gnu99 -O3 -Wno-unused-result
LIBS = -lpthread
COMPILE = $(CC) $(CFLAGS)
PROGRAMS = client server

all: $(PROGRAMS)

server: server.c server.h
		$(COMPILE) -o $@ $< $(LIBS)

client: client.c
		$(COMPILE) -o $@ $< $(LIBS)

clean:
		rm -rfv *.o $(PROGRAMS)