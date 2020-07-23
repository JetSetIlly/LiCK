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

#ifndef MTFLIB_H
#define MTFLIB_H

/*
 * support model types
 *
 * MTF-0 -- plain MTF
 * MTF-1 -- move to front only if moving from second position
 * 					else move to second position
 * MTF-2 -- as MTF-1 but only move to front if the previous move
 * 					was from position one to position zero
 *
 * 
 * model defaults to zero if specified model_type is unsupported
 */

enum MODEL_TYPES
{
	MTF0 = 0,
	MTF1,
	MTF2
};

int	mtf_encode (unsigned char *data, unsigned long data_size, int model_type);
int mtf_decode (unsigned char *data, unsigned long data_size, int model_type);

#endif
