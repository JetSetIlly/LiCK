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
#include	<errno.h>

#include	<types_lib.h>
#include	<bwt_lib.h>
#include	<huff_lib.h>
#include	<mtf_lib.h>
#include	<rle_lib.h>



/* {{{1 TEST INFO CONTROL AND FILE MANIPULATION */
 
#define COMPRESSFILENAME		"COMPRESS_TEST"
#define DECOMPRESSFILENAME	"DECOMPRESS_TEST"

struct testInfo
{
	unsigned char			*input;
	unsigned long	 		input_size;

	unsigned char			*output;
	unsigned long	 		output_size;
};


static bool
startTest (struct testInfo *ti, char * filename)
{
	FILE	*f;

	ti->input = NULL;
	ti->input_size = 0;
	ti->output = NULL;
	ti->output_size = 0;

	f = fopen (filename, "rb+");
	if ( NULL == f )
	{
		perror (filename);
		return false;
	}

	fseek (f, 0, SEEK_END);
	ti->input_size = ftell (f);
	rewind (f);

	ti->input = malloc(ti->input_size*sizeof(char));
	if ( NULL == ti->input )
	{
		puts("out of memory");
		fclose (f);
		return false;
	}

	if ( fread (ti->input, sizeof(char), ti->input_size, f) != ti->input_size )
	{
		puts("*** error reading file");
		free (ti->input);
		fclose (f);
		return false;
	}

	fclose (f);

	return true;
}

static bool
saveCompress (struct testInfo *ti)
{
	FILE	* outfc;

	if ( NULL == ti->output )
	{
		free (ti->input);
		ti->input = NULL;
		ti->input_size = 0;
		puts ("*** no output to save");
		return false;
	}

	outfc = fopen(COMPRESSFILENAME, "wb+");
	if ( NULL == outfc )
	{
		free (ti->input);
		ti->input = NULL;
		ti->input_size = 0;
		free (ti->output);
		ti->output = NULL;
		ti->output_size = 0;
		puts("*** couldn't open out file");
		return false;
	}
	else
	{
		if ( fwrite (ti->output, sizeof *ti->output, ti->output_size, outfc) != ti->output_size )
		{
			fclose (outfc);
			puts ("*** error writing to output file");
			return false;
		}
		fclose (outfc);
	}

	/* make output the next input */
	free (ti->input);
	ti->input = ti->output;
	ti->input_size = ti->output_size;
	ti->output = NULL;
	ti->output_size = 0;

	return true;
}

static bool
saveDecompress (struct testInfo * ti)
{
	FILE	* outf;

	if ( NULL == ti->output )
	{
		free (ti->input);
		ti->input = NULL;
		ti->input_size = 0;
		puts ("*** no output to save");
		return false;
	}

	outf = fopen(DECOMPRESSFILENAME, "wb+");
	if ( NULL == outf )
	{
		free (ti->input);
		ti->input = NULL;
		ti->input_size = 0;
		free (ti->output);
		ti->output = NULL;
		ti->output_size = 0;
		puts("*** couldn't open out file");
		return false;
	}
	else
	{
		if ( fwrite (ti->output, sizeof(char), ti->output_size, outf) != ti->output_size )
		{
			free (ti->input);
			ti->input = NULL;
			ti->input_size = 0;
			free (ti->output);
			ti->output = NULL;
			ti->output_size = 0;
			fclose (outf);
			puts ("*** error writing to output file");
			return false;
		}
		fclose (outf);
	}

	free (ti->input);
	ti->input = NULL;
	ti->input_size = 0;
	free (ti->output);
	ti->output = NULL;
	ti->output_size = 0;

	return true;
}
/* }}}1 */

/* {{{1 BWT */
static bool
testBWT (char * filename)
{
	struct testInfo	ti;
	int ret;


	if ( false == startTest (&ti, filename) )
		return false;

	ret = bwt_encode (ti.input, ti.input_size, &ti.output, &ti.output_size);
	if ( 1 == ret )
	{
		if ( false == saveCompress (&ti) )
			return false;

		ret = bwt_decode (ti.input, ti.input_size, &ti.output, &ti.output_size);
		if ( 1 == ret )
		{
			if ( false == saveDecompress (&ti) )
				return false;
		}
		else
		{
			puts("*** out of memory while decoding");
		}
	}
	else
	{
		puts("*** out of memory while encoding");
	}

	return true;
}
/* }}}1 */

/* {{{1 HUFFMAN ENCODER */
#define HUFF_PRE_PADDING	1;

static bool
testHuff (char * filename)
{
	struct testInfo	ti;
	int ret;

	unsigned long	pre_padding = HUFF_PRE_PADDING;
	unsigned long	i;

	if ( false == startTest (&ti, filename) )
		return false;

	ret = huff_encode (ti.input, ti.input_size, &ti.output, &ti.output_size, pre_padding);
	if ( HUFF_RET_SUCCESS == ret )
	{
		/*
		 * give pre_padding a value -- shuts valgrind up
		 */
		for ( i = 0; i < pre_padding; ++ i )
			ti.output[i] = 0;

		if ( false == saveCompress (&ti) )
			return false;

		ret = huff_decode (ti.input, ti.input_size, &ti.output, &ti.output_size, pre_padding);
		if ( HUFF_RET_SUCCESS == ret )
		{
			if ( false == saveDecompress (&ti) )
				return false;
		}
		else
		{
			if ( HUFF_RET_NOMEM == ret )
				puts("*** out of memory while decoding");
			else
			if ( HUFF_RET_MALFORMED == ret )
				puts("*** malformed data for huffman decoder");
			else
				puts("*** unexpected error");
		}
	}
	else
	{
		if ( HUFF_RET_NOMEM == ret )
			puts("*** out of memory while compressing");
		else
		if ( HUFF_RET_TOOBIG == ret )
			puts("*** output will be bigger than input");
		else
		if ( HUFF_RET_EMPTY_INPUT == ret )
			puts("*** empty input");
		else
			puts("*** unexpected error");
	}

	return true;
}

/* }}}1 */

/* {{{1 MTF */
#define MTF_TYPE	MTF1

static bool
testMTF (char * filename)
{
	struct testInfo	ti;
	int ret;


	if ( false == startTest (&ti, filename) )
		return false;

	ret = mtf_encode (ti.input, ti.input_size, MTF_TYPE);
	if ( 1 == ret )
	{
		/* mtf is inplace. swap output for input */
		ti.output = ti.input;
		ti.output_size = ti.input_size;
		ti.input = NULL;
		ti.input_size = 0;

		if ( false == saveCompress (&ti) )
			return false;

		ret = mtf_decode (ti.input, ti.input_size, MTF_TYPE);
		if ( 1 == ret )
		{
			/* mtf is inplace. swap output for input */
			ti.output = ti.input;
			ti.output_size = ti.input_size;
			ti.input = NULL;
			ti.input_size = 0;

			if ( false == saveDecompress (&ti) )
				return false;
		}
		else
		{
			puts("*** out of memory while decoding");
		}
	}
	else
	{
		puts("*** out of memory while encoding");
	}

	return true;
}
/* }}}1 */

/* {{{1 RLE */
static bool
testRLE (char * filename)
{
	struct testInfo	ti;
	int ret;


	if ( false == startTest (&ti, filename) )
		return false;

	ret = rle_packbits_compress (ti.input, ti.input_size, &ti.output, &ti.output_size, false);
	if ( RLE_RET_SUCCESS == ret )
	{
		if ( false == saveCompress (&ti) )
			return false;

		ret = rle_packbits_decompress (ti.input, ti.input_size, &ti.output, &ti.output_size);
		if ( RLE_RET_SUCCESS == ret )
		{
			if ( false == saveDecompress (&ti) )
				return false;
		}
		else
		{
			if ( RLE_RET_NOMEM == ret )
				puts("*** out of memory while decompressing");
			else
			if ( RLE_RET_MALFORMED == ret )
				puts("*** malformed rle data");
			else
				puts("*** unexpected error");
		}
	}
	else
	{
		if ( RLE_RET_TOOBIG == ret )
			puts("*** output will be bigger than input");
		else
		if ( RLE_RET_NOMEM == ret )
			puts("*** out of memory while compressing");
	}

	return true;
}
/* }}}1 */

static int
mainTest (char * filename, char * library)
{
	if ( 0 == strcmp (library, "bwt") )
		return testBWT (filename);
	else
	if ( 0 == strcmp (library, "huff") )
		return testHuff (filename);
	else
	if ( 0 == strcmp (library, "mtf") )
		return testMTF (filename);
	else
	if ( 0 == strcmp (library, "rle") )
		return testRLE (filename);
	else
	{
		/* ... */
	}

	puts ("*** unsupported test");

	return false;
}

int
main (int argc, char ** argv)
{
	if ( 3 != argc )
	{
		printf("usage: %s filename library\n", *argv);
		return EXIT_FAILURE;
	}

	if ( false ==  mainTest (argv[1], argv[2]) )
		return EXIT_FAILURE;
	
	return EXIT_SUCCESS;
}
