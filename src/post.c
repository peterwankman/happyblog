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

#include <sqlite3.h>

#include "sha1.h"

#define MAXBUF 512

void usage(char *argv) {
	printf("USAGE: %s [database] ...\n", argv);
}

void delnewline(char *in) {
	int i;
	for(i = 0; i < strlen(in); i++)
		if(in[i] == '\n')
			in[i] = ' ';
}

void addpost(char *post, size_t size, char *dbname) {
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

	if(sqlite3_prepare(db, "INSERT INTO entries (id, hash, time, entry) "
		"VALUES (NULL, :hsh, :tim, :pst);", MAXBUF, &statement, NULL) != 
		SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return;
	}

	sqlite3_bind_int(statement, 1, hash.h4);
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

	if(argc < 2) {
		usage(argv[0]);
		return 1;
	}

	fgets(buf, MAXBUF, stdin);
	while(strlen(buf) > 1) {
		oldsize = size;
		delnewline(buf);
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

	for(i = 1; i < argc; i++)
		addpost(post, size, argv[i]);
	
	free(post);
	return 0;
}
