/* 
 * sha1.h -- sha1.c header
 * 
 * Copyright (C) 2010  Martin Wolters et al.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to 
 * the Free Software Foundation, Inc.
 * 51 Franklin Street, Fifth Floor
 * Boston, MA  02110-1301, USA
 * 
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
