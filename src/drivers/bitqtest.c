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
#include	<limits.h>

#include	<types_lib.h>
#include	<bitq_lib.h>


#define STREAM_SIZE	512
#define	OUTF_NAME		"BITQ_TEST"

int
main (void)
{
	struct bitqStream	* bq;

	unsigned char	* stream;	
	unsigned long	size;
	unsigned long	len = 0;

	int	ret;

	FILE	* outf;


	size = STREAM_SIZE;
	stream = malloc (size);
	if ( NULL == stream )
	{
		puts ("*** couldn't allocate memory");
		return EXIT_FAILURE;
	}

	bq = bitq_newStream (stream, size, &len);
	if ( NULL == bq )
	{
		puts ("*** couldn't create new bit queue");
		free (stream);
		return EXIT_FAILURE;
	}

	ret = bitq_writeStream (bq, 1, 33);
	if ( BITQ_CONTINUE != ret )
	{
		puts ("*** error writing stream");
		free (stream);
		bitq_freeStream (bq);
		return EXIT_FAILURE;
	}

	ret = bitq_flushWriteStream(bq);
	if ( BITQ_CONTINUE != ret )
	{
		puts ("*** error writing stream");
		free (stream);
		bitq_freeStream (bq);
		return EXIT_FAILURE;
	}

	outf = fopen (OUTF_NAME, "w+");
	if ( NULL == outf )
	{
		puts ("*** error opening output file");
		free (stream);
		bitq_freeStream (bq);
		return EXIT_FAILURE;
	}

	if ( len != fwrite (stream, sizeof *stream, len, outf) )
	{
		puts ("*** error writing to file");
		fclose (outf);
		free (stream);
		bitq_freeStream (bq);
		return EXIT_FAILURE;

	}

	fclose (outf);
	free (stream);
	bitq_freeStream (bq);

	return EXIT_SUCCESS;
}
