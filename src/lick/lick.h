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

#ifndef LICK_H
#define LICK_H

#include	"llist_lib.h"

enum LICK_ACTIONS
{
  LA_NOTHING = 0,
  LA_USAGE,
  LA_ADD,
  LA_VIEW,
  LA_EXTRACT
};

enum COMPRESS_TYPES
{
  CT_NONE = 0
};

enum EXTRACT_OPTIONS
{
  EXT_OPT_NONE = 0x0,
  EXT_OPT_CLOBBER = 0x1,
  EXT_OPT_TOUCH = 0x2
};

struct lickOptions
{
  char *archive;
  int action;
  struct llist *files;
  int compress_mode;
  int options;
};

#endif /* LICK_H */
