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

#ifndef HUFFLIB_H
#define HUFFLIB_H

/*
 * encoded format
 * --------------
 *
 * . 4 byte, big endian integer indicating expecting output size
 * . 1 byte indicating dictionary size -- 256 entries in the dictionary
 *   means that this byte will be 0x0
 * . dictionary
 *   . 1 byte entry
 *   . 1 byte code length
 * . huffman encoded input
 */

enum HUFF_RET_CODES
{
	HUFF_RET_SUCCESS,
	HUFF_RET_NOMEM,
	
	/* encoder only */
	HUFF_RET_TOOBIG,
	HUFF_RET_EMPTY_INPUT,

	/* decoder only */
	HUFF_RET_MALFORMED
};

int huff_encode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size, unsigned long pre_padding);
int huff_decode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size, unsigned long pre_padding);

#endif /* HUFFLIB_H */
