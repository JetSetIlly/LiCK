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

#ifndef TYPESLIB_H
#define TYPESLIB_H

/* check to see if compiler is C99 or not
-- define bool
*/
#if __STDC_VERSION__ == 199901L

#include <stdbool.h>

#else

typedef char    bool;
#ifndef true
#define true    1
#endif
#ifndef false
#define false   0
#endif

#endif /* C99 */

/* used for qsmodel -- remove eventually */
#ifdef GCC
#define Inline inline
#else
#define Inline __inline
#endif

#if INT_MAX > 0x7FFF
typedef unsigned short uint2;  /* two-byte integer (large arrays)      */
typedef unsigned int   uint4;  /* four-byte integers (range needed)    */
#else
typedef unsigned int   uint2;
typedef unsigned long  uint4;
#endif

#endif /* TYPESLIB_H */

