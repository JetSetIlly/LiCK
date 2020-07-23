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

#ifndef LOCALE_H
#define LOCALE_H


#define LOC_ERROR_FLAG			"*** "

/* lick.c */
#define LOC_BAD_COMMAND			LOC_ERROR_FLAG "unknown command"
#define LOC_BAD_OPTION			LOC_ERROR_FLAG "unknown option"
#define LOC_BAD_COMPRESS		LOC_ERROR_FLAG "unknown compression mode"
#define LOC_NO_ARCHIVE			LOC_ERROR_FLAG "archive not specified"
#define LOC_OUT_OF_MEM			LOC_ERROR_FLAG "out of memory"

#define LOC_AS_NOT_LICK_FILE	LOC_ERROR_FLAG "%s is not a lick archive\n"
#define	LOC_NOT_OPEN_ARC			LOC_ERROR_FLAG "could not open %s\n"
#define LOC_CANT_WRITE_ARC		LOC_ERROR_FLAG "could not write data to archive"

#define LOC_FILE_NOT_EXIST		"%s doesn't exist\n"
#define LOC_FILE_READ_ERR			"error reading %s\n"

#define LOC_PLATFORM_ERROR		LOC_ERROR_FLAG "error in platform specific code"


#endif /* LOCAL_H */
