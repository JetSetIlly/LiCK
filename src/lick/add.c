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

#include	<crc32_lib.h>
#include	<llist_lib.h>

#include  "types_lib.h"
#include	"locale.h"
#include	"fileformat.h"
#include	"platform.h"
#include	"lick.h"


struct addInfo
{
	struct lickOptions *  options;

	FILE *                archive_h;

	unsigned char *       uncompressed_data;
	struct hunkInfo       hunk;

	unsigned long         added;
	unsigned long         errors;
};


static bool add_cleanup (struct addInfo *info, bool ret_val);
static bool add_prepareArchive (struct addInfo *info);
static bool add_eachFile (struct addInfo *info);
static bool add_compressFile (struct addInfo *info);


bool
add (struct lickOptions *options)
{
	struct addInfo info;

	memset (&info, 0, sizeof info);

	info.options = options;

	if (false == add_prepareArchive (&info))
		return add_cleanup (&info, false);

	if (false == add_eachFile (&info))
		return add_cleanup (&info, false);

	return add_cleanup (&info, true);
}


/* clean up info structure */
static bool
add_cleanup (struct addInfo *info, bool ret_val)
{
	if (info->archive_h)
		fclose (info->archive_h);
	free (info->uncompressed_data);
	free (info->hunk.compressed_data);

	return ret_val;
}


/* discover the status of the "archive"
	 return error if it is not a lick file
	 other return types don't matter for adding */
static bool
add_prepareArchive (struct addInfo *info)
{
	ARCHIVE_STATUS s;

	s = ff_checkFile (info->options->archive);
	if (s == AS_NOT_LICK_FILE)
	{
		printf (LOC_AS_NOT_LICK_FILE, info->options->archive);
		return false;
	}

	/* open archive according to archive status */
	if (s == AS_LICK_FILE)
	{
		info->archive_h = fopen (info->options->archive, "ab+");
		if (NULL == info->archive_h)
		{
			printf (LOC_NOT_OPEN_ARC, info->options->archive);
			return false;
		}
	}
	else if (s == AS_NOT_EXIST)
	{
		info->archive_h = fopen (info->options->archive, "wb+");
		if (NULL == info->archive_h)
		{
			printf (LOC_NOT_OPEN_ARC, info->options->archive);
			return false;
		}

		if (false == ff_writeArchiveHead (info->archive_h))
		{
			puts (LOC_CANT_WRITE_ARC);
			return false;
		}
	}
	else
  {
		return false;
  }

	return true;
}


/* step through the nodes in the file
	 and add each file to the archive */
static bool
add_eachFile (struct addInfo *info)
{
	struct lnode *n;

	n = ll_initialiseSearch (info->options->files);
	while (0 == ll_isEndOfList (info->options->files, n))
	{
		info->hunk.file = (char *) ll_returnNodeData (n);

		if (0 == add_compressFile (info))
			return false;

		n = ll_advancePointer (n);

		/* free memory before next iteration */
		free (info->uncompressed_data);
		info->uncompressed_data = NULL;
		free (info->hunk.compressed_data);
		info->hunk.compressed_data = NULL;
	}

	return true;
}


/* Checks for files existance, reads file into memory, checksums file,
	 gets platform specific info and finally compresses the data according
	 to the compression mode selected at the command line */
static bool
add_compressFile (struct addInfo *info)
{
	FILE *h;

	h = fopen (info->hunk.file, "rb+");
	if (NULL == h)
	{
		printf (LOC_FILE_NOT_EXIST, info->hunk.file);
		return true;
	}

	/* find the size of the uncompressed file */
	fseek (h, 0, SEEK_END);
	info->hunk.uncompressed_size = ftell (h);
	rewind (h);

	/* allocate memory for file and read in data */
	info->uncompressed_data = malloc (info->hunk.uncompressed_size * sizeof *info->uncompressed_data);
	if (NULL == info->uncompressed_data)
	{
		puts (LOC_OUT_OF_MEM);
		fclose (h);
		return false;
	}
	if (fread (info->uncompressed_data, sizeof (char), info->hunk.uncompressed_size, h) != info->hunk.uncompressed_size)
	{
		printf (LOC_FILE_READ_ERR, info->hunk.file);
		fclose (h);
		return true;			/* dependent on tolerance level? */
	}
	fclose (h);

	/* generate checksum */
	info->hunk.checksum = crc_generate (info->uncompressed_data, info->hunk.uncompressed_size);

	/* get platform infomation */
	if (0 == pl_getFileInfo (info->hunk.file, &info->hunk.fs_info))
	{
		puts (LOC_PLATFORM_ERROR);
		return false;
	}


	/* compress depending on selected mode */
	info->hunk.mode = info->options->compress_mode;
	if (info->options->compress_mode == CT_NONE)
	{
		/* simple tar like function */
		info->hunk.compressed_size = info->hunk.uncompressed_size;
		info->hunk.compressed_data = malloc (info->hunk.compressed_size * sizeof *info->hunk.compressed_data);
		if (NULL == info->hunk.compressed_data)
		{
			puts (LOC_OUT_OF_MEM);
			fclose (h);
			return false;
		}
		memcpy (info->hunk.compressed_data, info->uncompressed_data, info->hunk.compressed_size);
	}
	else
	{
		/* unsupported compression modes should have been
			 caught during command line parsing */
		puts (LOC_BAD_COMPRESS);
		return false;
	}

	return ff_writeFile (info->archive_h, &info->hunk);
}
