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

#ifndef BITQLIB_H
#define BITQLIB_H


enum BITQ_ERRORS
{
	BITQ_CONTINUE,
	BITQ_POP_READY,
	BITQ_TOO_MUCH,
	BITQ_EMPTY
};


struct bitqState;


/*
 * returns new instance of struct bitqState.  structure is initialised
 * ala bitq_initState()
 */
struct bitqState *	bitq_newState (void);

/*
 * free instance of struct bitqState
 */
void bitq_free (struct bitqState *);

/*
 * ensures state structure is empty
 */
void bitq_initState (struct bitqState *);

/*
 * returns either:
 * 	BITQ_CONTINUE
 * 	BITQ_TOO_MUCH
 * 	BITQ_POP_READY
 */
int	bitq_push (struct bitqState *, unsigned int data, unsigned char data_len);


/*
 *  bit should be 0 to indicate 0 and >0 to indicate 1
 *
 *  returns as bitq_push()
 */
int bitq_pushBit (struct bitqState *, unsigned char bit);

/*
 * returns either:
 * 	BITQ_CONTINUE
 * 	BITQ_EMPTY
 * 	BITQ_POP_READY
 */
int	bitq_popChar (struct bitqState *, unsigned char * output);
int bitq_popBit (struct bitqState *, unsigned char * bit);

/*
 * returns BITQ_POP_READY until queue is empty, at which
 * point it returns BITQ_EMPTY. for example,
 *
 * while ( 1 == bitq_flush (bitq, &output_c) )
 * {
 *   do something with output_c
 * }
 *
 */
int	bitq_flush (struct bitqState *, unsigned char * output);



/*
 * write stream functions
 * ----------------------
 * wrapper functions for the main bitq functions above
 * conveniently handles multiple popping of queue when
 * characters are ready
 *
 * doesn't resize memory
 *
 */

struct bitqStream;

/*
 * returns stream control structure. you need to supply some
 * allocated memory for the stream, the size of the stream
 * and pointer to record the amount of the stream that has
 * been used.
 */
struct bitqStream * bitq_newStream (unsigned char * stream, unsigned long size, unsigned long * len);

/*
 * doesn't free the memory associated with the
 * stream argument passed to bitq_newStream()
 */
void bitq_freeStream (struct bitqStream *);

/*
 * pushes 'data' onto bit queue and pops complete bytes when appropriate
 * and writes to stream 
 *
 * returns either
 * 	BITQ_TOO_MUCH
 * 	BITQ_CONTINUE
 */
int bitq_writeStream (struct bitqStream *, unsigned int data, unsigned char data_len);

/*
 * empties bitq into stream
 *
 * returns either
 * 	BITQ_TOO_MUCH
 * 	BITQ_CONTINUE
 */
int bitq_flushWriteStream (struct bitqStream *);

/*
 * read data_len number of bits from stream and leave in data
 *
 * returns number of bits actually read
 *
 * if return val != data_len then the end of the stream has been reached.
 */
unsigned char bitq_readStream (struct bitqStream * stream, unsigned char * data, unsigned char data_len);

#endif /* BITQLIB_H */

