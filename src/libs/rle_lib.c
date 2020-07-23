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
#include	<limits.h>

#include	<types_lib.h>
#include	"rle_lib.h"

/* print out some basic debugging info */
//#define RLE_DEBUG	1

/* dump relevent data during encoding/decoding */
//#define RLE_DEBUG_DUMP 1

/* {{{1 OUTPUT SIZE HANDLER */
static inline int
outputSizeCheck (bool check, unsigned long grow_size,
	unsigned char ** output, unsigned long output_size, unsigned long * max_output_size)
{
	unsigned char	* tmp;
	
	/* if output is now the same size as the input */
	if ( output_size >= *max_output_size )
	{
		/* is output too big */
		if ( false != check )
		{
			free (*output);
			return RLE_RET_TOOBIG;
		}

		*max_output_size += grow_size;
		tmp = realloc (*output, *max_output_size);
		if ( NULL == tmp )
		{
			free (*output);
			return RLE_RET_NOMEM;
		}
		*output = tmp;
	}

	return RLE_RET_SUCCESS;
}	
/* }}}1 */

/* {{{1 BASIC METHOD */
/*
 * encoded format
 * --------------
 *
 * . 4 byte, big endian integer indicating expecting output size
 * . rle encoded data
 *   . input character
 *	 . if two identical characters are written to the output, a
 *	   1 byte repeat count is then written -- a zero means that
 *     only two characters were consecutive.
 *
 *   In the case of RLE-0, only input values of zero are encoded
 */

int
rle_basic_compress ( unsigned char *input, unsigned long input_size,
							 unsigned char **output, unsigned long *output_size,
							 bool output_size_check, bool rle0 )
{
	int	res;

	unsigned long   input_c = 0;
	unsigned char		*input_p = input;
	unsigned long   output_i = 0;

	unsigned long		max_output_size;

	unsigned char   last = 0;
	unsigned char   c = 0;

	unsigned char  *tmp;					/* tmp pointer -- used for realloc() */

/* {{{2 DEBUG CODE */
#ifdef RLE_DEBUG
printf("RLE Compression\n-----\ninput size -> %ld\n", input_size);
#endif
/* }}} */

	max_output_size = input_size;
	*output = malloc ( max_output_size * sizeof **output );
	if ( NULL == *output )
		return RLE_RET_NOMEM;

	/* write input size to output */
	(*output)[0]= (input_size >> 24) & 0xff;
	(*output)[1]= (input_size >> 16) & 0xff;
	(*output)[2]= (input_size >> 8) & 0xff;
	(*output)[3]= (input_size) & 0xff;
	output_i += 4;

	for ( ;; )
	{
		/* read character from input */
		c = *input_p;
		input_p += sizeof *input_p;
		++ input_c;

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
printf("%c\n", c);
#endif
/* }}} */

		/* write character to output */
		(*output)[output_i] = c;
		++ output_i;

		res = outputSizeCheck (output_size_check, input_size, output, output_i, &max_output_size);
		if ( RLE_RET_SUCCESS != res )
			return res;

		/* exit for loop if that was the last character in the input */
		if ( input_c == input_size )
			break;

		if ( (true == rle0 && 0 == c && c == last) || c == last )
		{
			int i = 0;

			do
			{
				c = *input_p;
				input_p += sizeof *input_p;
				++ input_c;

				if ( c != last )
					break;	/* out of for loop */

				++ i;
			}
			while (	i < 255 && input_c < input_size );

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
printf("repeated %d times\n", i);
#endif
/* }}} */

			(*output)[output_i] = i;		/* write repeat count to output */
			++ output_i;

			res = outputSizeCheck (output_size_check, input_size, output, output_i, &max_output_size);
			if ( RLE_RET_SUCCESS != res )
				return res;

			if ( i != 255 )
			{
/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
printf("%c\n", c);
#endif
/* }}} */
				(*output)[output_i] = c;
				++ output_i;
			}

			if ( input_c == input_size )
				break;
		}

		last = c;
	}

	/* trim memory */
	*output_size = output_i;
/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG
printf("output size -> %d\n", *output_size);
#endif
/* }}} */
	tmp = realloc (*output, *output_size);
	if (NULL == tmp)
	{
		free (*output);
		return RLE_RET_NOMEM;
	}
	*output = tmp;

	return RLE_RET_SUCCESS;
}

int
rle_basic_decompress ( unsigned char * input, unsigned long input_size,
		unsigned char ** output, unsigned long * output_size,
		bool rle0 )
{
	unsigned long	input_c = 0;
	unsigned char *input_p = input;
	unsigned long	output_i = 0;
	unsigned char *output_p;

	unsigned char last = 0;
	unsigned char	c;


/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG
printf("\nRLE Decompression\n-----\ninput size -> %ld\n", input_size);
#endif
/* }}} */

	/* read in output size */
	*output_size = (*(input_p)) << 24;
	*output_size |= (*(input_p+1)) << 16;
	*output_size |= (*(input_p+2)) << 8;
	*output_size |= (*(input_p+3));
	input_p += 4;
	input_c += 4;

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG
printf("output size -> %ld\n", *output_size);
#endif
/* }}} */

	output_p = *output = malloc (*output_size * sizeof **output);
	if ( NULL == output_p )
		return RLE_RET_NOMEM;

	for ( ;; )
	{
		c = *input_p;
		input_p += sizeof *input_p;
		++ input_c;

		*output_p = c;
		output_p += sizeof *output_p;

		if ( input_c == input_size )
			break;

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
printf("*%c\n", c);
#endif
/* }}} */

		/* if last character is the same as the previous */
		if ( (true == rle0 && 0 == c && c == last) || c == last )
		{
			unsigned char	i;

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
puts("repeat");
#endif
/* }}} */

			/* get repeat count */
			i	= *input_p;
			input_p += sizeof *input_p;
			++ input_c;
			if ( input_c == input_size )
				break;

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
printf("count -> %d\n", i);
#endif
/* }}} */

			output_i += i;

			if ( output_i > *output_size )
			{
				free (*output);
				return RLE_RET_MALFORMED;
			}

			while ( i -- > 0 )
			{
/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG_DUMP
printf("%c\n", c);
#endif
/* }}} */
				*output_p = c;
				output_p += sizeof *output_p;
			}
		}

		last = c;
	}

	return RLE_RET_SUCCESS;
}
/* }}} */

/* {{{1 PACKBITS METHOD */
int
rle_packbits_compress ( unsigned char *input, unsigned long input_size,
	unsigned char **output, unsigned long *output_size,
	bool output_size_check )
{
	int	res;

	unsigned long	i, j, k;

	unsigned long	output_i = 0;
	unsigned long	max_output_size;

	unsigned char	* tmp; /* used for realloc() */


	max_output_size = input_size;
	*output = malloc ( max_output_size * sizeof **output );
	if ( NULL == *output )
		return RLE_RET_NOMEM;

	/* write input size to output */
	(*output)[0]= (input_size >> 24) & 0xff;
	(*output)[1]= (input_size >> 16) & 0xff;
	(*output)[2]= (input_size >> 8) & 0xff;
	(*output)[3]= (input_size) & 0xff;
	output_i += 4;

	/* input loop */
	for ( i = 0; i < input_size; ++ i )
	{
		/* handle duplicate runs */
		if ( (i < input_size-1) && (input[i] == input[i+1]) )
		{
			k = 2;

			while ( (i+k <= input_size-1) && (input[i+k] == input[i]) && (k < SCHAR_MAX) )
				++ k;

			(*output)[output_i] = -k;
			++ output_i;

			res = outputSizeCheck (output_size_check, input_size, output, output_i, &max_output_size);
			if ( RLE_RET_SUCCESS != res )
				return res;

			(*output)[output_i] = input[i];
			++ output_i;

			res = outputSizeCheck (output_size_check, input_size, output, output_i, &max_output_size);
			if ( RLE_RET_SUCCESS != res )
				return res;

			i += k-1;
		}
		else
		/* handle non-duplicate runs */
		if ( i < input_size-1 )
		{
			if ( input[i] != input[i+1] )
			{
				k = 2;

				while ( (i+k+1 <= input_size-1) && (input[i+k] != input[i+k+1]) && (k < SCHAR_MAX) )
					++ k;
	
				(*output)[output_i] = k;
				++ output_i;
	
				res = outputSizeCheck (output_size_check, input_size, output, output_i, &max_output_size);
				if ( RLE_RET_SUCCESS != res )
					return res;
	
				for ( j = 0 ; j < k ; ++ j )
				{
					(*output)[output_i] = input[i+j];
					++ output_i;
	
					res = outputSizeCheck (output_size_check, input_size, output, output_i, &max_output_size);
					if ( RLE_RET_SUCCESS != res )
						return res;
				}
				
				i += k-1;
			}
		}
		else /* i == input_size-1 */
		{
			/* edge case where there is one stray character at end of input stream */
			(*output)[output_i] = 1;
			(*output)[output_i+1] = input[i];
			output_i += 2;
		}
	}

	/* trim memory */
	*output_size = output_i;
	tmp = realloc (*output, *output_size);
	if (NULL == tmp)
	{
		free (*output);
		return RLE_RET_NOMEM;
	}
	*output = tmp;

	return RLE_RET_SUCCESS;
}

int
rle_packbits_decompress ( unsigned char *input, unsigned long input_size,
	unsigned char **output, unsigned long *output_size )
{
	unsigned long	i, j, k;
	unsigned long	input_i = 0;
	unsigned long	output_i = 0;

	signed char	dup = 0;

	/* read in output size */
	*output_size = (input[0]) << 24;
	*output_size |= (input[1]) << 16;
	*output_size |= (input[2]) << 8;
	*output_size |= (input[3]);
	input_i += 4;

/* {{{2 DEBUG_CODE */
#ifdef RLE_DEBUG
printf("output size -> %ld\n", *output_size);
#endif
/* }}} */

	*output = malloc (*output_size * sizeof **output);
	if ( NULL == *output )
		return RLE_RET_NOMEM;

	while ( input_i < input_size )
	{
		dup = input[input_i];

		++ input_i;

		if ( dup < 0 )
		{
			/* handle duplicate runs */
			for ( dup = -dup; dup > 0; -- dup )
				(*output)[output_i++] = input[input_i];

			++ input_i;
		}
		else
		{
			/* handle non-duplicate runs */
			for ( ; dup > 0; -- dup )
				(*output)[output_i++] = input[input_i++];
		}
	}

	return RLE_RET_SUCCESS;
}
/* }}}1 */

