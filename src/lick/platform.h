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

#ifndef PLATFORM_H
#define PLATFORM_H

#include  "types_lib.h"

#ifdef AMIGA
#define PLATFORM_ID		1

#ifdef AMIGA_FFS_COMP
#define FILESYSTEM_ID		1

#define COMMENT_LEN	79
struct fsInfo
{
  char comment[COMMENT_LEN + 1];
  long protection;
};
#endif /* AMIGA_FFS_COMP */
#elif UNIX
#define PLATFORM_ID		2

#ifdef UFS_COMP
#define FILESYSTEM_ID		1

struct fsInfo
{
  char fudge;
};
#endif /* UFS_COMP */
#endif /* UNIX */

bool pl_getFileInfo (char *file, struct fsInfo *fs);

#endif /* PLATFORM_H */
