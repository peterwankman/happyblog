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

#define MAXBUF 512

#define TYPE_NONE	0
#define TYPE_TIME	1
#define TYPE_HASH	2
#define TYPE_MON	3
#define	TYPE_CSS	4

typedef struct {
	char *title;
	char *head;
	char *tail;

	sqlite3 *db;
} config_t;

typedef struct {
	time_t start, end;
	unsigned int hash;
	int type;
} postmask_t;

static void head(char *title, char *head, char *css, int setcss) {
	if(setcss && css) {
		printf("Set-Cookie: css=");
		if(setcss == 2)
			printf(" ; expires=Sat, 1-Jan-2000 00:00:00 GMT\r\n");
		else
			printf("%s\r\n", css);
	}

	printf("Content-Type: text/html;charset=UTF-8\r\n\r\n");
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML "
		"4.0 Transitional//EN\">\n");

	if(css && css[0])
		printf("<link rel=stylesheet type=\"text/css\" href=\"%s\">\n", css);

#ifdef RSS
	printf("<link rel=\"alternate\" type=\"application/rss+xml\" "
		"title=\"RSS-Feed\" href=\"blag-rss.cgi\">\n");
#endif

	printf("\n<title>%s</title>", title);
	printf("<h2><a href=\"blag.cgi\" style=\"text-decoration:none;"
		"color:black\">%s</a></h2>\n", title);
	printf("<b>%s</b>\n\n", head);
}

static void tail(char *tail) {
	printf("<div align=right>%s</div>\n", tail);
}

static void delnewline(char *in) {
	int i;
	for(i = 0; i < strlen(in); i++)
		if(in[i] == '\n')
			in[i] = '\0';
}

static int isolder(struct tm *curr, struct tm *last) {
	if(!last)
		return 1;
	if(curr->tm_year < last->tm_year)
		return 1;
	if(curr->tm_yday < last->tm_yday)
		return 1;
	return 0;
}

static void printupdates(unsigned int hash, sqlite3 *db) {
	sqlite3_stmt *statement;
	char *buf;

	sqlite3_prepare(db, "SELECT entry FROM updates WHERE hash "
		"= :hsh ORDER BY time;", MAXBUF, &statement, NULL);
	sqlite3_bind_int(statement, 1, hash);

	while(sqlite3_step(statement) == SQLITE_ROW) {
		buf = sqlite3_column_text(statement, 0);
		printf(" <p><b>Update</b>: %s\n", buf);
	}

	sqlite3_finalize(statement);
}

static int printposts(postmask_t mask, sqlite3 *db) {
	sqlite3_stmt *statement;
	int newblock = 1, hash, count = 0;
	time_t posttime;
	struct tm *currtime, lasttime;
	char *buf, timebuf[MAXBUF];

	if(mask.type == TYPE_TIME) {
		sqlite3_prepare(db, "SELECT time, hash, entry FROM entries WHERE time "
			">= :sta AND time < :end ORDER BY time DESC;", 
			MAXBUF, &statement, NULL);
		sqlite3_bind_int(statement, 1, mask.start);
		sqlite3_bind_int(statement, 2, mask.end);
	} else if(mask.type == TYPE_HASH) {
		sqlite3_prepare(db, "SELECT time, hash, entry FROM entries WHERE hash "
			"= :hsh ORDER BY time DESC;", MAXBUF, &statement, NULL);
		sqlite3_bind_int(statement, 1, mask.hash);
	} else {
		printf("NIL: %d\n", mask.type);
	}

	newblock = 1;
	lasttime.tm_year = 9999;
	while(sqlite3_step(statement) == SQLITE_ROW) {
		posttime = sqlite3_column_int(statement, 0);
		hash = sqlite3_column_int(statement, 1);
		buf = sqlite3_column_text(statement, 2);

		currtime = localtime(&posttime);
		if(isolder(currtime, &lasttime)) {
			strftime(timebuf, MAXBUF, "%a %b %d %Y", currtime);
			if(!newblock) {
				printf("</ul>\n\n");
			} else {
				newblock = 0;
			}
			printf("<p><h3>%s</h3>\n<ul>\n", timebuf);
			lasttime = *currtime;
		}
		printf("<li><a href=\"?ts=%08x\">[l]</a> %s\n", hash, buf);
		printupdates(hash, db);
		count++;
	}
	sqlite3_finalize(statement);

	if(count)
		printf("</ul>\n\n");
	return count;
}

static config_t readconfig(char *conffile) {
	config_t out;
	FILE *fp;
	char dbfile[MAXBUF], *buf;
	int buflen;
	sqlite3_stmt *statement;

	out.title = out.head = out.tail = NULL;
	out.db = NULL;

	if((fp = fopen(conffile, "r")) == NULL) {
		printf("ERROR: '%s' not found.\n", conffile);
		return out;
	}

	fgets(dbfile, MAXBUF, fp);
	fclose(fp);
	delnewline(dbfile);

	if(sqlite3_open(dbfile, &out.db)) {
		printf("ERROR: Could not open database '%s': %s\n", dbfile,
			sqlite3_errmsg(out.db));
		out.db = NULL;
		return out;
	}

	sqlite3_prepare(out.db, "SELECT title, head, tail FROM config;",
		MAXBUF, &statement, NULL);

	if(sqlite3_step(statement) == SQLITE_ROW) {
		buflen = sqlite3_column_bytes(statement, 0) + 1;
		buf = sqlite3_column_text(statement, 0);

		if((out.title = malloc(buflen)) == NULL) {
			printf("ERROR: malloc(title) failed.\n");
			out.db = NULL;
			return out;
		}
		strncpy(out.title, buf, buflen);

		buflen = sqlite3_column_bytes(statement, 1) + 1;
		buf = sqlite3_column_text(statement, 1);

		if((out.head = malloc(buflen)) == NULL) {
			printf("ERROR: malloc(head) failed.\n");
			free(out.title);
			out.title = NULL;
			out.db = NULL;
			return out;
		}
		strncpy(out.head, buf, buflen);

		buflen = sqlite3_column_bytes(statement, 2) + 1;
		buf = sqlite3_column_text(statement, 2);

		if((out.tail = malloc(buflen)) == NULL) {
			printf("ERROR: malloc(tail) failed.\n");
			free(out.head);
			out.head = NULL;
			free(out.title);
			out.title = NULL;
			out.db = NULL;
			return out;
		}
		strncpy(out.tail, buf, buflen);
	}

	sqlite3_finalize(statement);
	return out;
}

static int getquerytype(char *query) {
	if(query == NULL)
		return TYPE_NONE;
	else if(!strncmp(query, "ts=", 3))
		return TYPE_HASH;
	else if(!strncmp(query, "mon=", 4))
		return TYPE_MON;
	else if(!strncmp(query, "css=", 4))
		return TYPE_CSS;

	return TYPE_NONE;
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

static void querytotime(char *query, int *year, int *mon, 
						time_t *start, time_t *end) {
	int datestr;
	struct tm *buf;
	time_t now;

	*start = *end = 0;

	if(strlen(query) < 9)
		return;

	datestr = atoi(query + 4);
	*mon = datestr % 100;
	*year = datestr / 100;

	time(&now);
	buf = localtime(&now);

	buf->tm_sec = 0;
	buf->tm_min = 0;
	buf->tm_hour = 0;
	buf->tm_mday = 1;
	buf->tm_mon = *mon - 1;
	buf->tm_year = *year - 1900;

	*start = mktime(buf);

	buf->tm_mon++;
	if(buf->tm_mon > 11) {
		buf->tm_mon = 0;
		buf->tm_year++;
	}

	*end = mktime(buf);
}

static void querytohash(char *query, unsigned int *hash) {
	*hash = 0;

	if(strlen(query) < 11)
		return;

	*hash = hex2int(query + 3);
}

static void dispatch(char *query, int type, sqlite3 *db) {
	postmask_t mask;
	int count, mon, pmon, year, pyear;
	time_t now = time(NULL);
	struct tm *local;

	switch(type) {
		case TYPE_MON:
			querytotime(query, &year, &mon, &mask.start, &mask.end);
			mask.type = TYPE_TIME;
			break;
		case TYPE_HASH:
			querytohash(query, &mask.hash);
			mask.type = TYPE_HASH;
			break;
		default:
			mask.end = now;
			mask.start = mask.end - 345600;
			mask.type = TYPE_TIME;
			break;
	}

	count = printposts(mask, db);
					
	if(!count) {
		printf("<p>No entries found.\n\n");
	}
	
	printf("<p><div align=center>");

	local = localtime(&now);

	if(type == TYPE_MON) {
		pyear = year;
		pmon = mon - 1;
		if(pmon == 0) {
			pmon = 12;
			pyear--;
		}
		printf("<a href=\"blag.cgi?mon=%04d%02d\">fr&uuml;her</a> -- ",
			pyear, pmon);

		printf("<a href=\"blag.cgi?mon=%04d%02d\">aktuell</a> -- ",
			local->tm_year + 1900, local->tm_mon + 1);

		pyear = year;
		pmon = mon + 1;
		if(pmon > 12) {
			pmon = 1;
			pyear++;
		}
		printf("<a href=\"blag.cgi?mon=%04d%02d\">sp&auml;ter</a>", 
			pyear, pmon);
	} else {
		printf("<a href=\"blag.cgi?mon=%04d%02d\">ganzer Monat</a>",
			local->tm_year + 1900, local->tm_mon + 1);
	}

	printf("</div>\n");
}

int main(void) {
	config_t config;
	char *query, *cookie, *css = NULL, *buf;
	int query_type, setcss = 0;

	config = readconfig("/etc/blag.conf");

	if(config.db == NULL)
		return EXIT_FAILURE;

	cookie = getenv("HTTP_COOKIE");
	query = getenv("QUERY_STRING");
	query_type = getquerytype(query);

	if(query_type == TYPE_CSS) {
		css = query + 4;
		if(css[0] == '\0')
			setcss = 2; /* del cookie */
		else
			setcss = 1;
	} else if((cookie = strstr(cookie, "css")) != NULL) {
		css = cookie + 4;
		buf = strchr(css, ';');
		if(buf)
			buf[0] = '\0';
	}

	head(config.title, config.head, css, setcss);
	dispatch(query, query_type, config.db);
	tail(config.tail);

	free(config.title);
	free(config.head);
	free(config.tail);

	sqlite3_close(config.db);
	return EXIT_SUCCESS;
}
