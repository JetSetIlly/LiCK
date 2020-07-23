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

#include	<bwt_lib.h>
#include	<mtf_lib.h>
#include	<rle_lib.h>
#include	<huff_lib.h>
#include	<types_lib.h>

#include	"compress_lib.h"


/*
 *	perform an rle on input data before bwt stage tends to improve block sorting times
 *	with poor sort routines, but damages compression rates
 */
#define PRE_RLE

/*
 * perform an rle after bwt stage. for some entropy encoders this tends to improve
 * performance with regards to entropy reduction but not with others
 */
#define POST_RLE

/*
 * what MTF model to use
 */
#define	MTF_TYPE	MTF1


enum COMPRESS_MODES
{
	RLE_AFTER_BWT = 0x1,
};


static void
stub_errorHook (char * error, void * callback_data)
{
}

static bool
stub_compressHook (unsigned char * output, unsigned long output_size, void * callback_data)
{
	return true;
}

static bool
stub_decompressStartHook (FILE * f, unsigned long * block_size, void * callback_data)
{
	return true;
}

static bool
stub_decompressEndHook (unsigned char * output, unsigned long output_size, void * callback_data)
{
	return true;
}


static int
compress (unsigned char * input, unsigned long input_size, unsigned char ** output, unsigned long * output_size, errorHookT errorHook, void * errorHook_data)
{
	unsigned char	*a, *b;
	unsigned long	l, m;

	int		ret;

	unsigned char	compress_mode = 0;


#ifdef PRE_RLE
	ret = rle_basic_compress (input, input_size, &a, &l, false, false);
	if ( RLE_RET_SUCCESS != ret )
	{
		if ( RLE_RET_NOMEM == ret )
		{
			errorHook ("out of memory while run length encoding", errorHook_data);
			return COMP_RET_NOMEM;
		}
		else
			errorHook ("unexpected response from run length encoder!!", errorHook_data);

		return COMP_RET_COMP;
	}
#else
	a = input;
	l = input_size;
#endif /* PRE_RLE */

	ret = bwt_encode (a, l, &b, &m);
	if ( 0 == ret )
	{
#ifdef PRE_RLE
		free (a);
#endif /* PRE_RLE */
		errorHook ("out of memory while burrows-wheeler transforming", errorHook_data);
		return COMP_RET_NOMEM;
	}
#ifdef PRE_RLE
	free (a);
#endif /* PRE_RLE */

	ret = mtf_encode (b, m, MTF_TYPE);
	if ( 0 == ret )
	{
		free (b);
		errorHook ("error during mtf encode", errorHook_data);
		return COMP_RET_COMP;
	}

	/* try to rle compress again to see if it has any effect */
#ifdef POST_RLE
	ret = rle_packbits_compress (b, m, &a, &l, true);
	if ( RLE_RET_TOOBIG != ret )
	{
		free (b);
		compress_mode |= RLE_AFTER_BWT;
	}
	else
#endif /* POST_RLE */
	{
		a = b;
		l = m;
	}

	/* encode with a pre padding space of sizeof compress_mode */
	ret = huff_encode (a, l, output, output_size, sizeof compress_mode);
	if ( HUFF_RET_SUCCESS != ret )
	{
		free (a);

		if ( HUFF_RET_NOMEM == ret )
		{
			errorHook ("out of memory while huffman encoding", errorHook_data);
			return COMP_RET_NOMEM;
		}
		else if ( HUFF_RET_TOOBIG == ret )
			errorHook ("compressed data would be bigger than original", errorHook_data);
		else
			errorHook ("unexpected response from huffman encoder!!", errorHook_data);

		return COMP_RET_COMP;
	}


	/* copy compress mode to first byte of output */
	**output = compress_mode & 0xff;

	free (a);

	return COMP_RET_OKAY;
}

static int
decompress (unsigned char * input, unsigned long input_size, unsigned char ** output, unsigned long * output_size, errorHookT errorHook, void * errorHook_data)
{
	unsigned char	*a, *b;
	unsigned long	l, m;

	int		ret;

	unsigned char	compress_mode;


	compress_mode = *input & 0xff;

	ret = huff_decode (input, input_size, &a, &l, sizeof compress_mode);
	if ( HUFF_RET_SUCCESS != ret )
	{
		if ( HUFF_RET_NOMEM == ret )
		{
			errorHook ("out of memory while huffman decoding", errorHook_data);
			return COMP_RET_NOMEM;
		}
		else
		if ( HUFF_RET_MALFORMED == ret )
			errorHook ("input data has confused the huffman decoder", errorHook_data);
		else
			errorHook ("unexpected response from huffman decoder!!", errorHook_data);

		return COMP_RET_COMP;
	}

#ifdef POST_RLE
	if ( RLE_AFTER_BWT == (compress_mode & RLE_AFTER_BWT) )
	{
		ret = rle_packbits_decompress (a, l, &b, &m);
		if ( RLE_RET_SUCCESS != ret )
		{
			free (a);
			if ( RLE_RET_NOMEM == ret )
			{
				errorHook ("out of memory while run length decoding", errorHook_data);
				return COMP_RET_NOMEM;
			}
			else
			if ( RLE_RET_MALFORMED == ret )
				errorHook ("input data has confused the run length decoder", errorHook_data);
			else
				errorHook ("unexpected response from run length decoder!!", errorHook_data);

			return COMP_RET_COMP;
		}

		free (a);
	}
	else
#endif /* POST_RLE */
	{
		b = a;
		m = l;
	}

	ret = mtf_decode (b, m, MTF_TYPE);
	if ( 0 == ret )
	{
		errorHook ("error during mtf decode", errorHook_data);
		free (b);
		return COMP_RET_COMP;
	}

	ret = bwt_decode (b, m, &a, &l);
	if ( 0 == ret )
	{
		errorHook ("out of memory while reversing burrows-wheeler transform", errorHook_data);
		free (b);
		return COMP_RET_NOMEM;
	}
	free (b);

#ifdef PRE_RLE
	ret = rle_basic_decompress (a, l, output, output_size, false);
	if ( RLE_RET_SUCCESS != ret )
	{
		free (a);

		if ( RLE_RET_NOMEM == ret )
		{
			errorHook ("out of memory while run length decoding", errorHook_data);
			return COMP_RET_NOMEM;
		}
		else
		if ( RLE_RET_MALFORMED == ret )
			errorHook ("input data has confused the run length decoder", errorHook_data);
		else
			errorHook ("unexpected response from run length decoder!!", errorHook_data);

		return COMP_RET_COMP;
	}

	free (a);
#else
	*output = a;
	*output_size = l;
#endif /* POST_RLE */

	return COMP_RET_OKAY;
}



int
comp_compressFile (struct compressInfo * info, FILE * inputf, unsigned long max_block)
{
	unsigned char	*input,
								*output;

	unsigned long input_size,
								output_size;

	int		compress_ret;


	/* stubify callback hooks if necessary */
	compressHookT		compressHook;
	errorHookT			errorHook;
	
	if ( NULL != info )
	{
		if ( NULL == info->compressHook )
			compressHook = stub_compressHook;
		else
			compressHook = info->compressHook;

		if ( NULL == info->errorHook )
			errorHook = stub_errorHook;
		else
			errorHook = info->errorHook;
	}


	/* allocate enough memory for input data */
	input = malloc (max_block * sizeof *input);
	if ( NULL == input )
		return COMP_RET_NOMEM;

	/* loop until end of file is reached */
	do
	{
		input_size = fread (input, sizeof *input, max_block, inputf);
		if ( 0 == input_size )
		{
			free (input);

			if ( 0 != feof (inputf) )
				return COMP_RET_READ;

			return COMP_RET_OKAY;
		}

		/* do compression */
		compress_ret = compress (input, input_size, &output, &output_size, errorHook, info?info->errorHook_data:NULL);
		if ( COMP_RET_OKAY != compress_ret )
		{
			free (input);
			return compress_ret;
		}

		/* call compression hook */
		if ( false == compressHook (output, output_size, info?info->compressHook_data:NULL) )
		{
			free (output);
			free (input);
			return COMP_RET_HOOKEND;
		}

		free (output);
	}
	while ( input_size == max_block );

	free (input);

	return COMP_RET_OKAY;
}


int
comp_decompressFile (struct compressInfo * info, FILE * inputf, bool until_eof, unsigned long data_length)
{
	unsigned long	block_size,
								max_block_size = 0;

	unsigned char	* input = NULL,
								* output;

	unsigned long	input_size,
								output_size;

	int decompress_ret;


	/* stubify callback hooks if necessary */
	decompressStartHookT 	decompressStartHook;
	decompressEndHookT    decompressEndHook;
	errorHookT						errorHook;

	if ( NULL != info )
	{
		if ( NULL == info->decompressStartHook )
			decompressStartHook = stub_decompressStartHook;
		else
			decompressStartHook = info->decompressStartHook;
		
		if ( NULL == info->decompressEndHook )
			decompressEndHook = stub_decompressEndHook;
		else
			decompressEndHook = info->decompressEndHook;

		if ( NULL == info->errorHook )
			errorHook = stub_errorHook;
		else
			errorHook = info->errorHook;
	}


	do
	{
		if ( false == decompressStartHook (inputf, &block_size, info?info->decompressHook_data:NULL) )
			return COMP_RET_HOOKEND;

		/*
		 * if block_size is greater than the amount of memory
		 * already allocated then reallocate
		 */
		if ( block_size > max_block_size )
		{
			free (input);
			input = malloc (block_size * sizeof *input);
			if ( NULL == input )
				return COMP_RET_NOMEM;
			max_block_size = block_size;
		}

		/* check to see if data_length is still valid */
		if ( false == until_eof )
		{
			if ( data_length < block_size )
			{
				free (input);
				return COMP_RET_MALFORMED;
			}
			data_length -= block_size;
		}

		/* read data */
		input_size = fread (input, sizeof *input, block_size, inputf);
		if ( 0 == input_size )
		{
			free (input);

			/* return read error if eof has not been reached */
			if ( 0 != feof (inputf) )
				return COMP_RET_READ;

			return COMP_RET_OKAY;
		}

		/*
		 * amount of data read is different to
		 * the amount that was expected
		 */
		if ( block_size != input_size )
		{
			free (input);

			/* return read error if eof has not been reached */
			if ( 0 != feof (inputf) )
				return COMP_RET_READ;

			/* eof has been reached but it wasn't expected */
			return COMP_RET_UNEXPECTEDEND;
		}

		/* do decompression */
		decompress_ret = decompress (input, input_size, &output, &output_size, errorHook, info?info->errorHook_data:NULL);
		if ( COMP_RET_OKAY != decompress_ret )
		{
			free (input);
			return decompress_ret;
		}

		/* call decompression end hook */
		if ( false == decompressEndHook (output, output_size, info?info->decompressHook_data:NULL) )
		{
			free (output);
			free (input);
			return COMP_RET_HOOKEND;
		}

		free (output);
	}
	while ( (until_eof ? true : (0 != data_length)) && 0 == feof(inputf) ); /* while eof hasn't been reached */

	free (input);

	return COMP_RET_OKAY;
}

