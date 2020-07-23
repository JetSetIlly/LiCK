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
#include	<limits.h>

#include	"bitq_lib.h"

/* state control {{{1 */
struct bitqState
{
	unsigned long	buffer;
	unsigned char	buffer_used;
};

void
bitq_initState (struct bitqState * state)
{
	state->buffer = 0;
	state->buffer_used = 0;
}

struct bitqState *
bitq_newState (void)
{
	struct bitqState * ns;

	ns = malloc (sizeof *ns);
	if ( NULL == ns )
		return NULL;

	bitq_initState (ns);

	return ns;
}

void
bitq_free (struct bitqState * state)
{
	free (state);
}
/* }}}1 */

/* level 1 functions {{{1 */
int
bitq_push (struct bitqState * state, unsigned int data, unsigned char data_len)
{
	if ( data_len > CHAR_BIT * 3 )
	{
		if ( (data_len - (CHAR_BIT * 3)) < state->buffer_used )
			return BITQ_TOO_MUCH;
	}

	state->buffer <<= data_len;
	state->buffer	|= data;
	state->buffer_used += data_len;

	if ( state->buffer_used >= CHAR_BIT )
		return BITQ_POP_READY;

	return BITQ_CONTINUE;
}

int
bitq_pushBit (struct bitqState * state, unsigned char bit)
{
	return bitq_push (state, bit?1:0, 1);
}

int
bitq_popChar (struct bitqState * state, unsigned char * output)
{
	unsigned long n;

	if ( 0 == state->buffer_used )
		return BITQ_EMPTY;

	if ( state->buffer_used >= 3*CHAR_BIT )
	{
		state->buffer	<<= (4*CHAR_BIT) - state->buffer_used;
		n = state->buffer & 0xFF000000;
		n >>= 3*CHAR_BIT;
		*output = (unsigned char) n;
		state->buffer >>= (4*CHAR_BIT) - state->buffer_used;
		state->buffer_used -= CHAR_BIT;
		return BITQ_POP_READY;
	}

	if ( state->buffer_used >= 2*CHAR_BIT )
	{
		state->buffer	<<= (3*CHAR_BIT) - state->buffer_used;
		n = state->buffer & 0xFF0000;
		n >>= 2*CHAR_BIT;
		*output = (unsigned char) n;
		state->buffer >>= (3*CHAR_BIT) - state->buffer_used;
		state->buffer_used -= CHAR_BIT;
		return BITQ_POP_READY;
	}

	state->buffer	<<= (2*CHAR_BIT) - state->buffer_used;
	n = state->buffer & 0xFF00;
	n >>= CHAR_BIT;
	*output = (unsigned char) n;
	state->buffer >>= (2*CHAR_BIT) - state->buffer_used;

	if ( state->buffer_used > CHAR_BIT )
		state->buffer_used -= CHAR_BIT;
	else
		state->buffer_used = 0;
	
	if ( state->buffer_used >= CHAR_BIT )
		return BITQ_POP_READY;

	return BITQ_CONTINUE;
}

int
bitq_popBit (struct bitqState * state, unsigned char * output)
{
	unsigned long	t;

	if ( 0 == state->buffer_used )
		return BITQ_EMPTY;

	t = state->buffer >> (state->buffer_used - 1);
	*output = t & 0x1;

	/*
	 * mask off popped bit
	 */
	t = 0x1 << (state->buffer_used - 1);
	state->buffer &= ~t;
	-- state->buffer_used;

	if ( state->buffer_used >= CHAR_BIT )
		return BITQ_POP_READY;

	return BITQ_CONTINUE;
}

int
bitq_flush (struct bitqState * state, unsigned char * output)
{
	if ( 0 == state->buffer_used )
		return BITQ_EMPTY;

	if ( state->buffer_used > 3*CHAR_BIT )
	{
		*output = (state->buffer >> 3*CHAR_BIT) & 0xff;
		state->buffer_used -= CHAR_BIT;
		return BITQ_POP_READY;
	}

	if ( state->buffer_used > 2*CHAR_BIT )
	{
		*output = (state->buffer >> 2*CHAR_BIT) & 0xff;
		state->buffer_used -= CHAR_BIT;
		return BITQ_POP_READY;
	}

	if ( state->buffer_used > CHAR_BIT )
	{
		*output = (state->buffer >> CHAR_BIT) & 0xff;
		state->buffer_used -= CHAR_BIT;
		return BITQ_POP_READY;
	}

	*output = (state->buffer << (CHAR_BIT-state->buffer_used)) & 0xff;

	if ( state->buffer_used > CHAR_BIT )
		state->buffer_used -= CHAR_BIT;
	else
		state->buffer_used = 0;

	return BITQ_POP_READY;
}
/* }}}1 */

/* streaming {{{1 */
struct bitqStream
{
	struct bitqState	* state;

	unsigned char	* stream;

	/* allocated memory */
	unsigned long	size;

	/* amount of stream currently used */
	unsigned long * len;
};

struct bitqStream *
bitq_newStream (unsigned char * stream, unsigned long size, unsigned long * len)
{
	struct bitqStream * ns;

	if ( NULL == stream || 0 == size || NULL == len )
		return NULL;

	ns = malloc (sizeof *ns);
	if ( NULL == ns )
		return NULL;

	ns->state = bitq_newState ();
	if ( NULL == ns->state )
	{
		free (ns);
		return NULL;
	};

	ns->stream = stream;
	ns->len = len;
	ns->size = size;

	return ns;
}

void
bitq_freeStream (struct bitqStream * stream)
{
	free (stream->state);
	free (stream);
}

int
bitq_writeStream (struct bitqStream * stream, unsigned int data, unsigned char data_len)
{
	int	ret;
	unsigned char output_c;

	ret = bitq_push (stream->state, data, data_len);

	if ( BITQ_TOO_MUCH == ret )
		return BITQ_TOO_MUCH;

	if ( BITQ_POP_READY == ret )
	{
		do
		{
			ret = bitq_popChar (stream->state, &output_c);

			stream->stream[*stream->len] = output_c;
			++ *stream->len;
			if ( *stream->len == stream->size )
				return BITQ_TOO_MUCH;
		}
		while ( BITQ_POP_READY == ret );
	}

	return BITQ_CONTINUE;
}

int
bitq_flushWriteStream (struct bitqStream * stream)
{
	unsigned char output_c;

	while ( 1 == bitq_flush (stream->state, &output_c) )
	{
		stream->stream[*stream->len] = output_c;
		++ *stream->len;
		if ( *stream->len > stream->size )
			return BITQ_TOO_MUCH;
	}

	return BITQ_CONTINUE;
}

unsigned char
bitq_readStream (struct bitqStream * stream, unsigned char * data, unsigned char data_len)
{
	unsigned char		c,
									ret;

	*data = 0;

	for ( ret = 0; ret < data_len; ++ ret )
	{
		*data <<= 1;

		if ( BITQ_EMPTY == bitq_popBit (stream->state, &c) )
		{
			/* run out of input data */
			if ( *stream->len == stream->size )
				return ret;

			bitq_push (stream->state, stream->stream[*stream->len], CHAR_BIT);
			++ *stream->len;
			bitq_popBit (stream->state, &c);
		}

		*data |= c;
	}

	return ret;
}
/* }}}1 */
