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

#ifndef BWTLIB_H
#define BWTLIB_H

int bwt_encode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size);
int bwt_decode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size);

#endif /* BWT_H */
