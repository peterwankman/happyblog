#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sqlite3.h>

#define MAXBUF 512

static void filltable(sqlite3 *db, char *title, char *head, char *tail) {
	sqlite3_stmt *statement;

	if(sqlite3_prepare(db, "INSERT INTO config (title, head, tail) "
		"VALUES (:ttl, :hed, :tai);", MAXBUF, &statement, NULL) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", sqlite3_errmsg(db));
		return;
	}

	sqlite3_bind_text(statement, 1, title, strlen(title), SQLITE_STATIC);
	sqlite3_bind_text(statement, 2, head, strlen(head), SQLITE_STATIC);
	sqlite3_bind_text(statement, 3, tail, strlen(tail), SQLITE_STATIC);

	sqlite3_step(statement);

	sqlite3_finalize(statement);
}

static void mktables(sqlite3 *db) {
	int ret;
	char *errmsg = NULL;

	if(sqlite3_exec(db, "CREATE TABLE entries (id INTEGER PRIMARY KEY, hash "
		"INTEGER, time INTEGER, entry TEXT);", NULL, 0, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return;
	}

	if(sqlite3_exec(db, "CREATE TABLE config (title TEXT, head TEXT, " 
		"tail TEXT);", NULL, 0, &errmsg) != SQLITE_OK) {
		fprintf(stderr, "SQLite error: %s\n", errmsg);
		sqlite3_free(errmsg);
		return;
	}
}

static int delifexists(char *filename) {
	FILE *fp;

	if((fp = fopen(filename, "r")) == NULL)
		return 1;
	fclose(fp);

	printf("Deleting '%s'...\n", filename);
	if(remove(filename) == -1) {
		fprintf(stderr, "ERROR: Could not delete '%s'.\n", filename);
		return 0;
	}

	return 1;
}

static void usage(char *argv) {
	printf("USAGE: %s [-h] [-t title] [-e head] [-f foot] [-o filename]\n", argv);
}

int main(int argc, char **argv) {
	char *filename = "blog.db";
	char *title = "Blog";
	char *head = "";
	char *tail = "";
	int opt;
	sqlite3 *db;

	while((opt = getopt(argc, argv, "he:f:o:t:")) != -1) {
		switch(opt) {
			case 'h': usage(argv[0]); return EXIT_SUCCESS;
			case 'e': head = optarg; break;
			case 'f': tail = optarg; break;
			case 'o': filename = optarg; break;
			case 't': title = optarg; break;
			default:
				usage(argv[0]);
				return EXIT_FAILURE;
		}
	}

	if(!delifexists(filename))
		return EXIT_FAILURE;

	if(sqlite3_open(filename, &db)) {
		fprintf(stderr, "ERROR: Could not open '%s': %s\n", filename,
			sqlite3_errmsg(db));
		return EXIT_FAILURE;
	}

	mktables(db);
	filltable(db, title, head, tail);
	sqlite3_close(db);

	return EXIT_SUCCESS;
}
