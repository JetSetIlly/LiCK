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

#ifndef FILEFORMAT_H
#define FILEFORMAT_H

#include	"types_lib.h"
#include	"platform.h"

struct hunkInfo
{
  char *file;
  unsigned char mode;
  unsigned long checksum;
  unsigned long compressed_size;
  unsigned long uncompressed_size;
  char *compressed_data;
  struct fsInfo fs_info;
};

enum ARCHIVE_STATUSES
{
  AS_NOT_EXIST,
  AS_NOT_LICK_FILE,
  AS_LICK_FILE
};
typedef int ARCHIVE_STATUS;

ARCHIVE_STATUS ff_checkFile (char *file);
bool ff_writeArchiveHead (FILE *);
bool ff_writeFile (FILE *, struct hunkInfo *);

#endif /* FILEFORMAT_H */
