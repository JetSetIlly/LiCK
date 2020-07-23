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

/* include assertions */
//#define BWT_ASSERTIONS 1

/* print out debugging signposts */
//#define BWT_DEBUG 1

/* matrix digest during encoding/decoding -- requires BWT_DEBUG */
#define BWT_PRINT_MATRIX 1
#define BWT_PRINT_MATRIX_SIZE 5

/* use input barrel instead of circular memory */
#define BWT_ENCODER_INPUT_BARREL 1

/* sorting algorithm to use for encoder -- defaults to shell sort */
//#define BWT_RADIX 1
//#define BWT_QUICK 1
#define	BWT_MULTI_QUICK 1

/*** END OF user definable sections ***/

#include	<stdio.h>

#include	<stdlib.h>
#include	<string.h>
#include	<limits.h>

#ifdef BWT_ASSERTIONS
#include <assert.h>
#endif

// print matrix doesn't make sense without BWT_DEBUG
#ifndef BWT_DEBUG
#undef BWT_PRINT_MATRIX
#endif

/*
 * non input barrel version of multikey quick sort has
 * not been coded/tested so force use of input barrel
 */
#ifdef BWT_MULTI_QUICK
#define BWT_ENCODER_INPUT BARREL 1
#endif

/*
 * each encoded block starts with a preamble 4 bytes indicating the
 * location of the original index. BWT_HEADERLEN defines this size.
 */
#define BWT_HEADERLEN	 4

/* {{{1 SUPPORT FUNCTIONS */
static long
bwt_memcmp (unsigned int *s1, unsigned int *s2, unsigned int *o, unsigned int *e, unsigned long n)
{
	/*
	 * s1 = memory vector 1
	 * s2 = memory vector 2
	 * o = memory heap origin
	 * e = memory heap end
	 *	( o <= s1 <= e && o <= s2 <= e )
	 * n = length of longest vector (length of s1 and s2 should be equal)
	 */
	while (n -- > 0)
	{
		// no match at this offset - return difference
		if (*s1 != *s2)
			return (long) *s1 - *s2;

		++ s1;
		if (s1 > e)
			s1 = o;

		++ s2;
		if (s2 > e)
			s2 = o;
	}

	return 0;
}

#define swap(a,b)	{ unsigned int *t; t = *a; *a = *b; *b = t; }

#ifndef min
#define min(a, b) ((a)<=(b) ? (a) : (b))
#endif

#ifdef BWT_PRINT_MATRIX
/* print matrix digest */
static void
matrixPrint(unsigned int ** matrix, unsigned long input_size) {
#ifdef BWT_ENCODER_INPUT_BARREL
	int detail_i, detail_j;

	puts("=====");
	if (input_size >= BWT_PRINT_MATRIX_SIZE)
	{
		for (detail_i = 0; detail_i < BWT_PRINT_MATRIX_SIZE; ++detail_i)
		{
			for (detail_j = 0; detail_j < BWT_PRINT_MATRIX_SIZE; ++detail_j)
				printf("%c, ", matrix[detail_i][detail_j]-1);
			printf("... %c", matrix[detail_i][input_size-1]-1);
			puts("");
		}
		puts("...");
		for (detail_j = 0; detail_j < BWT_PRINT_MATRIX_SIZE; ++detail_j)
			printf("%c, ", matrix[input_size-1][detail_j]-1);
		printf("... %c", matrix[input_size-1][input_size-1]-1);
		puts("");
	}
	puts("=====");
#endif
}
#endif
/* }}} */

/* {{{1 SHELL SORT */
static void
shellsort (unsigned int **matrix, unsigned long range, unsigned long len, unsigned int *o, unsigned int *e)
{
	unsigned long gap, i;
	long j;

#ifdef BWT_DEBUG
	int ns = 0;
#endif

	for (gap = range/2; gap > 0; gap /= 2)
	{
		for (i = gap; i < range; ++ i)
		{
			for (j = i-gap; j >= 0; j -= gap)
			{
				if (bwt_memcmp (matrix[j], matrix[j+gap], o, e, len) < 0)
					break;

#ifdef BWT_DEBUG
				printf("swapping index %ld and %ld\n", j+gap, j);
				matrixPrint(matrix, len);
				ns ++;
#endif
				swap(&matrix[j+gap], &matrix[j]);
			}
		}
	}

#ifdef BWT_DEBUG
	printf("number of swaps: %d\n", ns);
#endif
}
/* }}} */

/* {{{1 QUICKSORT ROUTINES */
#ifdef BWT_QUICK
static unsigned long
partition (unsigned int **matrix, unsigned long n, unsigned int *o, unsigned int *e, unsigned long l, unsigned long r)
{
	/*
	 * partition a[l],... a[r] around pivot a[l] 
	 *
	 * return index at which pivot ends 
	 */

	unsigned int *pivot = matrix[l];
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
qksort (unsigned int **matrix, unsigned long n, unsigned long len, unsigned int *o, unsigned int *e, unsigned long l, unsigned long r)
{
	unsigned long k,
								i;
	unsigned int *v;


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
static void
vecswap (int i, int j, int n, unsigned int **matrix)
{
	while (n -- > 0)
	{
		swap (&matrix[i], &matrix[j]);
		++ i;
		++ j;
	}
}

static void
multiqksort (unsigned int **matrix, unsigned long n, unsigned long len, unsigned int *o, unsigned int *e, int depth)
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
		shellsort (matrix, n, len, o, e);
		return;
	}

	a = rand () % n;
	swap (&matrix[0], &matrix[a]);
	v =	matrix[0][depth];
	a = b = 1;
	c = d = n - 1;
	for (;;)
	{
		while (b <= c && (r = matrix[b][depth] - v) <= 0)
		{
			if (r == 0)
			{
				swap (&matrix[a], &matrix[b]);
				++ a;
			}
			++ b;
		}
		while (b <= c && (r = matrix[c][depth] - v) >= 0)
		{
			if (r == 0)
			{
				swap (&matrix[c], &matrix[d]);
				-- d;
			}
			-- c;
		}
		if (b > c)
			break;
		swap (&matrix[b], &matrix[c]);
		++ b;
		-- c;
	}
	r = min (a, b - a);
	vecswap (0, b - r, r, matrix);
	r = min (d - c, (int) n - d - 1);
	vecswap (b, n - r, r, matrix);
	r = b - a;
	multiqksort (matrix, r, len, o, e, depth);

	/*
	 * recurse if we  haven't reached end of string
	 * alternative is to check for NUL char
	 *
	 *	if ( 0 != matrix[r][depth] )
	 *
	 * but this obviously doesn't work for binary streams
	 */
	if ( a + n - d - 1 != len )
		multiqksort (matrix + r, a + n - d - 1, len, o, e, depth + 1);
	r = d - c;
	multiqksort (matrix + n - r, r, len, o, e, depth);
}
#endif /* BWT_MULTI_QUICK */
/* }}} */

/* {{{1 RADIX SORT ROUTINES */
#ifdef BWT_RADIX
/*
 * American Flag algorithm - McIlroy, Bostic implemenation
 * -- minor alterations to support circular memory
 */

#define RS_STACK_SIZE				1020
#define RS_THRESHOLD	16

struct rs_stack
{
	unsigned int	**sa;
	unsigned long sn;
	unsigned long si;
};

#define rs_push(a,n,i) sp->sa = a, sp->sn = n, sp->si = i, ++ sp
#define rs_pop(a,n,i)	 a = (--sp)->sa, n = sp->sn, i = sp->si
#define rs_empty()		 (sp <= stack+1)

static void
radixsort (unsigned int **a, unsigned long n, unsigned int *o, unsigned int *e)
{
	struct rs_stack stack[RS_STACK_SIZE+1],
									*sp = stack + 1;

	unsigned int	**pile[UCHAR_MAX+2],
								**an,		/* last string in current array */
								**ak,		/* current string */
								 *r;

	int	c,
			cmax,
			*cp,			/* count array pointer */
			count[UCHAR_MAX+2],
			cmin,			/* minimum character value -- lowest non 0 element in count array */
			nc = 0;		/* total chacter count -- how many non 0 elements in count array */

	/*
	 * used for shell sort -- n is the number of "rows" in the matrix
	 * to be sorted whereas len is the number of "columns"
	 */
	unsigned long len = n;		

	/* the offset of the byte to split on; the strings agree on earlier bytes */
	unsigned long b = 0;

	/*
	 * initialise count
	 */
	for ( c = 0; c < UCHAR_MAX+2; ++ c )
		count[c] = 0;

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
			continue;	// while loop
		}

		/* get last string in current array */
		an = a + n;

		/*
		 * tallying stage
		 */
		cmin = UCHAR_MAX;

		/* loop through array from beginning to end */
		for (ak = a; ak < an; )
		{
			/*
			 * get the first string after split (+ b)
			 * in the current string (ak) and
			 * move onto the next string in array
			 */
#ifdef BWT_ENCODER_INPUT_BARREL
			c = *((*ak++) + b);
#else
			unsigned int *check = (*ak++) + b;

			if (check > e)
				c = *(o + (check - e) - 1);
			else
				c = *check;
#endif

			/* increase count for current character */
			if (1 == ++ count[c] && c > 0)
			{
				/*
				 * if this is the first time we've
				 * encountered this character, check for
				 * cmin and increase character count.
				 */
				if (c < cmin)
					cmin = c;
				++ nc;
			}
		}

		/*
		 * find places
		 */
		cmax = 0;
		ak = a + count[0];
		pile[0] = ak;
		for (cp = count + cmin; nc > 0; ++ cp, -- nc)
		{
			/* skip any elements of count that are zero */
			while (0 == *cp)
				++ cp;

			/*
			 * if more than one character was found
			 * in the tallying stage, push the current
			 * string onto stack. Using the character
			 * count as a slice counter
			 */
			if (*cp > 1)
				rs_push (ak, *cp, b + 1);

			/*
			 * (cp - count) would be cmin if no zero elements in count
			 * array were skipped
			 */
			cmax = cp - count;

			ak += *cp;
			pile[cmax] = ak;
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
				unsigned int *check = r + b;

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
#endif
/* }}} */

/* {{{1 SORT MATRIX */
/*
	matrix	--> array of "strings"
	len	--> number of strings
	o	--> origin point of master string
	e --> end point of master string
*/
static void
sortMatrix (unsigned int **matrix, unsigned long len, unsigned int *o, unsigned int *e)
{
#ifdef BWT_QUICK
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts("quicksort!");
#endif
/* }}} */
	qksort (matrix, len, len, o, e, 0, len - 1);
#elif BWT_MULTI_QUICK
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts("multikey quicksort!");
#endif
/* }}} */
	multiqksort (matrix, len, len, o, e, 0);
#elif BWT_RADIX
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts("radixsort!");
#endif
/* }}} */
	radixsort (matrix, len, o, e);
#else
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts("shellsort!");
#endif
/* }}} */
	shellsort (matrix, len, len, o, e);
#endif
}
/* }}}1 */

/* {{{1 ENCODER */
int
bwt_encode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size)
{
	unsigned long i,
								orig_index;

	unsigned int	**matrix;

	unsigned int	*input_barrel;


/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts ("\nBWT Encoder");
	puts ("------------\n");
	puts ("allocating memory");
#endif
/* }}} */

	if ( 0 == input_size )
		return 0;

	/*
	 * allocate memory for input barrel 
	 */
#ifdef BWT_ENCODER_INPUT_BARREL
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts("using input barrel");
#endif
/* }}} */
	input_barrel = malloc (2 * input_size * sizeof *input_barrel);
	if (NULL == input_barrel)
		return 0;
#else
	input_barrel = malloc (input_size * sizeof *input_barrel);
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
	*output_size = ((BWT_HEADERLEN + input_size) * sizeof **output);
	*output = malloc (*output_size);
	if (NULL == *output)
	{
		free (matrix);
		free (input_barrel);
		return 0;
	}

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts ("populate input barrel");
#endif
/* }}} */

	/*
	 * populate input barrel
	 */
#ifdef BWT_ENCODER_INPUT_BARREL
	for (i = 0; i < input_size; ++ i)
		input_barrel[i + input_size] = input_barrel[i] = input[i] + 1;
#else
	for (i = 0; i < input_size; ++ i)
		input_barrel[i] = input[i] + 1;
#endif

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts ("create rotations");
#endif
/* }}} */

	/*
	 * create rotations 
	 */
	for (i = 0; i < input_size; ++i)
		matrix[i] = input_barrel + i;

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
#ifdef BWT_PRINT_MATRIX
#endif
	matrixPrint(matrix, input_size);
	puts ("sort matrix");
#endif
/* }}} */

	/* sort matrix entries */
	sortMatrix (matrix, input_size, input_barrel, input_barrel + input_size - 1);
	
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
#ifdef BWT_PRINT_MATRIX
	matrixPrint(matrix, input_size);
#endif
	puts ("find original input in matrix");
#endif
/* }}} */

	/* find orignal input in matrix */
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

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
		printf("orig index=%ld (max index=%ld)\n", orig_index, input_size-1);
#endif

#ifdef BWT_PRINT_MATRIX
		if (input_size >= BWT_PRINT_MATRIX_SIZE)
		{
			int detail_i;
			printf("       input=");
			for (detail_i = 0; detail_i < BWT_PRINT_MATRIX_SIZE; ++ detail_i) {
				printf("%c, ", *(input+detail_i));
			}
			printf("... %c", *(input+input_size-1));
			puts("");
			printf("matrix entry=");
			for (detail_i = 0; detail_i < BWT_PRINT_MATRIX_SIZE; ++ detail_i) {
				printf("%c, ", matrix[orig_index][detail_i]-1);
			}
			printf("... %c", matrix[orig_index][input_size-1]-1);
			puts("");
		}
#endif
/* }}} */

			break;
		}
	}

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts ("outputting last column");
#endif
/* }}} */

	/* copy last column to output */
	for (i = 0; i < input_size; ++i)
	{
#ifdef BWT_ENCODER_INPUT_BARREL
		(*output)[i+BWT_HEADERLEN] = ((*((matrix[i])+input_size-1)) - 1) & 0xff;
#else
		if (i != orig_index)
			(*output)[i+BWT_HEADERLEN] = ((*(matrix[i]-1)) - 1) & 0xff;
		else
			(*output)[i+BWT_HEADERLEN] = *(input+ (input_size-1));
#endif
	}

	/*
	 * free working memory 
	 */
	free (matrix);
	free (input_barrel);

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts ("done");
#endif
/* }}} */

	return 1;
}
/* }}} */

/* {{{1 DECODER */
int
bwt_decode (unsigned char *input, unsigned long input_size, unsigned char **output, unsigned long *output_size)
{
	unsigned long	i, j, sum, orig_index;
	
	unsigned long C[UCHAR_MAX + 1] = {0};
	unsigned long	*P;

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	puts ("\nBWT Decoder");
	puts ("------------\n");
	printf("input size=%ld\n", input_size);
#endif
/* }}} */

	/* parse header */
	orig_index = input[0] << 24;
	orig_index |= input[1] << 16;
	orig_index |= input[2] << 8;
	orig_index |= input[3];

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	printf("orig index=%ld\n", orig_index);
#endif
/* }}} */

	/* point input to block proper */
	input += BWT_HEADERLEN;
	*output_size = input_size -= BWT_HEADERLEN;

	P = malloc (input_size * sizeof *P);
	if ( NULL == P )
		return 0;

	/* allocate output memory */
	*output	= malloc (input_size * sizeof **output);
	if ( NULL == *output )
	{
		free (P);
		return 0;
	}

	/*
	 * count number of instances of each possible character in input stream (C)
	 * record number of previous occurances of each character in input stream (P)
	 */
	for ( i = 0; i < input_size; ++ i )
	{
		P[i] = C[input[i]];
		C[input[i]]	++;
	}

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	printf("       P=");
	for ( i = 0; i < input_size; ++ i )
	{
		printf("%ld,", P[i]);
	}
	puts("");
	printf("       C=");
	for ( i = 0; i < UCHAR_MAX+1; ++ i )
	{
		printf("%ld,", C[i]);
	}
	puts("");
#endif
/* }}} */


	/* cumulative sum over count arrary (C) */
	sum = 0;
	for ( i = 0; i < UCHAR_MAX+1; ++ i )
	{
		sum += C[i];
		C[i] = sum - C[i];
	}
	
/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	printf("summed C=");
	for ( i = 0; i < UCHAR_MAX+1; ++ i )
	{
		printf("%ld,", C[i]);
	}
	puts("");
#endif
/* }}} */

#ifdef BWT_ASSERTIONS
	assert(sum==*output_size); 
#endif

/* {{{2 DEBUG CODE */
#ifdef BWT_DEBUG
	printf("%ld\n", sum);
	puts("unwinding transform");
#endif
/* }}} */

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

	free (P);

	return 1;
}
/* }}} */

