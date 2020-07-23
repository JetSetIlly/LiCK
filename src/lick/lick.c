// This file is part of the Lick project
//
// Lick is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Lick is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Lick.  If not, see <https://www.gnu.org/licenses/>.

#include	<stdlib.h>
#include	<stdio.h>
#include	<string.h>

#include	<llist_lib.h>

#include	"version.h"

#include	"locale.h"
#include	"add.h"
#include	"lick.h"


enum HANDLE_ARGS_STATES {
	ARGS_OKAY = 0,
	ARGS_UNKNOWN_COMMAND,
	ARGS_UNKNOWN_OPTION,
	ARGS_UNKNOWN_CRUNCH_MODE,
	ARGS_ARCHIVE_NOT_SPECIFIED,
	ARGS_OUT_OF_MEM
};
typedef char  HANDLE_ARGS_STATE;


static HANDLE_ARGS_STATE
handleArgs (struct lickOptions *options, int argc, char **argv)
{
	int i;

	options->action = LA_USAGE;

	if (argc == 1)
		return ARGS_OKAY;

	// decide on command
	if (argc >= 2) {
		if (strlen(argv[1]) != 1) 
			return ARGS_UNKNOWN_COMMAND;
		
		switch(argv[1][0]) {
			case 'a':
				options->action = LA_ADD;
				break;

			case 'v':
				options->action = LA_VIEW;
				break;

			case 'x':
				options->action = LA_EXTRACT;
				break;

			default:
				return ARGS_UNKNOWN_COMMAND;
		}
	}

	// process options
	for (i = 2; i < argc && strlen(argv[i]) > 1 && argv[i][0] == '-'; ++ i) {
		switch(argv[i][1]) {
			case 'e':
				if (strlen(argv[i]) != 3)
					return ARGS_UNKNOWN_CRUNCH_MODE;

				switch(argv[i][2]) {
					case '0':
						options->compress_mode = CT_NONE;
						break;

					default:
						return ARGS_UNKNOWN_CRUNCH_MODE;
				}
				break;

			case 't':
				if (strlen(argv[i]) != 2)
					return ARGS_UNKNOWN_OPTION;

				options->options |= EXT_OPT_TOUCH;
				break;

			case 'c':
				if (strlen(argv[i]) != 2)
					return ARGS_UNKNOWN_OPTION;

				options->options |= EXT_OPT_CLOBBER;
				break;

			default:
				return ARGS_UNKNOWN_OPTION;
		}
	}

	// get archive name
	if (i >= argc)
		return ARGS_ARCHIVE_NOT_SPECIFIED;
	options->archive = malloc(strlen(argv[i]) * sizeof *options->archive);
	strcpy(options->archive, argv[i]);
	++i;

	// add files to filelist
	for (; i < argc; ++ i) {
		ll_newTail(options->files, argv[i]);
	}

	return ARGS_OKAY;
}

static void
cleanupLickOptions (struct lickOptions *options)
{
	free (options->archive);
	ll_disposeList (options->files);
}

static bool
initLickOptions (struct lickOptions *options)
{
  options->archive = NULL;
  options->action = LA_NOTHING;
  options->compress_mode = CT_NONE;
  options->options = EXT_OPT_NONE;

	options->files = ll_newList (NULL);	/* we can use the default node data handling -- strings */
	if (NULL == options->files)
		return false;

	return true;
}

static void
printUsage (char *prog_name)
{
	printf ("\nLiCK %s Archive/Extract utility - %s\n", VERSION, PLATFORM_DESC);
	printf ("Copyright %s, %s\n\n", COPYRIGHT, AUTHOR);

	printf ("Usage: %s <command> [-options] <archive> [<file>...] [<dest_dir>]\n\n", prog_name);

	puts ("<command>:\na  Add file(s)\nv  View archive contents");
	puts ("x  Extract files from archive\n");

	puts ("<extract options>:\n-e  Set crunch mode\n-t  Touch files\n-c  Clobber files (without prompting)\n");
}

int
main (int argc, char **argv)
{
	struct lickOptions options;
	HANDLE_ARGS_STATE arg_state;
  bool ret_val;

	if (initLickOptions (&options) == false)
		return EXIT_FAILURE;

	arg_state = handleArgs (&options, argc, argv);
	switch(arg_state) {
		case ARGS_UNKNOWN_COMMAND:
			puts(LOC_BAD_COMMAND);
			break;
		case ARGS_UNKNOWN_OPTION:
			puts(LOC_BAD_OPTION);
			break;
		case ARGS_UNKNOWN_CRUNCH_MODE:
			puts(LOC_BAD_COMPRESS);
			break;
		case ARGS_ARCHIVE_NOT_SPECIFIED:
			puts(LOC_NO_ARCHIVE);
			break;
		case ARGS_OUT_OF_MEM:
			puts(LOC_OUT_OF_MEM);
			break;
	}
	if (arg_state != ARGS_OKAY) {
		cleanupLickOptions (&options);
		return EXIT_FAILURE;
	}

	/* decide on action to be taken */
  ret_val = true;
	switch (options.action)
	{
		case LA_USAGE:
			printUsage (*argv);
			break;

		case LA_ADD:
			ret_val = add(&options);
			break;

		case LA_VIEW:
			puts("TODO: viewing archive");
			break;

		case LA_EXTRACT:
			puts("TODO: extracting files");
			break;

		default:
			break;
	}

	cleanupLickOptions (&options);

	return true == ret_val ? EXIT_SUCCESS : EXIT_FAILURE;
}
