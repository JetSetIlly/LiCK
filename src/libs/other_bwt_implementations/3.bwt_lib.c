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


#define INPUT_BARREL 1

//#define BWT_SHELL 1
//#define	BWT_QUICK	1
//#define BWT_DEBUG 1


/// SUPPORT FUNCTIONS
static int
bwt_memcmp (unsigned char *s1, unsigned char *s2, unsigned char *o, unsigned char *e, unsigned long n)
{
#ifdef INPUT_BARREL
	return memcmp (s1, s2, n);
#else
	while (n -- > 0)
	{
		if (*s1 != *s2)
			return *s1 - *s2;

		++ s1;
		if (s1 > e)
			s1 = o;

		++ s2;
		if (s2 > e)
			s2 = o;
	}
	return 0;
#endif
}

static void
swap (unsigned char **a, unsigned char **b)
{
	unsigned char *t;

	t = *a;
	*a = *b;
	*b = t;
}
///

/// SHELL SORT
inline static void
shellsort (unsigned char ** a, unsigned long n, unsigned char * o, unsigned char * e, unsigned long range)
{
	int	i,
			j,
			h,
			k,
			incs[16] = { 1391376, 463792, 198768, 86961, 33936, 13776,
								4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1 };

	unsigned char * v;


	for ( k = 0; k < 16; ++ k )
	{
		for ( h = incs[k], i = h; i < range; ++ i )
		{
			v = a[i];
			j = i;

			while ( (j >= h) && (bwt_memcmp(a[j-h], v, o, e, n) > 0) )
			{
				a[j] = a[j-h];
				j -= h;
			}
			a[j] = v;
		}
	}
}
///

/// QUICKSORT ROUTINES
#ifdef BWT_QUICK

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
		while ((left <= r) && (bwt_memcmp (m[left], pivot, o, e, n) <= 0))
			++ left;

		while ((right >= l) && (bwt_memcmp (m[right], pivot, o, e, n) > 0))
			-- right;

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
	unsigned long k,
								i;
	unsigned char	* v;


	if (l >= r)
		return;

	/* check to see if this partition is already sorted */
	for ( i = l+1, v = m[l]; i <= r; ++ i )
	{
		if ( bwt_memcmp (m[i], v, o, e, n) < 0 )
			break;

		v = m[i];
	}
	if ( i > r )
		return;

	/* if the pile is small then
	do a shell sort */
	if ( l < r && r-l <= 20 )
	{
		shellsort (m+l, n, o, e, r-l+1);
		return;
	}
	else
	if ( r < l && (e-l+r-o) <= 20 )
	{
		shellsort (m+l, n, o, e, e-l+r-o+1);
		return;
	}

	k = partition (m, o, e, n, l, r);

	/* because we are using unsigned values, it is
	important we check for equality of k and zero now */
	if (k != 0)
		qksort (m, o, e, n, l, k - 1);

	/* k should never equal ULONG_MAX
		 so it's okay to add one to it */
	qksort (m, o, e, n, k + 1, r);
}
#endif /* BWT_QUICK */
///

/// RADIX SORT ROUTINES

/*
American Flag algorithm - McIlroy, Bostic implemenation
-- minor alterations to support circular memory
*/

#define RS_SIZE				510
#define RS_THRESHOLD	16

struct rs_stack
{
	unsigned char ** sa;
	unsigned long	sn;
	int						si;
};

#define rs_push(a,n,i) sp->sa = a, sp->sn = n, (sp++)->si = i
#define rs_pop(a,n,i)	 a = (--sp)->sa, n = sp->sn, i = sp->si
#define rs_empty()		 (sp <= stack)
#define rs_swap(p,q,r) r = p, p = q, q = r;

static void
radixsort (unsigned char ** a, unsigned long n, unsigned char * o, unsigned char * e, int b)
{
	struct rs_stack		stack[RS_SIZE],
										*sp = stack;

	unsigned char	** pile[256],
								** an,	/* current string */
								** ak,	/* counts from a to an*/
								* r,
								* t;		/* swap variable */

	static int	count[256],
							cmin,
							nc;

	int		* cp,
				c,
				cmax;

	unsigned long	a_len = n;


	rs_push (a, n, b);

	while ( !rs_empty() )
	{
		rs_pop (a, n, b);

		/* use shellsort if pile is small */
		if ( n < RS_THRESHOLD )
		{
			shellsort (a, a_len, o, e, n);
			continue; /* while loop */
		}

		/* currnt string in array */
		an = a + n;

		cmin = 255;
		for ( ak = a; ak < an; ++ ak)
		{
#ifdef INPUT_BARREL
			unsigned char	* check = (*ak)+b;

			if ( check > e )
				c = *(o+(check-e)-1);
			else
				c = *check;
#else
			c = (*ak)+b;
#endif

			if ( 1 == ++ count[c] && c > 0 )
			{
				if ( c < cmin )
					cmin = c;
				++ nc;
			}
		}

		pile[0] = ak = a + count[cmax=0];
		for ( cp = count + cmin; nc > 0; ++ cp, -- nc )
		{
			while ( 0 == *cp )
				++ cp;
			if ( *cp > 1)
				rs_push (ak, *cp, b+1);
			pile[cmax = cp - count] = ak += *cp;
		}


		/* permute in place */
		an -= count[cmax];
		count[cmax] = 0;
		for (ak = a; ak < an; ak += count[c], count[c] = 0)
		{
			r = *ak;

			for (;;)
			{
#ifdef INPUT_BARREL
				unsigned char	* check = r+b;

				if ( check > e )
					c = *(o+(check-e)-1);
				else
					c = *check;
#else
				c = r+b;
#endif

				if ( -- pile[c] <= ak )
					break;

				rs_swap (*pile[c], r, t);
			}

			*ak = r;
		}
	}
}
///


/*
	a	--> array of "strings"
	n	--> number of "strings"
	o	--> origin point of master string
	e --> end point of master string
*/
static void
sortMatrix (unsigned char ** a, unsigned long n, unsigned char * o, unsigned char * e)
{
#ifdef BWT_SHELL
	shellsort (a, n, o, e, n);
#elif BWT_QUICK
	qksort (a, o, e, n, 0, n-1);
#else
	radixsort (a, n, o, e, 0);
#endif
}

int
bwt_encode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *orig_index)
{
	unsigned long i,
								j;

	unsigned char **matrix;
	unsigned char *z;

	unsigned char *input_barrel,
								*last_column;

/// DEBUG CODE
#ifdef BWT_DEBUG_BRIEF
puts ("\nBWT Encoder\n");
	puts ("allocating memory");
#endif
///

	/* allocate memory for input barrel */
#ifdef INPUT_BARREL
	input_barrel = malloc (2* input_size * sizeof *input_barrel);
	if (NULL == input_barrel)
		return 0;
#endif

	/* create working matrix and initialize */
	matrix = malloc (input_size * sizeof *matrix);
	if (NULL == matrix)
	{
		free (input_barrel);
		return 0;
	}

	/* create memory for output */
	last_column = malloc (input_size * sizeof *last_column);
	if (NULL == last_column)
	{
		free (matrix);
		free (input_barrel);
		return 0;
	}

/// DEBUG CODE
#ifdef BWT_DEBUG_BRIEF
	puts("create rotations");
#endif
///

#ifdef INPUT_BARREL
	/* populate input barrel */
	for (i = 0; i < input_size; ++ i)
		input_barrel[i+input_size] = input_barrel[i] = input[i];
#else
	input_barrel = input;
#endif

	/* create rotations */
	for (i = 0; i < input_size; ++ i)
		matrix[i] = input_barrel + i;

/// DEBUG CODE
#ifdef BWT_DEBUG_BRIEF
puts("sort matrix");
#endif

#ifdef BWT_DEBUG
	puts ("\nUnsorted Matrix");
	for (i = 0; i < input_size; ++i)
	{
		z = matrix[i];
		do
		{
			printf ("%3d,", *z);
			if (z == input_barrel + (input_size - 1))
				z = input_barrel;
			else
				++z;
		}
		while (z != matrix[i]);

		putchar ('\n');
	}
#endif
///

	/* sort matrix entries */
	sortMatrix (matrix, input_size, input_barrel, input_barrel + input_size - 1);

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nSorted Matrix");
	for (i = 0; i < input_size; ++i)
	{
		z = matrix[i];

#ifdef INPUT_BARREL
		for ( j = 0; j < input_size; ++ j )
		{
			printf ("%3d,", *z);
			++ z;
		}
#else
		do
		{
			printf ("%3d,", *z);
			if (z == input_barrel + (input_size - 1))
				z = input_barrel;
			else
				++ z;
		}
		while (z != matrix[i]);
#endif

		putchar ('\n');
	}
#endif

#ifdef BWT_DEBUG_BRIEF
	puts("find original input in matrix");
#endif
///

	/* find orignal input in matrix */
	for (i = 0; i < input_size; ++i)
	{
		if (0 == bwt_memcmp (matrix[i], input_barrel, input_barrel, input_barrel + input_size - 1, input_size * sizeof *input))
		{
			*orig_index = i;
			break;
		}
	}

/// DEBUG CODE
#ifdef BWT_DEBUG_BRIEF
	puts("last column");
#endif

#ifdef BWT_DEBUG
	printf ("\nI = %d\n", *orig_index);

	/* copy last column of matrix to output */
	printf ("\nL = ");
#endif
///

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
#ifdef INPUT_BARREL
	free (input_barrel);
#endif

	/* point output to last column */
	*output = last_column;

/// DEBUG CODE
#ifdef BWT_DEBUG_BRIEF
	puts("done");
#endif
///

	return 1;
}


int
bwt_decode (unsigned char *input, unsigned long input_size, unsigned long orig_index, unsigned char **output)
{
	unsigned long i,
								j;

	unsigned long alphabet[CHAR_MAX + 1] = {0};
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

