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

#define MAXBUF	512

typedef struct {
	char *title;
	char *desc;
	char *baseurl;

	sqlite3 *db;
} config_t;

void delnewline(char *in) {
	int i;
	for(i = 0; i < strlen(in); i++)
		if(in[i] == '\n')
			in[i] = '\0';
}

static void head(char *title, char *baseurl, char *desc) {
	printf("Content-Type: text/xml;charset=utf-8\r\n\r\n");

	printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
	printf("<rss version=\"2.0\">\n\n");

	printf("<channel>\n");
	printf("<title>%s</title>\n", title);
	printf("<link>%s</link>\n", baseurl);
	printf("<description>%s</description>\n", desc);
	printf("<language>de</language>\n\n");
}

static void tail(void) {
	printf("</channel>\n");
	printf("</rss>\n");
}

static config_t readconfig(char *filename) {
	config_t out;
	FILE *fp;
	char dbfile[512], *buf;
	int buflen;
	sqlite3_stmt *statement;

	out.title = out.desc = out.baseurl = NULL;
	out.db = NULL;

	if((fp = fopen(filename, "r")) == NULL) {
		printf("ERROR: '%s' not found.\n", filename);
		return out;
	}

	fgets(dbfile, 512, fp);
	fclose(fp);
	delnewline(dbfile);

	if(sqlite3_open(dbfile, &out.db)) {
		printf("ERROR: Could not open database '%s': %s\n", dbfile,
			sqlite3_errmsg(out.db));
		out.db = NULL;
		return out;
	}

	sqlite3_prepare(out.db, "SELECT title FROM config;",
		MAXBUF, &statement, NULL);

	if(sqlite3_step(statement) == SQLITE_ROW) {
		buflen = sqlite3_column_bytes(statement, 0) + 1;
		buf = (char*)sqlite3_column_text(statement, 0);

		if((out.title = malloc(buflen)) == NULL) {
			printf("ERROR: malloc(title) failed.\n");
			out.db = NULL;
			return out;
		}
		strncpy(out.title, buf, buflen);
	}
	sqlite3_finalize(statement);

	sqlite3_prepare(out.db, "SELECT desc, baseurl FROM rssconfig;",
		MAXBUF, &statement, NULL);

	if(sqlite3_step(statement) == SQLITE_ROW) {
		buflen = sqlite3_column_bytes(statement, 0) + 1;
		buf = (char*)sqlite3_column_text(statement, 0);

		if((out.desc = malloc(buflen)) == NULL) {
			printf("ERROR: malloc(desc) failed.\n");
			free(out.title);
			out.title = NULL;
			out.db = NULL;
			return out;
		}
		strncpy(out.desc, buf, buflen);

		buflen = sqlite3_column_bytes(statement, 1) + 1;
		buf = (char*)sqlite3_column_text(statement, 1);

		if((out.baseurl = malloc(buflen)) == NULL) {
			printf("ERROR: malloc(baseurl) failed.\n");
			free(out.title);
			out.title = NULL;
			free(out.desc);
			out.desc = NULL;
			out.db = NULL;
			return out;
		}
		strncpy(out.baseurl, buf, buflen);
	}

	sqlite3_finalize(statement);
	return out;
}

static char *strip(char *in, unsigned int hash) {
	char *out;
	size_t len = strlen(in);
	int i, j = 0, words = 0, skip = 0;

	if((out = malloc(len + 4)) == NULL)
		return in;

	for(i = 0; i < len; i++) {
		if(in[i] == '<')
			skip += 1;
		if(!skip) {
			if(in[i] == ' ')
				words++;
			out[j++] = in[i];
		}
		if(in[i] == '>')
			skip -= 1;
		if(words == 6) {
			j--;
			break;
		}
	}
	out[j] = '\0';
	if(skip || (strlen(out) == 0)) {	/* Unbalanced <> */
		free(out);
		if((out = malloc(14)) == NULL)
			return in;
		sprintf(out, "Post %08x", hash);
	} else {
		if(i < len - 1)
			strcat(out, "...");
	}
	return out;
}

static void printposts(sqlite3 *db, char *baseurl, int num) {
	sqlite3_stmt *statement;
	int count = 0, hash;
	char *buf, *title;

	sqlite3_prepare(db, "SELECT hash, entry FROM entries "
		"ORDER BY time DESC;", MAXBUF, &statement, NULL);

	while((sqlite3_step(statement) == SQLITE_ROW) && (count < num)) {
		hash = sqlite3_column_int(statement, 0);
		buf = (char*)sqlite3_column_text(statement, 1);
		title = strip(buf, hash);

		printf("<item>\n");
		printf("<title>%s</title>\n", title);
		printf("<link>%s?ts=%08x</link>\n", baseurl, hash);
		printf("<guid>%s?ts=%08x</guid>\n", baseurl, hash);
		printf("<description><![CDATA[%s]]></description>\n", buf);
		printf("</item>\n\n");

		if(title && (title != buf))
			free(title);
		count++;
	}
	sqlite3_finalize(statement);
}

int main(void) {
	config_t config;

	config = readconfig("/etc/blag.conf");

	if(config.db == NULL)
		return EXIT_FAILURE;

	head(config.title, config.baseurl, config.desc);
	printposts(config.db, config.baseurl, 16);
	tail();

	sqlite3_close(config.db);
	free(config.title);
	free(config.baseurl);
	free(config.desc);

	return EXIT_SUCCESS;
}
