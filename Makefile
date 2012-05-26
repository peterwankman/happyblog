BIN=bin
INC=include
OBJ=objects
SRC=src
CGI=/usr/lib/cgi-bin

CC=gcc
CFLAGS=-O0 -Wall -ggdb -I$(INC)
LDFLAGS=-lsqlite3

#DDATE=-DDDATE

all:
	make $(BIN)/blag.cgi
	make $(BIN)/blag-rss.cgi
	make $(BIN)/createdb
	make $(BIN)/post

$(BIN)/blag.cgi: $(OBJ)/help.o $(OBJ)/ddate.o $(SRC)/webapp.c
	$(CC) $(CFLAGS) $(DDATE) -o $@ $(LDFLAGS) $^

$(BIN)/blag-rss.cgi: $(OBJ)/help.o $(SRC)/rss.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

$(BIN)/createdb: $(SRC)/createdb.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

$(BIN)/post: $(OBJ)/sha1.o $(OBJ)/help.o $(SRC)/post.c
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $^

$(OBJ)/sha1.o: $(SRC)/sha1.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJ)/help.o: $(SRC)/help.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(OBJ)/ddate.o: $(SRC)/ddate.c
	$(CC) $(CFLAGS) -c -o $@ $^

install: $(BIN)/blag.cgi
	cp -f $(BIN)/blag.cgi $(CGI)
	cp -f $(BIN)/blag-rss.cgi $(CGI)

clean:
	rm -f $(OBJ)/*
	rm -f $(BIN)/*
