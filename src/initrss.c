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

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#define RETURN_FAILURE	0
#define RETURN_SUCCESS	1

#define MAXBUF	512

static int addtable(char *baseurl, char *desc, sqlite3 *db) {
	char *errmsg = NULL;
	sqlite3_stmt *statement;

	if(sqlite3_exec(db, "CREATE TABLE rssconfig (desc TEXT, baseurl TEXT);",
		NULL, 0, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return RETURN_FAILURE;
	}

	if(sqlite3_prepare(db, "INSERT INTO rssconfig (desc, baseurl) "
		"VALUES (:dsc, :url);", MAXBUF, &statement, NULL) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		return RETURN_FAILURE;
	}

	sqlite3_bind_text(statement, 1, desc, strlen(desc), SQLITE_STATIC);
	sqlite3_bind_text(statement, 2, baseurl, strlen(baseurl), SQLITE_STATIC);

	sqlite3_step(statement);

	sqlite3_finalize(statement);

	return RETURN_SUCCESS;
}

static void usage(char *argv) {
	printf("USAGE: %s [-h] [-d desc] [-b url] [-o filename]\n", argv);
}

int main(int argc, char **argv) {
	int opt;
	char *filename = NULL;
	char *baseurl = NULL;
	char *desc = "";
	sqlite3 *db;
	
	while((opt = getopt(argc, argv, "hd:b:o:")) != -1) {
		switch(opt) {
			case 'h': usage(argv[0]); return EXIT_SUCCESS;
			case 'd': desc = optarg; break;
			case 'b': baseurl = optarg; break;
			case 'o': filename = optarg; break;
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if((filename == NULL) || (baseurl == NULL)) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if(sqlite3_open(filename, &db)) {
		fprintf(stderr, "ERROR: Could not open '%s': '%s'\n", filename,
			sqlite3_errmsg(db));
	}

	addtable(baseurl, desc, db);
	sqlite3_close(db);

	return EXIT_SUCCESS;
}
