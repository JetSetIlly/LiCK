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
#include	<string.h>

#include  "types_lib.h"
#include	"platform.h"

#ifdef AMIGA
#include	<exec/types.h>
#include	<dos/dos.h>
#include	<proto/dos.h>
#endif

bool
pl_getFileInfo (char *file, struct fsInfo *fs)
{
#ifdef AMIGA
#ifdef AMIGA_FFS_COMP
  BPTR h;
  struct FileInfoBlock *fib;


  h = Lock (file, ACCESS_READ);
  if (NULL == h)
    return false;

  fib = AllocDosObject (DOS_FIB, NULL);
  if (NULL == fib)
  {
    UnLock (h);
    return false;
  }

  if (FALSE == Examine (h, fib))
  {
    UnLock (h);
    FreeDosObject (DOS_FIB, fib);
    return false;
  }

  strncpy (fs->comment, fib->fib_Comment, COMMENT_LEN);
  fs->protection = fib->fib_Protection;

  FreeDosObject (DOS_FIB, fib);
  UnLock (h);

#endif /* AMIGA_FFS_COMP */
#endif /* AMIGA */

  return true;
}
