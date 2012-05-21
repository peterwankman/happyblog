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
#include "help.h"

#define MAXBUF 512
#define TYPE_NONE	0
#define TYPE_TIME	1
#define TYPE_HASH	2
#define TYPE_MON	3
#define	TYPE_CSS	4
#define TYPE_SEAR	5
#define COOKIE_NONE	5
#define COOKIE_SET	6
#define COOKIE_DEL	7
#define ERRHEAD		"Content-Type: text/plain;charset=us-ascii\n\n"

typedef struct {
	char *title, *head, *tail, *css, *query, *self;
	int cookie_cmd, query_type;
	sqlite3 *db;
} config_t;

typedef struct {
	time_t start, end;
	unsigned int hash;
	char *string;
	int type;
} postmask_t;

static void querytohash(char *query, unsigned int *hash) {
	*hash = 0;
	if(strlen(query) < 11)
		return;
	if(hextoint(query + 3, hash) != H2I_OK)
		return;
}

static void querytotime(char *query, int *year, int *mon,
						time_t *start, time_t *end) {
	int datestr, bmon, byear;
	struct tm *buf;
	time_t now;

	if(start)
		*start = 0;
	if(end)
		*end = 0;
	if(strlen(query) < 9)
		return;

	datestr = atoi(query + 4);
	bmon = datestr % 100;
	byear = datestr / 100;

	if(year)
		*year = byear;
	if(mon)
		*mon = bmon;

	if(start && end) {
		time(&now);
		buf = localtime(&now);
		buf->tm_sec = 0;
		buf->tm_min = 0;
		buf->tm_hour = 0;
		buf->tm_mday = 1;
		buf->tm_mon = bmon - 1;
		buf->tm_year = byear - 1900;

		*start = mktime(buf);
		buf->tm_mon++;
		if(buf->tm_mon > 11) {
			buf->tm_mon = 0;
			buf->tm_year++;
		}

		*end = mktime(buf);
	}
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
	else if(!strncmp(query, "search=", 7))
		return TYPE_SEAR;
	return TYPE_NONE;
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

static void head(config_t conf) {
	if((conf.cookie_cmd != COOKIE_NONE) && conf.css) {
		printf("Set-Cookie: css=");
		if(conf.cookie_cmd == COOKIE_DEL)
			printf(" ; expires=Sat, 1-Jan-2000 00:00:00 GMT\r\n");
		else
			printf("%s; expires=Sun, 17-Jan-2038 19:14:07 GMT\r\n", conf.css);
	}
	printf("Content-Type: text/html;charset=UTF-8\r\n\r\n");
	printf("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML "
		"4.0 Transitional//EN\">\n");
	if(conf.css && conf.css[0])
		printf("<link rel=stylesheet type=\"text/css\" href=\"%s\">\n", 
			conf.css);
	printf("<link rel=\"alternate\" type=\"application/rss+xml\" "
		"title=\"RSS-Feed\" href=\"blag-rss.cgi\">\n");
	printf("\n<title>%s</title>", conf.title);
	printf("<h2><a href=\"%s\" style=\"text-decoration:none;"
		"color:black\">%s</a></h2>\n", conf.self?conf.self:"/", conf.title);
	printf("<b>%s</b>\n\n", conf.head);
}

static void tail(config_t conf) {
	time_t now = time(NULL);
	struct tm *local;
	int mon, pmon, year, pyear;

	printf("<p><div align=center>");
	local = localtime(&now);
	if(conf.query_type == TYPE_MON) {
		querytotime(conf.query, &year, &mon, NULL, NULL);
		pyear = year;
		pmon = mon - 1;
		if(pmon == 0) {
			pmon = 12;
			pyear--;
		}
		printf("<a href=\"?mon=%04d%02d\">früher</a> -- ",
			pyear, pmon);

		printf("<a href=\"?mon=%04d%02d\">aktuell</a> -- ",
			local->tm_year + 1900, local->tm_mon + 1);

		pyear = year;
		pmon = mon + 1;
		if(pmon > 12) {
			pmon = 1;
			pyear++;
		}
		printf("<a href=\"?mon=%04d%02d\">später</a>",
			pyear, pmon);
	} else {
		printf("<a href=\"?mon=%04d%02d\">ganzer Monat</a>",
			local->tm_year + 1900, local->tm_mon + 1);
	}
	printf("</div>\n");
	printf("<div align=right>%s</div>\n", conf.tail);
}

static void printupdates(unsigned int hash, sqlite3 *db) {
	sqlite3_stmt *statement;
	char *buf;

	sqlite3_prepare(db, "SELECT entry FROM updates WHERE hash "
		"= :hsh ORDER BY time;", MAXBUF, &statement, NULL);
	sqlite3_bind_int(statement, 1, hash);

	while(sqlite3_step(statement) == SQLITE_ROW) {
		buf = (char*)sqlite3_column_text(statement, 0);
		printf(" <p><b>Update</b>: %s\n", buf);
	}

	sqlite3_finalize(statement);
}

static int printposts(postmask_t mask, sqlite3 *db) {
	sqlite3_stmt *statement;
	int newblock = 1, hash, count = 0;
	time_t posttime;
	struct tm *currtime, lasttime;
	char *buf, timebuf[MAXBUF], searchbuf[MAXBUF];

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
	} else if(mask.type == TYPE_SEAR) {
		sqlite3_prepare(db, "SELECT time, hash, entry FROM entries WHERE "
			"entry LIKE :src ORDER BY time DESC;", 
			MAXBUF, &statement, NULL);
		snprintf(searchbuf, MAXBUF, "%%%s%%", mask.string);
		sqlite3_bind_text(statement, 1, searchbuf, 
			strlen(searchbuf), SQLITE_STATIC);
	}

	newblock = 1;
	lasttime.tm_year = 9999;
	while(sqlite3_step(statement) == SQLITE_ROW) {
		posttime = sqlite3_column_int(statement, 0);
		hash = sqlite3_column_int(statement, 1);
		buf = (char*)sqlite3_column_text(statement, 2);

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

static void dispatch(config_t conf) {
	postmask_t mask;
	int count;
	time_t now = time(NULL);

	switch(conf.query_type) {
		case TYPE_MON:
			querytotime(conf.query, NULL, NULL, &mask.start, &mask.end);
			mask.type = TYPE_TIME;
			break;
		case TYPE_HASH:
			querytohash(conf.query, &mask.hash);
			mask.type = TYPE_HASH;
			break;
		case TYPE_SEAR:
			mask.type = TYPE_SEAR;
			mask.string = conf.query + 7;
			break;
		default:
			mask.end = now;
			mask.start = mask.end - 345600;
			mask.type = TYPE_TIME;
			break;
	}
	count = printposts(mask, conf.db);
	if(!count) {
		printf("<p>No entries found.\n\n");
	}
}

static int getcgivars(config_t *config) {
	char *cookie, *buf;

	cookie = getenv("HTTP_COOKIE");
	config->query = getenv("QUERY_STRING");
	config->self = getenv("SCRIPT_NAME");
	config->query_type = getquerytype(config->query);
	config->css = NULL;
	config->cookie_cmd = COOKIE_NONE;

	if(config->query_type == TYPE_CSS) {
		config->css = config->query + 4;
		if(config->css[0] == '\0')
			config->cookie_cmd = COOKIE_DEL;
		else
			config->cookie_cmd = COOKIE_SET;
	} else if(cookie && (cookie = strstr(cookie, "css=")) != NULL) {
		config->css = cookie + 4;
		buf = strchr(config->css, ';');
		if(buf)
			buf[0] = '\0';
	}

	return 1;
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
		printf(ERRHEAD "ERROR: File '%s' not found.\n", conffile);
		return out;
	}

	fgets(dbfile, MAXBUF, fp);
	fclose(fp);
	delnewline(dbfile);

	if(sqlite3_open(dbfile, &out.db)) {
		printf(ERRHEAD "ERROR: Could not open database '%s': %s\n", dbfile,
			sqlite3_errmsg(out.db));
		goto clean3;
	}

	sqlite3_prepare(out.db, "SELECT title, head, tail FROM config;",
		MAXBUF, &statement, NULL);
	if(sqlite3_step(statement) == SQLITE_ROW) {
		buflen = sqlite3_column_bytes(statement, 0) + 1;
		buf = (char*)sqlite3_column_text(statement, 0);
		if((out.title = malloc(buflen)) == NULL) {
			printf(ERRHEAD "ERROR: malloc(title) failed.\n");
			goto clean3;
		}
		strncpy(out.title, buf, buflen);

		buflen = sqlite3_column_bytes(statement, 1) + 1;
		buf = (char*)sqlite3_column_text(statement, 1);
		if((out.head = malloc(buflen)) == NULL) {
			printf(ERRHEAD "ERROR: malloc(head) failed.\n");
			goto clean2;
		}
		strncpy(out.head, buf, buflen);

		buflen = sqlite3_column_bytes(statement, 2) + 1;
		buf = (char*)sqlite3_column_text(statement, 2);
		if((out.tail = malloc(buflen)) == NULL) {
			printf(ERRHEAD "ERROR: malloc(tail) failed.\n");
			goto clean;
		}
		strncpy(out.tail, buf, buflen);
	}
	sqlite3_finalize(statement);

	if(!getcgivars(&out))
		goto clean;

	return out;
clean:
	free(out.head);
	out.head = NULL;
clean2:
	free(out.title);
	out.title = NULL;
clean3:
	out.db = NULL;
	return out;
}

int main(void) {
	config_t config;

	config = readconfig("/etc/blag.conf");
	if(config.db == NULL)
		return EXIT_FAILURE;

	head(config);
	dispatch(config);
	tail(config);

	free(config.title);
	free(config.head);
	free(config.tail);
	sqlite3_close(config.db);
	return EXIT_SUCCESS;
}
