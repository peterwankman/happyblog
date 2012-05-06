/*
 * Happyblog -- A Blog in the imperative programming Language C
 * (C) 2012 Martin Wolters
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/wtfpl/COPYING for more details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sqlite3.h>

#include "sha1.h"

#define MAXBUF 512

static void usage(char *argv) {
	printf("USAGE: %s [-u hash] [database] ...\n", argv);
}

static unsigned int hex2int(char *in) {
	unsigned int out = 0;
	int i;

	for(i = 0; i < 8; i++) {
		out <<= 4;

		switch(in[i]) {
			case '0': out += 0; break;
			case '1': out += 1; break;
			case '2': out += 2; break;
			case '3': out += 3; break;
			case '4': out += 4; break;
			case '5': out += 5; break;
			case '6': out += 6; break;
			case '7': out += 7; break;
			case '8': out += 8; break;
			case '9': out += 9; break;
			case 'a': out += 10; break;
			case 'b': out += 11; break;
			case 'c': out += 12; break;
			case 'd': out += 13; break;
			case 'e': out += 14; break;
			case 'f': out += 15; break;
		}
	}

	return out;
}

static void addpost(char *post, size_t size, unsigned int uhash, char *dbname) {
	sqlite3 *db;
	sqlite3_stmt *statement;
	time_t posttime;
	hash_t hash;
	
	printf("Adding post to database '%s'...\n", dbname);
	
	if(sqlite3_open(dbname, &db)) {
		fprintf(stderr, "ERROR: Could not open database '%s': %s\n", dbname,
			sqlite3_errmsg(db));
		return;
	}

	posttime = time(NULL);

	hash = sha1(post, size);

	if(uhash == 0) {
		if(sqlite3_prepare(db, "INSERT INTO entries (id, hash, time, entry) "
			"VALUES (NULL, :hsh, :tim, :pst);", MAXBUF, &statement, NULL) != 
			SQLITE_OK) {
			fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return;
		}

		sqlite3_bind_int(statement, 1, hash.h4);
	} else {
		if(sqlite3_prepare(db, "INSERT INTO updates (id, hash, time, entry) "
			"VALUES (NULL, :hsh, :tim, :pst);", MAXBUF, &statement, NULL) != 
			SQLITE_OK) {
			fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
			sqlite3_close(db);
			return;
		}

		sqlite3_bind_int(statement, 1, uhash);
	}

	sqlite3_bind_int(statement, 2, posttime);
	sqlite3_bind_text(statement, 3, post, size, SQLITE_STATIC);
	
	sqlite3_step(statement);
	sqlite3_finalize(statement);

	sqlite3_close(db);
}

int main(int argc, char **argv) {
	char buf[MAXBUF], *post = NULL;
	size_t oldsize, size = 0;
	int i;
	unsigned int hash = 0;

	if(argc < 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if(!strcmp(argv[1], "-u")) {
		if(argc < 4) {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
		hash = hex2int(argv[2]);
	}

	fgets(buf, MAXBUF, stdin);
	while(strlen(buf) > 1) {
		oldsize = size;
		size += strlen(buf);
		if((post = realloc(post, size)) == NULL) {
			fprintf(stderr, "ERROR: realloc() failed.\n");
			if(post)
				free(post);
			return 1;
		}
		post[oldsize] = '\0';

		strcat(post, buf);

		fgets(buf, MAXBUF, stdin);
	}

	if(hash == 0)
		i = 1;
	else
		i = 3;
	for(; i < argc; i++)
		addpost(post, size, hash, argv[i]);
	
	free(post);
	return 0;
}
