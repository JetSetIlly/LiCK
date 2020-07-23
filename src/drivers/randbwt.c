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
#include	<time.h>
#include	<limits.h>

#include	<bwt_lib.h>


void
dumpdata (char * filename, unsigned char * data, unsigned long data_size)
{
	FILE	* f;

	f = fopen (filename, "wb+");
	if ( NULL == f )
	{
		puts ("*** couldn't open output file");
		return;
	}
	
	if ( data_size != fwrite (data, sizeof *data, data_size, f) )
		puts ("*** error writing to output file");

	fclose (f);
}

#define ONESHOT 0
#define	DEPTH	 8192

int
main (void)
{
	unsigned char	*orig,
								*encoded,
								*decoded;

	unsigned long	encoded_size,
								decoded_size;

	unsigned long	count = 0;

	unsigned long i;




	orig = malloc (DEPTH * sizeof *orig);
	if ( NULL == orig )
	{
		puts("*** out of memory");
		return EXIT_FAILURE;
	}

	srand(time(NULL));

	for (;;)
	{
		++ count;

		if ( count % 100 == 0 )
		{
			printf ("%ld\r", count);
			fflush (stdout);
		}

		for ( i = 0; i < DEPTH; ++ i )
			*(orig+i) = (rand() % UCHAR_MAX);

		if ( 0 == bwt_encode (orig, DEPTH, &encoded, &encoded_size) )
		{
			puts("*** out of memory during encode");
			free (orig);
			return EXIT_FAILURE;
		}

		if ( 0 == bwt_decode (encoded, encoded_size, &decoded, &decoded_size) )
		{
			puts("*** out of memory during decode");
			free (orig);
			free (encoded);
			return EXIT_FAILURE;
		}

		if ( 1 == ONESHOT ) {
			dumpdata ("lick_bwt_rand.dump_orig", orig, DEPTH);
			dumpdata ("lick_bwt_rand.dump_encoding", encoded, encoded_size);
			dumpdata ("lick_bwt_rand.dump_decoding", decoded, decoded_size);
			free (encoded);
			free (decoded);
			break;
		}

		if ( decoded_size != DEPTH )
		{
			puts ("decoded size is different to original");
			dumpdata ("lick_bwt_rand.dump_orig", orig, DEPTH);
			dumpdata ("lick_bwt_rand.dump_encoding", encoded, encoded_size);
			dumpdata ("lick_bwt_rand.dump_decoding", decoded, decoded_size);
			free (encoded);
			free (decoded);
			break;
		}

		if ( 0 != memcmp (orig, decoded, DEPTH) )
		{
			puts ("original orig and encoded/decoded data differ");
			dumpdata ("lick_bwt_rand.dump_orig", orig, DEPTH);
			dumpdata ("lick_bwt_rand.dump_encoding", encoded, encoded_size);
			dumpdata ("lick_bwt_rand.dump_decoding", decoded, decoded_size);
			free (encoded);
			free (decoded);
			break;
		}

		free (encoded);
		encoded = NULL;
		free (decoded);
		decoded = NULL;

		if ( ULONG_MAX == count )
		{
			puts("iteration counter maxed out");
			break;
		}
	}

	if ( 1 == ONESHOT ) {
		printf("one iteration - debugging files saved\n");
	} else {
		printf("iterations = %ld\n", count);
	}

	free (orig);

	return EXIT_SUCCESS;
}

