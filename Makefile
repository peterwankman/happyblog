BIN=bin
INC=include
OBJ=objects
SRC=src
CGI=/usr/lib/cgi-bin

CC=gcc
CFLAGS=-O0 -Wall -I$(INC)
LDFLAGS=-lsqlite3

RSS=-DRSS

all:
	make $(BIN)/blag.cgi
	make $(BIN)/blag-rss.cgi
	make $(BIN)/createdb
	make $(BIN)/initrss
	make $(BIN)/post

$(BIN)/blag.cgi: $(SRC)/webapp.c
	$(CC) $(CFLAGS) $(LDFLAGS) $(RSS) -o $@ $^

$(BIN)/blag-rss.cgi: $(SRC)/rss.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN)/createdb: $(SRC)/createdb.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN)/initrss: $(SRC)/initrss.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(BIN)/post: $(OBJ)/sha1.o $(SRC)/post.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(OBJ)/sha1.o: $(SRC)/sha1.c
	$(CC) $(CFLAGS) -c -o $@ $^

install: $(BIN)/blag.cgi
	cp -f $(BIN)/blag.cgi $(CGI)
	cp -f $(BIN)/blag-rss.cgi $(CGI)

clean:
	rm -f $(OBJ)/*
	rm -f $(BIN)/*
