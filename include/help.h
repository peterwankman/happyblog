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

#ifndef HELP_H
#define HELP_H

#define H2I_OK		0
#define H2I_NHEX	1
#define H2I_SHOR	2

void delnewline(char *in);
int hextoint(char *in, unsigned int *out);

#endif
