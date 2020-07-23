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
#include	<limits.h>

#include	<types_lib.h>


/* {{{1 MODEL CODE */

struct mtfModel
{
	unsigned char L[UCHAR_MAX];

	void (*updateModel)(struct mtfModel *, unsigned char i);

	/* only used in MTF-2 */
	unsigned char	prev_transform;
};

/*
 * plain mtf as suggested in Burrow and Wheeler's
 * original paper
 */
static void
updateModel_0 (struct mtfModel * m, unsigned char i)
{
	unsigned char	c = m->L[i];

	if ( 0 == i )
		return;

	/*
	 * remove and reinsert m->L[i] at head of "list"
	 */
	for (; i > 0; -- i)
 		m->L[i] = m->L[i - 1];

	m->L[0] = c;
}

/*
 * variant mtf where characters are swapped
 * to position 1, or position 0 if it's already
 * at position 1
 */
static void
updateModel_1 (struct mtfModel * m, unsigned char i)
{
	unsigned char	c = m->L[i];

	if ( 0 == i )
		return;

	if ( 1 == i )
	{
		m->L[1] = m->L[0];
		m->L[0] = c;
		return;
	}

	/*
	 * remove and reinsert m->L[i] at head of "list"
	 */
	for (; i > 1; -- i)
 		m->L[i] = m->L[i - 1];

	m->L[1] = c;
}

static void
updateModel_2 (struct mtfModel * m, unsigned char i)
{
	unsigned char	c = m->L[i];

	if ( 0 == i )
	{
		m->prev_transform = c;
		return;
	}

	if ( 1 == i )
	{
		/*
		 * only swap positions 1 and 0 if previous transform
		 * resulted in the movement to position 0
		 */
		if ( m->prev_transform == m->L[0] )
		{
			m->L[1] = m->L[0];
			m->L[0] = c;
		}

		m->prev_transform = c;
		return;
	}

	/*
	 * remove and reinsert m->L[i] at head of "list"
	 */
	for (; i > 1; -- i)
 		m->L[i] = m->L[i - 1];

	m->prev_transform = m->L[1] = c;
}

/*
 * return index of character c in model
 */
static unsigned int
findCharacter(struct mtfModel * m, unsigned char c)
{
	unsigned int i;

	for (i = 0; i <= sizeof m->L; ++ i)
	{
		if (c == m->L[i])
			break;
	}

	return i;
}

/*
 * initialise model -- set initial list
 * and select appropriate update function
 *
 * model_type defaults to zero if specified
 * type is unknown
 */
static void
initModel (struct mtfModel * m, int model_type)
{
	unsigned int i;

  /* initialize alphabet array */
	for (i = 0; i <= sizeof m->L; ++ i)
    m->L[i] = i;

	if ( 2 == model_type )
	{
		m->updateModel = updateModel_2;
		m->prev_transform = 1;
	}
	else
	if ( 1 == model_type )
		m->updateModel = updateModel_1;
	else
		m->updateModel = updateModel_0;
}
/* }}}1 */

/* {{{1 ENCODE */
int
mtf_encode (unsigned char *data, unsigned long data_size, int model_type)
{
	struct mtfModel	m;
	unsigned int 		i;


	initModel (&m, model_type);

  /* data loop */
	for (; data_size > 0; -- data_size)
  {
		i = findCharacter (&m, *data);
    m.updateModel (&m, i);
		*data = i;
		++ data;
  }

	return 1;
}
/* }}} */

/* {{{1 DECODE */
int
mtf_decode (unsigned char *data, unsigned long data_size, int model_type)
{
	struct mtfModel	m;

	initModel (&m, model_type);

  /* data loop */
	for (; data_size > 0; -- data_size)
  {
		unsigned char	i = *data;

		*data = m.L[*data];
    m.updateModel (&m, i);
		++ data;
  }

	return 1;
}
/* }}} */
