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

#ifndef RLELIB_H
#define RLELIB_H

#include	<types_lib.h>


enum RLE_RET_CODES
{
	RLE_RET_SUCCESS,
	RLE_RET_NOMEM,
	RLE_RET_TOOBIG,	 /* compress -- output will be bigger than input */
	RLE_RET_MALFORMED /* decompress -- indicated output size is wrong */
};


/*
 * In both compress and decompress functions an rle0 argument
 * value of true, limits the coding of runs of zero only.
 */
int	rle_basic_compress ( unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long * output_size, bool output_size_check, bool rle0 );
int	rle_basic_decompress ( unsigned char * input, unsigned long input_size, unsigned char ** output, unsigned long * output_size, bool rle0 );

/* packbits algorithm sometimes known as byterun1 */
int	rle_packbits_compress ( unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long * output_size, bool output_size_check );
int	rle_packbits_decompress ( unsigned char * input, unsigned long input_size, unsigned char ** output, unsigned long * output_size );

#endif /* RLELIB_H */

