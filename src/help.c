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

#include <string.h>

#define H2I_OK		0
#define H2I_NHEX	1
#define H2I_SHOR	2

void delnewline(char *in) {
	int i;
	for(i = 0; i < strlen(in); i++)
		if(in[i] == '\n')
			in[i] = '\0';
}

int hextoint(char *in, unsigned int *out) {
	char c;
	int i;

	if(strlen(in) < 8)
		return H2I_SHOR;
	*out = 0;
	for(i = 0; i < 8; i++) {
		*out <<= 4;
		c = in[i];
		if(c >= '0' && c <= '9')
			*out += c - 48;
		else if((c >= 'a') && (c <= 'f'))
			*out += c - 87;
		else if((c >= 'A') && (c <= 'F'))
			*out += c - 55;
		else
			return H2I_NHEX;
	}
	return H2I_OK;
}
