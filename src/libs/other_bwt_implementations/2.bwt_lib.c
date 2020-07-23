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


static int
circular_memcmp (unsigned char *s1, unsigned char *s2, unsigned char *o, unsigned char *e, unsigned long n)
{
	while (n-- > 0)
	{
		if (*s1 != *s2)
			return *s1 - *s2;
		s1++;
		if (s1 > e)
			s1 = o;
		s2++;
		if (s2 > e)
			s2 = o;
	}
	return 0;
}

static void
swap (unsigned char **a, unsigned char **b)
{
	unsigned char *t;

	t = *a;
	*a = *b;
	*b = t;
}

static unsigned long
partition (unsigned char **m, unsigned char *o, unsigned char *e, unsigned long n, unsigned long l, unsigned long r)
{
	/* partition a[l],... a[r] around pivot a[l] */
	/* return index at which pivot ends */

	unsigned char	*pivot = m[l];
	unsigned long	left = l,
								right = r;

	while (left < right)
	{
		/* exchange next pair out of place */
		while ((left <= r) && (circular_memcmp (m[left], pivot, o, e, n) <= 0))
			++left;

		while ((right >= l) && (circular_memcmp (m[right], pivot, o, e, n) > 0))
			--right;

		if (left < right)
			swap (&m[left], &m[right]);
	}

	/* place pivot and return its index */
	swap (&m[l], &m[right]);

	return right;
}

static void
qksort (unsigned char **m, unsigned char *o, unsigned char *e, unsigned long n, unsigned long l, unsigned long r)
{
	unsigned long k;


	if (l >= r)
		return;

	k = partition (m, o, e, n, l, r);

	/* because we are using unsigned values,
		 it is important we check for equality of k an d zero now */
	if (k != 0)
		qksort (m, o, e, n, l, k - 1);

	/* k should never equal ULONG_MAX
		 so it's okay to add one to it */
	qksort (m, o, e, n, k + 1, r);
}

int
bwt_encode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *orig_index)
{
	unsigned long i,
		 j;

	unsigned char **matrix;
	unsigned char *z;

	unsigned char *last_column;


/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nBWT Encoder\n");
#endif
///

puts ("allocating memory");

	/* create working matrix and initialize */
	matrix = malloc (input_size * sizeof *matrix);
	if (NULL == matrix)
		return 0;

	/* create memory for output */
	last_column = malloc (input_size * sizeof *last_column);
	if (NULL == last_column)
	{
		free (matrix);
		return 0;
	}

puts("create rotations");

	/* create rotations */
	for (i = 0; i < input_size; ++i)
		matrix[i] = input + i;

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nUnsorted Matrix");
	for (i = 0; i < input_size; ++i)
	{
		z = matrix[i];
		do
		{
			printf ("%3d,", *z);
			if (z == input + (input_size - 1))
	z = input;
			else
	++z;
		}
		while (z != matrix[i]);

		putchar ('\n');
	}
#endif
///

puts("sort matrix");

	/* sort matrix entries */
	/* !!!!!simple bubble for now!!!!! */
#ifdef BWT_BUBBLE
puts("using bubblesort");
	for (i = 0; i < input_size - 1; ++i)
	{
		for (j = i + 1; j < input_size; ++j)
		{
			if (circular_memcmp (matrix[j], matrix[i], input, input + input_size - 1, input_size * sizeof (unsigned char)) < 0)
	   swap (&matrix[j], &matrix[i]);
		}
	}
#else
puts("using quicksort");
	qksort (matrix, input, input + input_size - 1, input_size, 0, input_size - 1);
#endif

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nSorted Matrix");
	for (i = 0; i < input_size; ++i)
	{
		z = matrix[i];
		do
		{
			printf ("%3d,", *z);
			if (z == input + (input_size - 1))
	z = input;
			else
	++z;
		}
		while (z != matrix[i]);

		putchar ('\n');
	}
#endif
///

puts("find original input in matrix");

	/* find orignal input in matrix */
	for (i = 0; i < input_size; ++i)
	{
		if (0 == circular_memcmp (matrix[i], input, input, input + input_size - 1, input_size * sizeof (unsigned char)))
		{
			*orig_index = i;
			break;
		}
	}

/// DEBUG CODE
#ifdef BWT_DEBUG
	printf ("\nI = %d\n", *orig_index);

	/* copy last column of matrix to output */
	printf ("\nL = ");
#endif
///

puts("last column");

	for (i = 0; i < input_size; ++i)
	{
		if (i != *orig_index)
			last_column[i] = *(matrix[i] - 1);
		else
			last_column[i] = *(input + (input_size - 1));

/// DEBUG CODE
#ifdef BWT_DEBUG
		printf ("%3d,", last_column[i]);
#endif
///
	}
/// DEBUG CODE
#ifdef BWT_DEBUG
	putchar ('\n');
#endif
///

	/* free working memory */
	free (matrix);

	/* point output to last column */
	*output = last_column;

puts("done");

	return 1;
}


int
bwt_decode (unsigned char *input, unsigned long input_size, unsigned long orig_index, unsigned char **output)
{
	unsigned long i,
		 j;

	unsigned long alphabet[CHAR_MAX + 1] =
	{0};
	unsigned long *T;
	unsigned char *F;

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nBWT Decoder\n");
#endif
///

	/* allocate memory for first column */
	F = malloc (input_size * sizeof *F);
	if (NULL == F)
		return 0;

	/* allocate memory for T (transformation vector) */
	T = malloc (input_size * sizeof *T);
	if (NULL == T)
	{
		free (F);
		return 0;
	}

	/* put input into first column */
	for (i = 0; i < input_size; ++i)
		F[i] = input[i];

	/* sort first column */
	for (i = 0; i < input_size; ++i)
	{
		for (j = i + 1; j < input_size; ++j)
		{
			if (F[j] < F[i])
			{
				unsigned char swap;

				swap = F[j];
				F[j] = F[i];
				F[i] = swap;
			}
		}
	}

/// DEBUG CODE
#ifdef BWT_DEBUG
	printf ("F = ");
	for (i = 0; i < input_size; ++i)
		printf ("%3d,", F[i]);
	puts ("");
	printf ("L = ");
	for (i = 0; i < input_size; ++i)
		printf ("%3d,", input[i]);
	puts ("");
#endif
///

	/* calculate T */
	for (i = 0; i < input_size; ++i)
	{
		unsigned long alpha_count = alphabet[input[i]];

		for (j = 0; j < input_size; ++j)
		{
			if (input[i] == F[j])
			{
				if (alpha_count == 0)
				{
					T[i] = j;
					++alphabet[input[i]];
					break;
				}
				else
					--alpha_count;
			}
		}
	}

/// DEBUG CODE
#ifdef BWT_DEBUG
	printf ("T = ");
	for (i = 0; i < input_size; ++i)
		printf ("%3d,", T[i]);
	puts ("");
#endif
///

	/* build output. we reuse F for this */
	i = orig_index;
	for (j = input_size; j > 0; --j)
	{
		F[j - 1] = input[i];
		i = T[i];
	}

	/* free working memory */
	free (T);

	/* point output to F */
	*output = F;

	return 1;
}
