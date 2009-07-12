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
