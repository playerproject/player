/* Getopt for GNU.
   NOTE: getopt is part of the C library, so if you don't know what
   "Keep this file name-space clean" means, talk to drepper@gnu.org
   before changing it!
   Copyright (C) 1987-1996,1998-2004,2008,2009 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

// Taken from glibc and modified for stand-alone compilation.
// NOTE: This does not implement the '+' and '-' modes required by POSIX, the
// two-colons GNU extension nor the -W GNU extension.

#include <stdio.h>
#include <string.h>

#include "replace/replace.h"

char *optarg = NULL;
int optind = 1, opterr = 1, optopt = 0;

int getopt (int argc, char * const argv[], const char *optstring)
{
	char c, *cptr = NULL;
	int index = 1;

	// Filter out: exhausted args list, NULL args (shouldn't happen?), args not
	// beginning with -, and args that are just -
	if (optind >= argc || argv[optind][0] == '\0' || argv[optind][0] != '-' ||
		(argv[optind][0] == '-' && argv[optind][1] == '\0'))
	{
		return -1;
	}

	// -- marks the end of the command line arguments list
	if (strncmp (argv[optind], "--", strlen (argv[optind])) == 0)
	{
		optind++;
		return -1;
	}

	// Get the option character
	optopt = c = argv[optind][index];
	// Check if it's a legal option
	if ((cptr = strchr (optstring, c)) == NULL)
	{
		if (opterr != 0)
			fprintf (stderr, "%s: Illegal option: %c", argv[0], c);
		// Check if this argument string has been exhausted, if so reset to beginning of the next one
		if (argv[optind][index] == '\0')
		{
			optind++;
			index = 1;
		}
		else
			index++;
		return '?';
	}

	// Does this option require an argument?
	if (*(cptr + 1) == ':')
	{
		// The argument might be the remainder of this argv string or the whole of the next one
		if (argv[optind][index + 1] != '\0')
		{
			optarg = &(argv[optind][index + 1]);
			optind++;
			index = 1;
		}
		else if (optind + 1 < argc)
		{
			optarg = argv[optind + 1];
			optind += 2;
			index = 1;
		}
		else
		{
			if (optstring[0] == ':')
			{
				if (opterr != 0)
					fprintf (stderr, "%s: Missing argument for option %c", argv[0], c);
				return ':';
			}
			else
				return '?';
		}
	}
	else
	{
		// Check if this argument string has been exhausted, if so reset to beginning of the next one
		if (argv[optind][index] == '\0')
		{
			optind++;
			index = 1;
		}
		else
			index++;
	}

	return c;
}
