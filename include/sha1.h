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

#include <stdint.h>

#ifndef SHA1_H
#define SHA1_H

#define rotate(x, n) ((x << n) | (x >> (32 - n)))

typedef unsigned char uchar;

typedef struct {
	uint32_t h0, h1, h2, h3, h4;
	int blocksize;
	int bytesize;
	uchar *string;
} hash_t;

hash_t sha1(char *in, int size);

#endif 
