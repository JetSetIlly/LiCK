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
#include	<stdio.h>
#include	<string.h>

#include  "types_lib.h"
#include	"locale.h"
#include	"fileformat.h"

/*
   overview of file format
   -=-=-=-=-=-=-=-=-=-=-=-

   file_name          (var length, NULL terminated)
   compressed size    (32 bits)
   uncompressed size  (32 bits)
   check sum          (32 bits)
   crunch mode        (8 bit)
   platform id        (8 bit)
   filesystem id      (8 bit)

   fs info size       (32 bit)
   fs info            ("fs info size" bits)

   compressed data    ("compressed size" bits)

   (words are written in big endian order)
 */

#define	ARCHIVE_HEAD		"LiCKa"
#define	ARCHIVE_HEAD_LEN	5
#define	HUNK_HEAD			"LiCKf"
#define	HUNK_HEAD_LEN		5

static bool ff_writeU8BitWord (FILE * h, unsigned char i);
//static bool ff_writeU16BitWord (FILE * h, unsigned int i);
static bool ff_writeU32BitWord (FILE * h, unsigned long i);

/* check whether or not file is a lick archive
   future versions should check the integrity of
   lick archives */
ARCHIVE_STATUS
ff_checkFile (char *file)
{
  FILE *f;
  char buff[ARCHIVE_HEAD_LEN + 1];

  f = fopen (file, "rb+");
  if (NULL == f)
    return AS_NOT_EXIST;
  if (ARCHIVE_HEAD_LEN == fread (buff, sizeof (char), ARCHIVE_HEAD_LEN, f))
  {
    if (0 == strncmp (buff, ARCHIVE_HEAD, ARCHIVE_HEAD_LEN))
    {
      fclose (f);
      return AS_LICK_FILE;
    }
  }

  fclose (f);
  return AS_NOT_LICK_FILE;
}

bool
ff_writeArchiveHead (FILE * h)
{
  if (ARCHIVE_HEAD_LEN != fwrite (ARCHIVE_HEAD, sizeof (char), ARCHIVE_HEAD_LEN, h))
       return false;

  return true;
}

bool
ff_writeFile (FILE * h, struct hunkInfo *hi)
{
  size_t s;

  /* hunk header */
  if (HUNK_HEAD_LEN != fwrite (HUNK_HEAD, sizeof (char), HUNK_HEAD_LEN, h))
       return false;

  /* filename */
  s = strlen (hi->file) + 1;
  if (s != fwrite (hi->file, sizeof (char), s, h))
       return false;

  /* compressed size */
  if (0 == ff_writeU32BitWord (h, hi->compressed_size))
    return false;

  /* uncompressed size */
  if (0 == ff_writeU32BitWord (h, hi->uncompressed_size))
    return false;

  /* check sum */
  if (0 == ff_writeU32BitWord (h, hi->checksum))
    return false;

  /* compress mode */
  if (0 == ff_writeU8BitWord (h, hi->mode))
    return false;

  /* platform data */
  if (0 == ff_writeU8BitWord (h, PLATFORM_ID))
    return false;

  if (0 == ff_writeU8BitWord (h, FILESYSTEM_ID))
    return false;

  /* compressed data */
  if (hi->compressed_size != fwrite (hi->compressed_data, sizeof (char), hi->compressed_size, h))
       return false;

  return true;
}

static bool
ff_writeU8BitWord (FILE * h, unsigned char i)
{
  if (EOF == fputc (i, h))
    return false;

  return true;
}

/*
static bool
ff_writeU16BitWord (FILE * h, unsigned int i)
{
  if (EOF == fputc ((i >> 8) & 0xff, h))
    return false;

  if (EOF == fputc (i & 0xff, h))
    return false;

  return true;
}
*/

static bool
ff_writeU32BitWord (FILE * h, unsigned long i)
{
  if (EOF == fputc ((i >> 24) & 0xff, h))
    return false;

  if (EOF == fputc ((i >> 16) & 0xff, h))
    return false;

  if (EOF == fputc ((i >> 8) & 0xff, h))
    return false;

  if (EOF == fputc (i & 0xff, h))
    return false;

  return true;
}
