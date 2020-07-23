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

#include	<getopt.h>

#include	<compress_lib.h>
#include	<types_lib.h>


#define FILE_EXTENSION			".flk"
#define FILE_EXTENSION_LEN	4

struct flickInfo
{
	struct compressInfo	 compress_info;

	char	* input_name;
	FILE	* input;
	char 	* output_name;
	FILE	* output;

	bool					decrunch;
	unsigned long	block_size;
};


static void
usage (char * progname)
{
	printf("\nUsage: %s [flags and then input]\n\
	-h  help\n\
	-d  decompress\n\
	-o  output file\n\
	-1 .. -9 (block size from 100k to 900k)\n\
	\n", progname);
}

static bool
parseArgs (struct flickInfo * info, int argc, char ** argv)
{
	int             op;

	opterr = 0;

	while ((op = getopt (argc, argv, "hdo:123456789")) != EOF)
	{
		switch (op)
		{
		case 'h':
			usage (argv[0]);
			return false;

		case 'd':
			info->decrunch = true;
			break;

		case 'o':
			free (info->output_name);
			info->output_name = malloc ((strlen (optarg) + 1) * sizeof *info->output);
			if ( NULL == info->output_name )
			{
				fputs("*** out of memory\n", stderr);
				return false;
			}
			strcpy (info->output_name, optarg);
			break;

		/* change block size */
		case '1':
			info->block_size = 102400;
			break;
		case '2':
			info->block_size = 204800;
			break;
		case '3':
			info->block_size = 307200;
			break;
		case '4':
			info->block_size = 409600;
			break;
		case '5':
			info->block_size = 512000;
			break;
		case '6':
			info->block_size = 614400;
			break;
		case '7':
			info->block_size = 716800;
			break;
		case '8':
			info->block_size = 819200;
			break;
		case '9':
			info->block_size = 921600;
			break;

		default:
			fputs("*** unknown argument\n", stderr);
			break;
		}
	}

	if ( optind >= argc )
	{
		fputs("*** specify an input file\n", stderr);
		return false;
	}

	return true;
}



static void
errorHook (char * error, void * callback_data)
{
	printf ("*** %s\n", error);
}

static bool
compressHook (unsigned char * output, unsigned long output_size, void * callback_data)
{
	struct flickInfo	* info = (struct flickInfo *)callback_data;

	/* write out block size in big endian format */
	fputc ((output_size >> 24) & 0xff, info->output);
	fputc ((output_size >> 16) & 0xff, info->output);
	fputc ((output_size >> 8) & 0xff, info->output);
	fputc (output_size & 0xff, info->output);

	/* write output data */
	if ( fwrite (output, sizeof *output, output_size, info->output) != output_size )
		return false;

	return true;
}

static bool
decompressStartHook (FILE * f, unsigned long * block_size, void * callback_data)
{
	*block_size = fgetc (f) << 24;
	*block_size |= fgetc (f) << 16;
	*block_size |= fgetc (f) << 8;
	*block_size |= fgetc (f);

	return true;
}

static bool
decompressEndHook (unsigned char * output, unsigned long output_size, void * callback_data)
{
	struct flickInfo	* info = (struct flickInfo *)callback_data;

	/* write output data */
	if ( fwrite (output, sizeof *output, output_size, info->output) != output_size )
		return false;

	return true;
}


static void
initFlickInfo (struct flickInfo * info)
{
	memset (info, 0, sizeof *info);

	info->compress_info.errorHook = errorHook;
	info->compress_info.errorHook_data = info;

	info->compress_info.compressHook = compressHook;
	info->compress_info.compressHook_data = info;

	info->compress_info.decompressStartHook = decompressStartHook;
	info->compress_info.decompressEndHook = decompressEndHook;
	info->compress_info.decompressHook_data = info;

	info->decrunch = false;
	info->block_size = 921600;
}

static void
cleanFlickInfo (struct flickInfo * info)
{
	if ( info->input )
		fclose (info->input);
	info->input = NULL;

	if ( info->output )
		fclose (info->output);
	info->output = NULL;

	free (info->output_name);
	info->output_name = NULL;
}

int
main (int argc, char ** argv)
{
	struct flickInfo	info;

	initFlickInfo (&info);

	/* check args */
	if ( false == parseArgs (&info, argc, argv) )
		return EXIT_FAILURE;

	/* attempt to open file */
	info.input_name = *(argv+optind);
	info.input = fopen (info.input_name, "rb");
	if ( NULL == info.input )
	{
		fputs ("*** input file doesn't exist\n", stderr);
		return EXIT_FAILURE;
	}

	/*
	 * if no output name has been specified on the command line
	 * prepare output filename depending on whether the file is to
	 * be crunched or decrunched
	 */
	if ( NULL == info.output_name )
	{
		if ( false == info.decrunch )
		{
			size_t	l;

			l = strlen (info.input_name);

			if ( l > FILE_EXTENSION_LEN && 0 == strncmp (info.input_name + l - FILE_EXTENSION_LEN, FILE_EXTENSION, FILE_EXTENSION_LEN) )
			{
				fputs ("*** input file seems to have already been crunched\n", stderr);
				cleanFlickInfo (&info);
				return EXIT_FAILURE;
			}

			info.output_name = malloc ( (l + FILE_EXTENSION_LEN + 1) * sizeof *info.output_name );
			if ( NULL == info.output_name )
			{
				fputs ("*** out of memory", stderr);
				cleanFlickInfo (&info);
				return EXIT_FAILURE;
			}
			sprintf(info.output_name, "%s%s", info.input_name, FILE_EXTENSION);
		}
		else
		{
			size_t	l;

			l = strlen (info.input_name);

			if ( l <= FILE_EXTENSION_LEN ||	 0 != strncmp (info.input_name+l-FILE_EXTENSION_LEN, FILE_EXTENSION, l) )
			{
				fputs ("*** not a valid flick file (bad file extension)\n", stderr);
				cleanFlickInfo (&info);
				return EXIT_FAILURE;
			}

			info.output_name = malloc ( (l + 1) * sizeof *info.output_name );
			if ( NULL == info.output_name )
			{
				fputs ("*** out of memory", stderr);
				cleanFlickInfo (&info);
				return EXIT_FAILURE;
			}

			strncpy (info.output_name, info.input_name, l-FILE_EXTENSION_LEN);
			info.output_name[l-FILE_EXTENSION_LEN] = '\0';
		}
	}

	/* open output file */
	info.output = fopen (info.output_name, "wb");
	if ( NULL == info.output )
	{
		fputs ("*** error opening output file", stderr);
		cleanFlickInfo (&info);
		return EXIT_FAILURE;
	}

	/* decrunch... */
	if ( true == info.decrunch )
	{
		if ( COMP_RET_OKAY != comp_decompressFile (&info.compress_info, info.input, true, 0) )
		{
			cleanFlickInfo (&info);
			return EXIT_FAILURE;
		}
	}
	else
	/* ...or crunch */
	if ( COMP_RET_OKAY != comp_compressFile (&info.compress_info, info.input, info.block_size) )
	{
		cleanFlickInfo (&info);
		return EXIT_FAILURE;
	}

	cleanFlickInfo (&info);

	return EXIT_SUCCESS;
}
