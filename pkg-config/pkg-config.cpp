/* -*- mode: C; c-file-style: "gnu" -*- */
/* pkgconfig.cpp Generic hash table utility (internal to D-Bus implementation)
 * 
 * Copyright (C) 2002  Red Hat, Inc.
 *
 * Licensed under the Academic Free License version 2.1
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <stdio.h>
#include <string.h>

// extend structure to support more compilers like gcc, msvc
char *dbus_cflags = "-IC:\\Programme\\dbus\\include";
char *dbus_lflags = "-LC:\\Programme\\dbus\\lib -ldbus-1";
char *libxml_cflags = "-IC:\\Programme\\gnuwin32\\include";
char *libxml_lflags = "-LC:\\Programme\\gnuwin32\\lib -lxml";

int verbose = 0; 

#define CFLAGS 0
#define LFLAGS 1
#define UNDEFINED -1

int main (int argc, char **argv)
{
	int index = UNDEFINED;
	char *mode = argv[1];
	char *module = argv[2];
	char *s;

	// detect required compiler gcc, msvc

	if (argc > 2) {
		if (verbose) 
			fprintf(stderr,"mode: %s module: %s",mode,module);

		if ( !strcmp(mode,"--cflags") )
			index = CFLAGS;
		else if ( !strcmp(mode,"--lflags") )
			index = LFLAGS;
		else {
			if (verbose) 
				printf("mode: %s not found ",mode);
			index = UNDEFINED;
			return 1;
		}

		if ( !strcmp(module,"dbus-1") )
			switch (index) {
				case CFLAGS: s = dbus_cflags;			break;
				case LFLAGS: s = dbus_lflags;			break;			
				default: s = "not found--";
			}			
			printf("%s",s);
			return 0;
	}
	if (verbose) 
		fprintf(stderr,"not found mode: %d module: %s",index,module );

	return 1;
}
