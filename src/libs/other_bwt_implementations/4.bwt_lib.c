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

#include	<stdio.h>

#include	<stdlib.h>
#include	<string.h>
#include	<limits.h>


/* use input barrel instead of circular memory */
#define BWT_ENCODER_INPUT_BARREL 1

/* sorting algorithm to use for
encoder -- defaults to radix sort */
//#define BWT_SHELL 1
//#define BWT_QUICK 1
//#define	BWT_MULTI_QUICK 1

/* print out some basic debugging info */
//#define BWT_DEBUG 1

/* dump relevent data during encoding/decoding */
//#define BWT_DEBUG_DUMP 1

/* limit the amount of data output by BWT_DEBUG_DUMP */
//#define BWT_DEBUG_LIMIT 3

/*
 * non input barrel version of multikey quick sort has
 * not been coded/tested so force use of input barrel
 */
#ifndef BWT_MULTI_QUICK
#define ENCODER_INPUT BARREL 1
#endif

/*
 * each encoded block starts with a preamble 4 bytes indicating the
 * location of the original index. BWT_HEADERLEN defines this size.
 */
#define BWT_HEADERLEN	 4



/// SUPPORT FUNCTIONS
static int
bwt_memcmp (unsigned char *s1, unsigned char *s2, unsigned char *o, unsigned char *e, unsigned long n)
{
#ifdef BWT_ENCODER_INPUT_BARREL
	return memcmp (s1, s2, n);
#else
	while (n-- > 0)
	{
		if (*s1 != *s2)
			return *s1 - *s2;

		++s1;
		if (s1 > e)
			s1 = o;

		++s2;
		if (s2 > e)
			s2 = o;
	}
	return 0;
#endif
}

#define swap(a,b)	{unsigned char *t; t = *a; *a = *b; *b = t;}
///

/// SHELL SORT
static void
shellsort (unsigned char **matrix, unsigned long range, unsigned long len, unsigned char *o, unsigned char *e)
{
	int     i,
	        j,
	        h,
	        k,
					incs[16] = { 1391376, 463792, 198768, 86961, 33936, 13776,
		4592, 1968, 861, 336, 112, 48, 21, 7, 3, 1
	};

	unsigned char *v;


	for (k = 0; k < 16; ++ k)
	{
		for (h = incs[k], i = h; i < range; ++i)
		{
			v = matrix[i];
			j = i;

			while ((j >= h) && (bwt_memcmp (matrix[j - h], v, o, e, len) > 0))
			{
				matrix[j] = matrix[j - h];
				j -= h;
			}
			matrix[j] = v;
		}
	}
}
///

/// QUICKSORT ROUTINES
#ifdef BWT_QUICK
static unsigned long
partition (unsigned char **matrix, unsigned long n, unsigned char *o, unsigned char *e, unsigned long l, unsigned long r)
{
	/*
	 * partition a[l],... a[r] around pivot a[l] 
	 */
	/*
	 * return index at which pivot ends 
	 */

	unsigned char *pivot = matrix[l];
	unsigned long left = l,
								right = r;

	while (left < right)
	{
		/*
		 * exchange next pair out of place 
		 */
		while ((left <= r) && (bwt_memcmp (matrix[left], pivot, o, e, n) <= 0))
			++left;

		while ((right >= l) && (bwt_memcmp (matrix[right], pivot, o, e, n) > 0))
			--right;

		if (left < right)
			swap (&matrix[left], &matrix[right]);
	}

	/*
	 * place pivot and return its index 
	 */
	swap (&matrix[l], &matrix[right]);

	return right;
}

static void
qksort (unsigned char **matrix, unsigned long n, unsigned long len, unsigned char *o, unsigned char *e, unsigned long l, unsigned long r)
{
	unsigned long k,
								i;
	unsigned char *v;


	if (l >= r)
		return;

	/*
	 * check to see if this partition is already sorted 
	 */
	for (i = l + 1, v = matrix[l]; i <= r; ++i)
	{
		if (bwt_memcmp (matrix[i], v, o, e, n) < 0)
			break;

		v = matrix[i];
	}
	if (i > r)
		return;

	/*
	 * if the pile is small then
	 * do a shell sort 
	 */
#ifdef BWT_ENCODER_INPUT_BARREL
	if (r - l <= 20)
	{
		shellsort (matrix + l, (r - l + 1), n, o, e);
		return;
	}
#else
	if (l < r && r - l <= 20)
	{
		shellsort (matrix + l, (r - l + 1), n, o, e);
		return;
	}
	else
	if (r < l && (e - l + r - o) <= 20)
	{
		shellsort (matrix + l, (e - l + r - o + 1), n, o, e);
		return;
	}
#endif /* BWT_ENCODER_INPUT_BARREL */

	k = partition (matrix, n, o, e, l, r);

	/*
	 * because we are using unsigned values, it is
	 * important we check for equality of k and zero now 
	 */
	if (k != 0)
		qksort (matrix, n, len, o, e, l, k - 1);

	/*
	 * k should never equal ULONG_MAX
	 * so it's okay to add one to it 
	 */
	qksort (matrix, n, len, o, e, k + 1, r);
}
#endif /* BWT_QUICK */
///

/// MULTIKEY QUICKSORT
#ifdef BWT_MULTI_QUICK

#define mkq_swap(a,b)	{unsigned char *t = x[a]; x[a]=x[b]; x[b] = t;}
#define i2c(i)	x[i][depth]
#ifndef min
#define min(a, b) ((a)<=(b) ? (a) : (b))
#endif

static void
vecswap (int i, int j, int n, unsigned char **x)
{
	while (n -- > 0)
	{
		mkq_swap(i, j);
		++ i;
		++ j;
	}
}

static void
multiqksort (unsigned char **x, unsigned long n, unsigned long len, unsigned char *o, unsigned char *e, int depth)
{
	int     a,
	        b,
	        c,
	        d,
	        r,
	        v;

	if (n <= 1)
		return;

	if ( n <= 16 )
	{
		shellsort (x, n, len, o, e);
		return;
	}

	a = rand () % n;
	mkq_swap (0, a);
	v = i2c (0);
	a = b = 1;
	c = d = n - 1;
	for (;;)
	{
		while (b <= c && (r = i2c (b) - v) <= 0)
		{
			if (r == 0)
			{
				mkq_swap (a, b);
				++ a;
			}
			++ b;
		}
		while (b <= c && (r = i2c (c) - v) >= 0)
		{
			if (r == 0)
			{
				mkq_swap (c, d);
				-- d;
			}
			-- c;
		}
		if (b > c)
			break;
		mkq_swap (b, c);
		++ b;
		-- c;
	}
	r = min (a, b - a);
	vecswap (0, b - r, r, x);
	r = min (d - c, n - d - 1);
	vecswap (b, n - r, r, x);
	r = b - a;
	multiqksort (x, r, len, o, e, depth);
	if (i2c (r) != 0)
		multiqksort (x + r, a + n - d - 1, len, o, e, depth + 1);
	r = d - c;
	multiqksort (x + n - r, r, len, o, e, depth);
}
#endif /* BWT_MULTI_QUICK */
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
	unsigned char **sa;
	unsigned long sn;
	unsigned long si;
};

#define rs_push(a,n,i) sp->sa = a, sp->sn = n, (sp++)->si = i
#define rs_pop(a,n,i)	 a = (--sp)->sa, n = sp->sn, i = sp->si
#define rs_empty()		 (sp <= stack)

static void
radixsort (unsigned char **a, unsigned long n, unsigned char *o, unsigned char *e)
{
	struct rs_stack stack[RS_SIZE],
									*sp = stack;

	unsigned char **pile[UCHAR_MAX+1],
								**an,										/* current string */
								**ak,										/* counts from a to an */
								 *r;

	int   c,
				cmax,
				*cp,
				b = 0;

	static int	count[UCHAR_MAX+1],
							cmin,
							nc = 0;

	unsigned long len = n;		/* used for shell sort -- n is the number of "rows" in the matrix
														to be sorted whereas len is the number of "columns" */


	rs_push (a, n, b);

	while (!rs_empty ())
	{
		rs_pop (a, n, b);

		/*
		 * use shellsort if pile is small 
		 */
		if (n < RS_THRESHOLD)
		{
			shellsort (a, n, len, o, e);
			continue;									/* while loop */
		}

		/*
		 * current string in array
		 */
		an = a + n;

		/*
		 * tally
		 */
		cmin = UCHAR_MAX;
		for (ak = a; ak < an; )
		{
			/*
			 * get next char -- account for circular
			 * memory if it's being used
			 */
#ifdef BWT_ENCODER_INPUT_BARREL
			c = *((*ak++) + b);
#else
			unsigned char *check = (*ak++) + b;

			if (check > e)
				c = *(o + (check - e) - 1);
			else
				c = *check;
#endif

			/* increase count for current character */
			if (1 == ++ count[c] && c > 0)
			{
				if (c < cmin)
					cmin = c;
				++ nc;
			}
		}

		/*
		 * find places
		 */
		pile[0] = ak = a + count[cmax = 0];
		for (cp = count + cmin; nc > 0; ++ cp, -- nc)
		{
			while (0 == *cp)
				++ cp;
			if (*cp > 1)
				rs_push (ak, *cp, b + 1);
			pile[cmax = cp - count] = ak += *cp;
		}

		/*
		 * permute in place 
		 */
		an -= count[cmax];
		count[cmax] = 0;
		for (ak = a; ak < an; ak += count[c], count[c] = 0)
		{
			r = *ak;

#ifdef BWT_ENCODER_INPUT_BARREL
			while ( --pile[c = r[b]] > ak )
				swap (&*pile[c], &r);
#else
			for (;;)
			{
				unsigned char *check = r + b;

				if (check > e)
					c = *(o + (check - e) - 1);
				else
					c = *check;

				if (-- pile[c] <= ak)
					break;

				swap (&*pile[c], &r);
			}
#endif

			*ak = r;
		}
	}
}
///


/*
	a	--> array of "strings"
	n	--> number of strings
	o	--> origin point of master string
	e --> end point of master string
*/
static void
sortMatrix (unsigned char **matrix, unsigned long len, unsigned char *o, unsigned char *e)
{
#ifdef BWT_SHELL
/// DEBUG CODE
#ifdef BWT_DEBUG
	puts("shellsort!");
#endif
///
	shellsort (matrix, len, len, o, e);
#elif BWT_QUICK
/// DEBUG CODE
#ifdef BWT_DEBUG
	puts("quicksort!");
#endif
///
	qksort (matrix, len, len, o, e, 0, len - 1);
#elif BWT_MULTI_QUICK
/// DEBUG CODE
#ifdef BWT_DEBUG
	puts("multikey quicksort!");
#endif
///
	multiqksort (matrix, len, len, o, e, 0);
#else
/// DEBUG CODE
#ifdef BWT_DEBUG
	puts("radixsort!");
#endif
///
	radixsort (matrix, len, o, e);
#endif
}

int
bwt_encode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size)
{
	unsigned long i,
								orig_index;

	unsigned char **matrix;

	unsigned char *input_barrel;


/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nBWT Encoder\n");
	puts ("allocating memory");
#endif
///

	/*
	 * allocate memory for input barrel 
	 */
#ifdef BWT_ENCODER_INPUT_BARREL
/// DEBUG CODE
#ifdef BWT_DEBUG
	puts("using input barrel");
#endif
///
	input_barrel = malloc (2 * input_size * sizeof *input_barrel);
	if (NULL == input_barrel)
		return 0;
#endif

	/*
	 * create working matrix and initialize 
	 */
	matrix = malloc (input_size * sizeof *matrix);
	if (NULL == matrix)
	{
		free (input_barrel);
		return 0;
	}

	/*
	 * create memory for output 
	 */
	*output = malloc ((BWT_HEADERLEN + input_size) * sizeof **output);
	if (NULL == *output)
	{
		free (matrix);
		free (input_barrel);
		return 0;
	}
	*output_size = ((BWT_HEADERLEN + input_size) * sizeof **output);

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("create rotations");
#endif
///

#ifdef BWT_ENCODER_INPUT_BARREL
	/*
	 * populate input barrel 
	 */
	for (i = 0; i < input_size; ++i)
		input_barrel[i + input_size] = input_barrel[i] = input[i];
#else
	input_barrel = input;
#endif

	/*
	 * create rotations 
	 */
	for (i = 0; i < input_size; ++i)
		matrix[i] = input_barrel + i;

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("sort matrix");
#endif

#ifdef BWT_DEBUG_DUMP
	puts ("\nUnsorted Matrix");
#ifdef BWT_DEBUG_LIMIT
	printf ("(output limited to %d chars)\n", BWT_DEBUG_LIMIT);
#endif
	for (i = 0; i < input_size; ++i)
	{
		unsigned char	* z;
#ifdef BWT_DEBUG_LIMIT
		unsigned long debug_limit = BWT_DEBUG_LIMIT;
#endif
		z = matrix[i];
		do
		{
			printf ("%3d,", *z);
			if (z == input_barrel + (input_size - 1))
				z = input_barrel;
			else
				++ z;
		}
		while (z != matrix[i]
#ifdef BWT_DEBUG_LIMIT
		&& -- debug_limit > 0
#endif
		);

		putchar ('\n');
	}
#endif
///

	/*
	 * sort matrix entries 
	 */
	sortMatrix (matrix, input_size, input_barrel, input_barrel + input_size - 1);

/// DEBUG CODE
#ifdef BWT_DEBUG_DUMP
	puts ("\nSorted Matrix");
#ifdef BWT_DEBUG_LIMIT
	printf ("(output limited to %d chars)\n", BWT_DEBUG_LIMIT);
#endif
	for (i = 0; i < input_size; ++i)
	{
		unsigned char	* z;
		unsigned long	j;

#ifdef BWT_DEBUG_LIMIT
#ifdef BWT_ENCODER_INPUT_BARREL
		unsigned long debug_limit = BWT_DEBUG_LIMIT<input_size?BWT_DEBUG_LIMIT:input_size;
#else
		unsigned long debug_limit = BWT_DEBUG_LIMIT;
#endif
#else
		unsigned long debug_limit = input_size;
#endif

		z = matrix[i];

#ifdef BWT_ENCODER_INPUT_BARREL
		for (j = 0; j < debug_limit; ++j)
		{
			printf ("%3d,", *z);
			++z;
		}
#else
		do
		{
			printf ("%3d,", *z);
			if (z == input_barrel + (input_size - 1))
				z = input_barrel;
			else
				++z;
		}
		while (z != matrix[i]
#ifdef BWT_DEBUG_LIMIT
		&& -- debug_limit > 0
#endif
		);
#endif

		putchar ('\n');
	}
#endif

#ifdef BWT_DEBUG
	puts ("find original input in matrix");
#endif
///

	/*
	 * find orignal input in matrix 
	 */
	for (orig_index = 0; orig_index < input_size; ++(orig_index))
	{
		if (0 == bwt_memcmp (matrix[orig_index], input_barrel, input_barrel, input_barrel + input_size - 1, input_size * sizeof *input))
		{
			/*
			 * first 4 bytes of output indicate location of original index
			 */
			(*output)[0] = (orig_index >> 24) & 0xff;
			(*output)[1] = (orig_index >> 16) & 0xff;
			(*output)[2] = (orig_index >> 8) & 0xff;
			(*output)[3] = orig_index & 0xff;
			break;
		}
	}

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("last column");
#endif

#ifdef BWT_DEBUG_DUMP
	printf ("\nI = %ld\n", orig_index);

	/*
	 * copy last column of matrix to output 
	 */
	printf ("\nL = ");
#endif
///

	/* copy last column to output */
	for (i = 0; i < input_size; ++i)
	{
#ifdef BWT_ENCODER_INPUT_BARREL
		(*output)[i + BWT_HEADERLEN] = *((matrix[i])+input_size-1);
#else
		if (i != orig_index)
			(*output)[i + BWT_HEADERLEN] = *(matrix[i] - 1);
		else
			(*output)[i + BWT_HEADERLEN] = *(input + (input_size - 1));
#endif
/// DEBUG CODE
#ifdef BWT_DEBUG_DUMP
		printf ("%3d,", (*output)[i+BWT_HEADERLEN]);
#endif
///
	}
/// DEBUG CODE
#ifdef BWT_DEBUG_DUMP
	putchar ('\n');
#endif
///

	/*
	 * free working memory 
	 */
	free (matrix);
#ifdef BWT_ENCODER_INPUT_BARREL
	free (input_barrel);
#endif


/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("done");
#endif
///

	return 1;
}


int
bwt_decode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size)
{
	unsigned long	i, j, sum, orig_index;
	
	unsigned long C[UCHAR_MAX + 1] = {0};
	unsigned long	*P;

/// DEBUG CODE
#ifdef BWT_DEBUG
	puts ("\nBWT Decoder\n");
#endif
///

	/* parse header */
	orig_index = input[0] << 24;
	orig_index |= input[1] << 16;
	orig_index |= input[2] << 8;
	orig_index |= input[3];

	/* point input to block proper */
	input += BWT_HEADERLEN;
	input_size -= BWT_HEADERLEN;

	P = malloc (input_size * sizeof *P);
	if ( NULL == P )
		return 0;

	*output	= malloc (input_size * sizeof **output);
	if ( NULL == *output )
	{
		free (P);
		return 0;
	}

	for ( i = 0; i < input_size; ++ i )
	{
		P[i] = C[input[i]];
		C[input[i]]	++;
	}

	sum = 0;
	for ( i = 0; i < UCHAR_MAX+1; ++ i )
	{
		sum += C[i];
		C[i] = sum - C[i];
	}

	i = orig_index;
	/* stop at j = 1 as j is unsigned
	and comparison to zero would be wrong */
	for ( j = input_size - 1; j > 0; -- j )
	{
		(*output)[j] = input[i];
		i = P[i] + C[input[i]];
	}

	/* do element zero  */
	(*output)[0] = input[i];
	*output_size = input_size;

	free (P);

	return 1;
}

