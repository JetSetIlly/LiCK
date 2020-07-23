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

#ifndef COMPRESS_LIB_H
#define COMPRESS_LIB_H

#include	<types_lib.h>



typedef	bool (*compressHookT) (unsigned char * output, unsigned long output_size, void * callback_data);
typedef	bool (*decompressStartHookT) (FILE * input, unsigned long * block_size, void * callback_data);
typedef	bool (*decompressEndHookT) (unsigned char * output, unsigned long output_size, void * callback_data);
typedef	void (*errorHookT) (char * error, void * callback_data);


struct compressInfo
{
	void * compressHook_data;
	compressHookT		compressHook;

	void * decompressHook_data;
	decompressStartHookT 	decompressStartHook;
	decompressEndHookT    decompressEndHook;

	/* error hook can be NULL */
	void * errorHook_data;
	errorHookT	errorHook;
};


enum COMP_RET_CODES
{
	COMP_RET_OKAY = 0,
	COMP_RET_NOMEM,			/* out of memory */
	COMP_RET_COMP,     	/* error occured during compression/decompression */
	COMP_RET_HOOKEND,	 	/* was asked to be terminated by callback hook */
	COMP_RET_BADARGS,		/* a supplied argument was NULL when it shouldn't be */
	COMP_RET_READ, 		  /* tried to read data from file but couldn't */

	/* the following are returned only by comp_decompressFile() */
	COMP_RET_UNEXPECTEDEND,	 /* unexpected end of data when reading input */
	COMP_RET_MALFORMED
};


int	comp_compressFile (struct compressInfo *, FILE * inputf, unsigned long max_block);

/*
 * the `until_eof` argument instructs the decompressFile function to
 * read data until the end of the file if set to true. If it is set to
 * false, the data_length argument limits the amount of data to be read
 * (it is otherwise ignored).
 *
 * If the decompress routines need to read more data than is allowed by
 * data_length (ie. the block size returned by decompressStartHook() is
 * greater than data_length) then a value of COMP_RET_MALFORMED is returned.
 */
int	comp_decompressFile (struct compressInfo *, FILE * inputf, bool until_eof, unsigned long data_length);

#endif /* COMPRESS_LIB_H */

