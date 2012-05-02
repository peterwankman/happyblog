/* 
 * sha1.c -- The SHA-1 message digest algorithm
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

/* From the description of SHA-1 in Wikipedia */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "sha1.h"

static uchar *statetochar(hash_t in) {
	uchar *out = malloc(20);
	if(out == NULL) return NULL;
	
	out[0] = (in.h0 >> 24) & 0xff;
	out[1] = (in.h0 >> 16) & 0xff;
	out[2] = (in.h0 >> 8) & 0xff;
	out[3] = in.h0 & 0xff;
	out[4] = (in.h1 >> 24) & 0xff;
	out[5] = (in.h1 >> 16) & 0xff;
	out[6] = (in.h1 >> 8) & 0xff;
	out[7] = in.h1 & 0xff;
	out[8] = (in.h2 >> 24) & 0xff;
	out[9] = (in.h2 >> 16) & 0xff;
	out[10] = (in.h2 >> 8) & 0xff;
	out[11] = in.h2 & 0xff;
	out[12] = (in.h3 >> 24) & 0xff;
	out[13] = (in.h3 >> 16) & 0xff;
	out[14] = (in.h3 >> 8) & 0xff;
	out[15] = in.h3 & 0xff;
	out[16] = (in.h4 >> 24) & 0xff;
	out[17] = (in.h4 >> 16) & 0xff;
	out[18] = (in.h4 >> 8) & 0xff;
	out[19] = in.h4 & 0xff;

	return out;
}

static hash_t sha1init(void) {
	hash_t out;

	out.h0 = 0x67452301;
	out.h1 = 0xefcdab89;
	out.h2 = 0x98badcfe;
	out.h3 = 0x10325476;
	out.h4 = 0xc3d2e1f0;

	out.blocksize = 64;
	out.bytesize = 20; // 20 Bytes to the hash.

	out.string = NULL;

	return out;
}

static int newsize(int insize) {
	int size;

	size = insize + 1;
	size += (size % 64 > 56)? 64: 0;
	size += 64 - size % 64;

	return size;
}

static uchar *preprocess(char *in, int insize, int size) {
	uchar *out;
	int i;

	if((out = malloc(size)) == NULL)
		return NULL;

	memcpy(out, in, insize);
	out[insize] = 0x80;
	
	/* We assume size to be 32bit, so we
	   write the first 32 bits as zeroes. */

	for(i = insize + 1; i < size - 4; i++)
		out[i] = 0;
	for(i = 0; i < 4; i++)
		out[size - 4 + i] = ((insize * 8) >> ((3 - i) * 8)) & 0xff;

	return out;
}

static hash_t digest(uchar *in, int size) {
	hash_t state;
	int i, j;
	uint32_t W[80], a, b, c, d, e, f, k, temp;
	
	state = sha1init();

	/* Process 512 blocks each. */
	for(i = 0; i < size / 64; i++) {
		/* Break each block into 16 32bit words */
		for(j = 0; j < 16; j++) {
			W[j] = 0;
			for(k = 0; k < 4; k++) {
				W[j] <<= 8;
				W[j] |= in[i * 64 + j * 4 + k];
			}
		}

		for(j = 16; j < 80; j++)
			W[j] = rotate((W[j - 3] ^ W[j - 8] ^ W[j - 14] ^ W[j - 16]), 1);
		
		a = state.h0;
		b = state.h1;
		c = state.h2;
		d = state.h3;
		e = state.h4;

		for(j = 0; j < 80; j++) {
			if(j < 20) {
				f = (b & c) | ((~b) & d);
				k = 0x5a827999;
			} else if(j < 40) {
				f = b ^ c ^ d;
				k = 0x6ed9eba1;
			} else if(j < 60) {
				f = (b & c) | (b & d) | (c & d);
				k = 0x8f1bbcdc;
			} else {
				f = b ^ c ^ d;
				k = 0xca62c1d6;
			}

			temp = rotate(a, 5) + f + e + k + W[j];
			e = d;
			d = c;
			c = rotate(b, 30);
			b = a;
			a = temp;
		}
		state.h0 += a;
		state.h1 += b;
		state.h2 += c;
		state.h3 += d;
		state.h4 += e;
	}

	state.string = statetochar(state);
	return state;
}

hash_t sha1(char *in, int insize) {
	uchar *prep;
	hash_t out;
	int size = newsize(insize);

	prep = preprocess(in, insize, size);
	out = digest(prep, size);
	free(prep);

	return out;
}
