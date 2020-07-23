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
#include	<limits.h>


static void	bwt_killMatrixMem (char *** matrix, unsigned long matrix_size);


int
bwt_encode (char * input, unsigned long input_size, char ** output, unsigned long * orig_index)
{
	unsigned long	i, j;

	char			** matrix;
	char			* last_column;


#ifdef BWT_DEBUG
puts("\nBWT Encoder\n");
#endif
	
	/* create working matrix and initialize */
	matrix = malloc (input_size * sizeof(char*));
	if ( NULL == matrix )
		return 0;
	for ( i = 0; i < input_size; ++ i )
		matrix[i] = NULL;

	/* allocate memory for each rotation */
	for ( i = 0; i < input_size; ++ i )
	{
		matrix[i] = malloc(input_size * sizeof(char));
		if ( NULL == matrix[i] )
		{
			bwt_killMatrixMem (&matrix, input_size);
			return 0;
		}
	}

	/* create memory for output */
	last_column	= malloc(input_size*sizeof(char));
	if ( NULL == last_column )
	{
		bwt_killMatrixMem (&matrix, input_size);
		return 0;
	}

	/* add data to first entry in the matrix */
	memcpy (matrix[0], input, input_size * sizeof(char));

#ifdef BWT_DEBUG
for (j = 0; j < input_size; ++ j )
	printf ("%3d,", matrix[0][j], matrix[0][j]);
putchar('\n');
#endif

	/* create rotations */
	for ( i = 1; i < input_size; ++ i )
	{
		memcpy (matrix[i], matrix[i-1]+1, (input_size-1)*sizeof(char));
		matrix[i][input_size-1] = matrix[i-1][0];
#ifdef BWT_DEBUG
	for (j = 0; j < input_size; ++ j )
		printf ("%3d,", matrix[i][j], matrix[i][j]);
	putchar('\n');
#endif
	}

	/* sort matrix entries */
	/* !!!!!simple bubble for now!!!!! */
	for ( i = 0; i < input_size-1; ++ i )
	{
		for ( j = i+1; j < input_size; ++j )
		{
			if ( memcmp (matrix[j], matrix[i], input_size * sizeof(char)) < 0 )
			{
				char	* swap;

				swap = matrix[j];
				matrix[j] = matrix[i];
				matrix[i] = swap;
			}
		}
	}

#ifdef BWT_DEBUG
puts("\nSorted Matrix");
for ( i = 0; i < input_size; ++ i )
{
	for (j = 0; j < input_size; ++ j )
		printf ("%3d,", matrix[i][j], matrix[i][j]);
	putchar('\n');
}
#endif

	/* find orignal input in matrix */
	for ( i = 0; i < input_size; ++ i )
		if ( 0 == memcmp (matrix[i], input, input_size * sizeof(char)) )
		{
			*orig_index = i;
			break;
		}

	/* copy last column of matrix to output */
#ifdef BWT_DEBUG
printf("\nL = ");
#endif
	for ( i = 0; i < input_size; ++ i )
	{
		last_column[i] = matrix[i][input_size-1];
#ifdef BWT_DEBUG
printf("%3d,", last_column[i]);
#endif
	}


#ifdef BWT_DEBUG
printf("\nI = %d\n", *orig_index);
#endif

	/* free working memory */
	bwt_killMatrixMem (&matrix, input_size);

	/* point output to last column */
	*output = last_column;

	return 1;
}

static void
bwt_killMatrixMem (char *** matrix, unsigned long matrix_size)
{
	unsigned long	i;

	for ( i = 0; i < matrix_size; ++ i )
		free ((*matrix)[i]);

	free (*matrix);
	*matrix = NULL;
}



int
bwt_decode (char * input, unsigned long input_size, unsigned long orig_index, char ** output)
{
	unsigned long	i, j;

	unsigned long	alphabet[CHAR_MAX+1] = {0};
	unsigned long	*T;
	char			*F;


#ifdef BWT_DEBUG
puts("\nBWT Decoder\n");
#endif

	/* allocate memory for first and last columns */
	F = malloc (input_size * sizeof(char));
	if ( NULL == F )
		return 0;

	/* allocate memory for T (transformation vector) */
	T = malloc (input_size * sizeof(unsigned long));
	if ( NULL == T )
	{
		free (F);
		return 0;
	}

	/* put input into first column */
	for ( i = 0; i < input_size; ++ i )
		F[i] = input[i];

	/* sort first column */
	for ( i = 0; i < input_size; ++ i )
	{
		for ( j = i+1; j < input_size; ++j )
		{
			if ( F[j] < F[i] )
			{
				char	swap;

				swap = F[j];
				F[j] = F[i];
				F[i] = swap;
			}
		}
	}

#ifdef BWT_DEBUG
printf("F = ");
for ( i = 0; i < input_size; ++ i)
	printf("%3d,", F[i]);
puts("");
printf("L = ");
for ( i = 0; i < input_size; ++ i)
	printf("%3d,", input[i]);
puts("");
#endif

	/* calculate T */
	for ( i = 0; i < input_size; ++ i )
	{
		unsigned long	alpha_count = alphabet[input[i]];

		for ( j = 0; j < input_size; ++ j )
		{
			if ( input[i] == F[j] )
			{
				if( alpha_count == 0 )
				{
					T[i] = j;
					++ alphabet[input[i]];
					break;
				}
				else
					-- alpha_count;
			}
		}
	}

#ifdef BWT_DEBUG
printf("T = ");
for ( i = 0; i < input_size; ++ i)
	printf("%3d,", T[i]);
puts("");
#endif

	/* build output. we reuse F for this */
	i = orig_index;
	for ( j = input_size; j > 0 ; -- j )
	{
		F[j-1] = input[i];
		i = T[i];
	}

	/* free working memory */
	free (T);

	/* point output to F */
	*output = F;
	
	return 1;
}

