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

//#define HUFF_DEBUG

#include	<stdlib.h>
#include	<limits.h>
#include	<string.h>

#ifdef HUFF_DEBUG
#include	<ctype.h>
#include	<stdio.h>
#endif

#include	<types_lib.h>
#include	<bitq_lib.h>
#include	<huff_lib.h>

/*
 * number of possible entries in dictionary
 */
#define DICT_SIZE				256

/* DEBUGGING FUNCTIONS {{{1 */
#ifdef HUFF_DEBUG
static void
printIntBinary (unsigned int number)
{
	unsigned int  value = number,
								rv,
								mask,
								i;

	mask = 1 << (sizeof(unsigned int) * CHAR_BIT - 1);

	for ( i = 0; i < sizeof(unsigned int) * CHAR_BIT; ++ i )
	{
		rv = (value & mask) >> (sizeof(unsigned int) * CHAR_BIT - 1);
		value <<= 1;
		printf("%d", rv);
	}
}

satic void
printCharBinary (unsigned char number)
{
	unsigned char  value = number,
								rv,
								mask,
								i;

	mask = 1 << (sizeof(unsigned char) * CHAR_BIT - 1);

	for ( i = 0; i < sizeof(unsigned char) * CHAR_BIT; ++ i )
	{
		rv = (value & mask) >> (sizeof(unsigned char) * CHAR_BIT - 1);
		value <<= 1;
		printf("%d", rv);
	}
}
#endif
/* }}}1 */

/* TREE CODE {{{1 */
struct tree_node
{
	struct tree_node	* left;
	struct tree_node	* right;

	unsigned char     out;
};

static void
killTree (struct tree_node * root)
{
	if ( NULL != root->left )
		killTree (root->left);
	
	if ( NULL != root->right )
		killTree (root->right);

	free (root);
}

static struct tree_node *
newTreeNode (void)
{
	struct tree_node	* node;
	
	node = malloc (sizeof *node);
	if ( NULL == node )
		return NULL;

	node->left = node->right = NULL;
	node->out = 0;

	return node;
}

static int
addCodeToTree (struct tree_node * root, unsigned char out, unsigned long code, unsigned int code_len)
{
	struct tree_node	*walk = root;
	unsigned long		 	code_play = code;
	int								long_size_bits = CHAR_BIT*sizeof(unsigned long);

	unsigned int	i;

	code_play <<= long_size_bits - code_len;

	for ( i = 1; i <= code_len; ++ i )
	{
		unsigned long	code_test = code_play >> (long_size_bits-i);

		if ( code_test & 0x1 )
		{
			/* go left */
			if ( NULL == walk->left )
			{
				walk->left = newTreeNode ();
				if ( NULL == walk->left )
					return HUFF_RET_NOMEM;
			}
			walk = walk->left;
		}
		else
		{
			/* go right */
			if ( NULL == walk->right )
			{
				walk->right = newTreeNode ();
				if ( NULL == walk->right )
					return HUFF_RET_NOMEM;
			}
			walk = walk->right;
		}
	}

	if ( !(NULL == walk->right && NULL == walk->left) )
	{
		return HUFF_RET_MALFORMED;
	}

	walk->out = out;
	
	return HUFF_RET_SUCCESS;
}
/* }}}1 */

/* COUNTSORT {{{1 */
/*
 * from "Mastering Algorithms With C"
 * Kyle Loudon, O'Reilly
 */
static int
countSort (unsigned long ** data, unsigned long max,
		unsigned char * idx, unsigned char * rev_idx)
{
	unsigned long *counts,
								*tmp,
								i, j;

	/* alloc counts array */
	counts = malloc ((max+1) * sizeof *counts);
	if (NULL == counts)
		return 0;

	/* alloc working space */
	tmp = malloc (DICT_SIZE * sizeof *tmp);
	if (NULL == tmp)
	{
		free (counts);
		return 0;
	}

	/* init counts array */
	for (i = 0; i <= max; ++ i)
		counts[i] = 0;

	/* accumulate frequencies */
	for (i = 0; i < DICT_SIZE; ++ i)
		++ counts[(*data)[i]];

	/* accumulate totals */
	for (i = 1; i <= max; ++ i)
		counts[i] += counts[i - 1];

	for ( i = 0; i < DICT_SIZE; ++ i)
	{
		tmp[counts[(*data)[i]] - 1] = (*data)[i];
		counts[(*data)[i]] = counts[(*data)[i]] - 1;

		/* update index */
		rev_idx[counts[(*data)[i]]] = i;
		idx[i] = counts[(*data)[i]];
	}

	free (counts);
	free (*data);
	*data = tmp;

	return 1;
}
/* }}}1 */

/* DICTIONARY STRUCTURE {{{1 */
struct huffDict
{
	unsigned int	num_entries;

	/*
	 * dict[i] points to code_lens entry for
	 * character i in the stream
	 */
	unsigned char	* dict;

	unsigned long	* code_lens;
	unsigned long * codes;

	/* 
	 * rev_dict[i] points to the dict
	 * entry that points to code_lens[i]
	 */
	unsigned char	* rev_dict;

	/*
	 * must be set to 0 in order for
	 * codeConstruct() to work properly for decoder
	 */
	unsigned int	dict_offset;
};

struct huffDict *
newDictionary (unsigned int num_entries)
{
	struct huffDict * dict;

	dict = malloc (sizeof *dict);
	if ( NULL == dict )
		return NULL;

	/*
	 * if the num_entries value is 0 then make the assumption that this
	 * is a complete dictionary of 256 entries
	 */
	if ( 0 == num_entries )
		dict->num_entries = DICT_SIZE;
	else
		dict->num_entries = num_entries;

	dict->codes = malloc (dict->num_entries * sizeof *dict->codes);
	if ( NULL == dict->codes )
	{
		free (dict);
		return NULL;
	}

	dict->dict = malloc (dict->num_entries * sizeof *dict->dict);
	if ( NULL == dict->dict )
	{
		free (dict->codes);
		free (dict);
		return NULL;
	}

	dict->code_lens = malloc (dict->num_entries * sizeof *dict->code_lens);
	if ( NULL == dict->code_lens )
	{
		free (dict->dict);
		free (dict->codes);
		free (dict);
		return NULL;
	}

	dict->rev_dict = malloc (dict->num_entries * sizeof *dict->rev_dict);
	if ( NULL == dict->rev_dict )
	{
		free (dict->code_lens);
		free (dict->dict);
		free (dict->codes);
		free (dict);
		return NULL;
	}


	dict->dict_offset = 0;

	return dict;
}

void
killDictionary (struct huffDict * dict)
{
	free (dict->rev_dict);
	free (dict->code_lens);
	free (dict->dict);
	free (dict->codes);
	free (dict);
}
/* }}}1 */

/* CODE CONSTRUCTION {{{1 */

/*
 * Minimum redundancy calculations from the paper
 * "In-Place Calculation of Minimum-Redundancy Codes" by 
 * Alistair Moffat and Jyrki Katajainen
 *
 * Original source: http://www.cs.mu.oz.au/~alistair/inplace.c
 */
static int
calcMinRedn (struct huffDict * dict)
{
	unsigned long	root;			/* next root node to be used */
	unsigned long leaf;			/* next leaf to be used */
	unsigned long next;			/* next value to be assigned */
	unsigned long avbl;			/* number of available nodes */
	unsigned long used;			/* number of internal nodes */
	unsigned long dpth;			/* current depth of leaves */

	unsigned long	* code_lens = dict->code_lens + dict->dict_offset;


	/* check for pathological cases */
	if (0 == dict->num_entries)
		return 0;

	if (1 == dict->num_entries)
	{
		code_lens[0] = 0;
		return 0;
	}

	/* first pass, left to right, setting parent pointers */
	code_lens[0] += code_lens[1];
	root = 0;
	leaf = 2;
	for (next = 1; next < dict->num_entries - 1; ++ next)
	{
		/* select first item for a pairing */
		if (leaf >= dict->num_entries || code_lens[root] < code_lens[leaf])
		{
			code_lens[next] = code_lens[root];
			code_lens[root ++] = next;
		}
		else
			code_lens[next] = code_lens[leaf ++];

		/* add on the second item */
		if (leaf >= dict->num_entries || (root < next && code_lens[root] < code_lens[leaf]))
		{
			code_lens[next] += code_lens[root];
			code_lens[root ++] = next;
		}
		else
			code_lens[next] += code_lens[leaf ++];
	}

	/* second pass, right to left, setting internal depths */
	code_lens[dict->num_entries - 2] = 0;
	for (next = dict->num_entries - 2; 0 != next ; -- next)
		code_lens[next-1] = code_lens[code_lens[next-1]] + 1;

	/* third pass, right to left, setting leaf depths */
	avbl = 1;
	used = dpth = 0;
	root = dict->num_entries - 2;
	next = dict->num_entries - 1;

	while (avbl > 0)
	{
		while ( code_lens[root] == dpth)
		{
			++ used;
			if ( 0 == root )
				break;
			-- root;
		}

		while (avbl > used)
		{
			code_lens[next --] = dpth;
			-- avbl;
		}
		
		avbl = 2 * used;
		++ dpth;
		used = 0;
	}

	return 1;
}

static int
codeConstruct (struct huffDict * dict)
{
	unsigned long last_amount = 0,		/* how many codes consist of last_len bits */
								last_start_code = 0,/* the start value of the last set of codes */
								shift_val = 1;			/* how far to shift the next starting code */

	unsigned long	i,
								j;

	unsigned long * codes = dict->codes + dict->dict_offset,
								* code_lens = dict->code_lens + dict->dict_offset;
							
	/* first code length is special case, it is always 00000...000 */
	*codes = last_start_code;	/* 0 in otherwords */
	for ( j = 1 ; j < DICT_SIZE; ++ j )
	{
		*(codes+j) = *(codes+j-1) + 1;

		if ( *code_lens != *(code_lens+j) )
			break;
	}
	last_amount = j;

	/* loop through the rest of the code_lens array */
	for ( i = j; i < dict->num_entries;)
	{
		shift_val = *(code_lens+i-1) - *(code_lens+i);

		/* work out start value for this code length */
		codes[i] = (last_start_code + last_amount) >> shift_val;
		last_start_code = codes[i];

		/* calculate other codes for this code len */
		for ( j = i+1; (j < dict->num_entries) && (*(code_lens+i)	== *(code_lens+j)); ++ j )
			*(codes+j) = *(codes+j-1) + 1;

		/* update some variables */
		last_amount = j - i;
		i = j;
	}

	return 1;
}
/* }}}1 */

/* READ/WRITE DICTIONARY {{{1 */

	/* max this can be is 6, so output 3 bits only */
#define CODELEN_INDICATOR_WIDTH	3

static int
writeDictionary (struct bitqStream * stream, struct huffDict	* dict)
{
	int		i, j, ret;
	unsigned char	k;


	/*
	 * what is the maximim number of bits required
	 * to describe all code lens in the dictionary
	 */
	if ( dict->code_lens[dict->dict_offset] > 31 )
		k = 6;
	else if ( dict->code_lens[dict->dict_offset] > 15 )
		k = 5;
	else if ( dict->code_lens[dict->dict_offset] > 7 )
		k = 4;
	else if ( dict->code_lens[dict->dict_offset] > 3 )
		k = 3;
	else
		k = 2;

	ret = bitq_writeStream (stream, k, CODELEN_INDICATOR_WIDTH); 
	if ( BITQ_CONTINUE != ret )
		return ret;

	/* add dictionary to output */
	for ( i = 0, j = DICT_SIZE-1; i < DICT_SIZE; ++ i, -- j )
	{
		if ( dict->dict[i] >= dict->dict_offset )
		{
			/* this dictionary entry is used so write a one bit ... */
			ret = bitq_writeStream (stream, 1, 1);
			if ( BITQ_CONTINUE != ret )
				return ret;
			/* ... and write the code length of this dictionary entry
			 * using just k bits */
			ret = bitq_writeStream (stream, dict->code_lens[dict->dict[i]], k);
			if ( BITQ_CONTINUE != ret )
				return ret;
		}
		else
		{
			/* this dictionary entry isn't used so write a zero bit
			 * and continue to the next dictionary entry */
			ret = bitq_writeStream (stream, 0, 1);
			if ( BITQ_CONTINUE != ret )
				return ret;
		}
	}

	return BITQ_CONTINUE;
}

static struct huffDict * 
readDictionary (struct bitqStream * stream)
{
	struct huffDict * dict; 
	int							i, j;

	unsigned long	s;
	unsigned char	t, codelen;

	unsigned long		count_max = 0;

	dict = newDictionary (DICT_SIZE);
	if ( NULL == dict )
		return NULL;

	/* how many bits are used per codelen */
	bitq_readStream (stream, &codelen, CODELEN_INDICATOR_WIDTH);

	/* read code lengths when appropriate */
	for ( i = 0; i < DICT_SIZE; ++ i )
	{
		dict->dict[i] = i;
		dict->rev_dict[i] = i;
		
		bitq_readStream (stream, &t, 1);
		if ( 1 == t )
		{
			bitq_readStream (stream, &t, codelen);
			dict->code_lens[i] = t;

			if ( dict->code_lens[i] > count_max )
				count_max = dict->code_lens[i];
		}
		else
		{
			dict->code_lens[i] = 0;
		}
	}

	/* sort dictionary */
	if (0 == countSort (&dict->code_lens, count_max, dict->dict, dict->rev_dict))
	{
		killDictionary (dict);
		return NULL;
	}

	/* find dictionary offset (first non zero element) in dict */
	for ( dict->dict_offset = 0; 0 == dict->code_lens[dict->dict_offset]; ++ dict->dict_offset )
		;

	/* ignore first dict_offset entries */
	dict->num_entries = DICT_SIZE - dict->dict_offset;

	/* flip code lens array */
	for ( i = dict->dict_offset, j = DICT_SIZE-1; i < j; ++ i, -- j )
	{
		s = dict->code_lens[i];
		dict->code_lens[i] = dict->code_lens[j];
		dict->code_lens[j] = s;

		t = dict->rev_dict[i];
		dict->rev_dict[i] = dict->rev_dict[j];
		dict->rev_dict[j] = t;
	}

	/* recreate dictionary */
	codeConstruct (dict);

	return dict;
}
/* }}}1 */

/* ENCODER {{{1 */
static
void sortRevDict (struct huffDict * dict)
{
	unsigned long i, j, k, l, m;

	/* sort according to rev_dict */
	for ( i = dict->dict_offset; i < DICT_SIZE; ++ i )
	{
		k = dict->code_lens[i];
		for ( j = i+1; j < DICT_SIZE; ++ j )
		{
			if ( dict->code_lens[j] != k )
				break; /* j for loop */
		}

		-- j;

		if ( j-i > 0 )
		{
			for ( m = i; m < j; ++ m )
			{
				for ( l = j; l > m; -- l )
				{
					if ( dict->rev_dict[l] < dict->rev_dict[m] )
					{
						unsigned char s = dict->rev_dict[m];

						dict->rev_dict[m] = dict->rev_dict[l];
						dict->rev_dict[l] = s;

						s = dict->dict[dict->rev_dict[m]];
						dict->dict[dict->rev_dict[m]] = dict->dict[dict->rev_dict[l]];
						dict->dict[dict->rev_dict[l]] = s;
					}
				}
			}
		}

		i = j;
	}
}

int
huff_encode (unsigned char *input, unsigned long input_size, unsigned char **output,
							unsigned long *output_size, unsigned long pre_padding)
{
	unsigned long i;
	unsigned char	*tmp;

	struct huffDict	*dict;
	unsigned long		count_max = 0;

	struct bitqStream	* stream;
	unsigned long			stream_size;

/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
puts ("\n\nHUFFMAN ENCODER\n----");
#endif
/* }}} */

	if ( 0 == input_size )
		return HUFF_RET_EMPTY_INPUT;

	dict = newDictionary (DICT_SIZE);
	if ( dict == NULL )
		return HUFF_RET_NOMEM;

	/*
	 * we use dict->code_lens for frequency
	 * accumulation and sorting
	 */
	for ( i = 0; i < DICT_SIZE; ++ i )
		dict->code_lens[i] = 0;

	/*
	 * initialize forward and reverse dictionary index
	 * -- set each element to value of it's position
	 */
	for (i = 0; i < DICT_SIZE; ++ i)
		dict->dict[i] = dict->rev_dict[i] = i;

	/* build frequencies */
	for (i = 0; i < input_size; ++ i)
	{
		++ dict->code_lens[input[i]];

		/*
		 * remember maximum value in input stream
		 * -- used for count sort later
		 */
		if ( dict->code_lens[input[i]] > count_max )
			count_max = dict->code_lens[input[i]];
	}

/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
	puts ("\nUnsorted Frequencies\n---");
	for (i = 0; i < DICT_SIZE; ++ i)
		printf ("%d -> %ld\n", dict->dict[i], dict->code_lens[i]);
#endif
/* }}} */

	/* sort dictionary */
	if (0 == countSort (&dict->code_lens, count_max, dict->dict, dict->rev_dict))
	{
		killDictionary (dict);
		return HUFF_RET_NOMEM;
	}

	/* find dictionary offset (first non zero element) in dict */
	for ( dict->dict_offset = 0; 0 == dict->code_lens[dict->dict_offset]; ++ dict->dict_offset )
		;

	/* ignore first dict_offset entries */
	dict->num_entries = DICT_SIZE - dict->dict_offset;

/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
	puts ("\nSorted Frequencies\n---");
	for (i = DICT_SIZE - dict->num_entries; i < DICT_SIZE; ++ i)
		printf ("%d(%d) -> %ld\n", dict->rev_dict[i], dict->dict[i], dict->code_lens[i]);
#endif
/* }}} */

	/* calculate minimum redundancy */
	if ( 0 == calcMinRedn (dict) )
	{
		killDictionary (dict);
		return HUFF_RET_NOMEM;
	}

	sortRevDict (dict);

/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
	puts ("\nCodeword Lengths\n---");
	for (i = dict->dict_offset; i < DICT_SIZE; ++ i)
		printf ("%d. %d -> %ld\n", i, dict->rev_dict[i], dict->code_lens[i]);
#endif
/* }}} */

	/* code construction */
	if ( 0 == codeConstruct (dict) )
	{
		killDictionary (dict);
		return HUFF_RET_NOMEM;
	}

/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
	puts ("\nCodewords\n---");
	for (i = dict->dict_offset; i < DICT_SIZE; ++ i)
	{
		printf ("%d", dict->rev_dict[i]);
		if ( isalnum (dict->rev_dict[i]) )
			printf (" (%c) -> ", dict->rev_dict[i]);
		else
			printf ("  -> ");
		printIntBinary (dict->codes[i]);
		printf(" (%ld)\n", dict->code_lens[i]);
	}

	printf("\n\n");
#endif
/* }}} */

	/* calculate max length of output */
	stream_size = (input_size + pre_padding) * sizeof **output;

	/* allocate memory for output */
	*output = malloc (stream_size);
	if ( NULL == *output )
	{
		killDictionary (dict);
		return HUFF_RET_NOMEM;
	}

	/* initialise bitq stream */
	stream = bitq_newStream (*output, stream_size, output_size);
	if ( NULL == stream )
	{
		killDictionary (dict);
		return HUFF_RET_NOMEM;
	}

	/* don't touch the pre-padding */
	*output_size = pre_padding;

	/* write input size to output */
	bitq_writeStream (stream, input_size, CHAR_BIT * 4);

	/* write dictionary to output stream */
	if ( BITQ_CONTINUE != writeDictionary (stream, dict) )
	{
		bitq_freeStream (stream);
		free (*output);
		killDictionary (dict);
		return HUFF_RET_TOOBIG;
	}

	/* do actual encoding */
	for ( i = 0; i < input_size; ++ i )
	{
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
	printf("(%x) - ", input[i]);
	printIntBinary ( dict->codes[dict->dict[input[i]]] );
	printf("(%ld) \n", dict->code_lens[dict->dict[input[i]]]);
#endif
/* }}} */

		if ( BITQ_CONTINUE != bitq_writeStream (stream,
			dict->codes[dict->dict[input[i]]],
			dict->code_lens[dict->dict[input[i]]]) )
		{
			bitq_freeStream (stream);
			free (*output);
			killDictionary (dict);
			return HUFF_RET_TOOBIG;
		}
	}

	/* output leftovers */
	bitq_flushWriteStream (stream);

	/* free memory */
	bitq_freeStream (stream);
	killDictionary (dict);

	/* trim output memory */
	tmp = realloc (*output, *output_size);
	if ( NULL == tmp )
	{
		free (*output);
		return HUFF_RET_NOMEM;
	}
	*output = tmp;

	return HUFF_RET_SUCCESS;
}
/* }}}1 */

/* DECODER {{{1 */
int
huff_decode (unsigned char *input, unsigned long input_size, unsigned char **output,
								unsigned long *output_size, unsigned long pre_padding)
{
	int	ret;

	unsigned long	i;
	unsigned long	input_i = 0;			/* offset into the input */
	unsigned long	output_i = 0;			/* offset into the ouput */

	struct huffDict		* dict;
	struct tree_node	* root;
	struct tree_node	* walk;

	unsigned char	c;	/* output character */

	struct bitqStream	* stream;
	unsigned char	d;


	/* make sure we ignore any padding bytes */
	input += pre_padding;
	input_size -= pre_padding;

	stream = bitq_newStream (input, input_size, &input_i);
	if ( NULL == stream )
		return HUFF_RET_NOMEM;

	/* read in original size */
	bitq_readStream (stream, &d, CHAR_BIT);
	*output_size = d << 24;
	bitq_readStream (stream, &d, CHAR_BIT);
	*output_size |= d << 16;
	bitq_readStream (stream, &d, CHAR_BIT);
	*output_size |= d << 8;
	bitq_readStream (stream, &d, CHAR_BIT);
	*output_size |= d;

	/* allocate output memory */
	*output = malloc (*output_size * sizeof ** output);
	if ( NULL == *output )
	{
		bitq_freeStream (stream);
		return HUFF_RET_NOMEM;
	}

	/* read dictionary from input and construct tree */
	dict = readDictionary (stream);
	if ( NULL == dict )
	{
		free (*output);
		bitq_freeStream (stream);
		return HUFF_RET_NOMEM;
	}

	/* prepare new instance of tree */
	root = newTreeNode ();
	if ( NULL == root )
	{
		killDictionary (dict);
		free (*output);
		bitq_freeStream (stream);
		return HUFF_RET_NOMEM;
	}

	/* build tree */
	for ( i = dict->dict_offset; i < DICT_SIZE; ++ i )
	{
		ret = addCodeToTree (root, dict->rev_dict[i], dict->codes[i], dict->code_lens[i]);
		if ( HUFF_RET_SUCCESS != ret )
		{
			killDictionary (dict);
			free (*output);
			bitq_freeStream (stream);
			return ret;
		}
	}

	/* once tree is built, dictionary is no longer needed */
	killDictionary (dict);

	/* read data */

	/* start at root of code tree */
	walk = root;

/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
puts("bits\n---");
#endif
/* }}} */

	while ( 1 == bitq_readStream (stream, &c, 1) )
	{
		/* if bit is a one... */
		if ( (c & 0x01) == 1 )
		{
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
putchar ('1');
#endif
/* }}} */
			/* ...and a left node exists at current point in tree... */
			if ( NULL != walk->left )
			{
				/* ...move tree pointer to it */
				walk = walk->left;
	
				/* if there are no child nodes at this point... */
				if ( NULL == walk->left && NULL == walk->right )
				{
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
printf ("\n%c\n", walk->out);
#endif
/* }}} */
					/* ...put char in output stream */
					*(*output+output_i) = walk->out;
	
					++ output_i;

					/* if output is now the predicted size then break */
					if ( output_i == *output_size )
						break; /* read stream loop */

					/* reset tree pointer */
					walk = root;
				}
			}
			else
			{
				killTree (root);
				free (*output);
				bitq_freeStream (stream);
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
puts("");
#endif
/* }}} */
				return HUFF_RET_MALFORMED;
			}
		} /* bit is one */
		else
		{
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
putchar ('0');
#endif
/* }}} */
			if ( NULL != walk->right )
			{
				walk = walk->right;
				if ( NULL == walk->left && NULL == walk->right )
				{
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
printf ("\n%c\n", walk->out);
#endif
/* }}} */
					*(*output+output_i) = walk->out;
					++ output_i;
	
					if ( output_i == *output_size )
						break; /* read strem loop */
	
					walk = root;
				}
			}
			else
			{
				killTree (root);
				free (*output);
				bitq_freeStream (stream);
/* DEBUG CODE {{{ */
#ifdef HUFF_DEBUG
puts("");
#endif
/* }}} */
				return HUFF_RET_MALFORMED;
			}
		}	/* bit is zero */
	} /* read stream loop */

	killTree (root);
	bitq_freeStream (stream);

	return HUFF_RET_SUCCESS;
}
/* }}}1 */
