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

#include	<types_lib.h>
#include	<llist_lib.h>



struct llist
{
	struct lnode   *head;
	struct ll_Options *options;
};

struct lnode
{
	struct lnode   *prev;
	struct lnode   *next;
	void           *contents;
};



/* prototypes for default hooks */
void *	ll_defaultConstruct (void *contents);
void    ll_defaultDestruct (void *contents);



struct llist *
ll_newList (struct ll_Options *options)
{
	struct llist   *list = {0};

	/* create list structure */
	list = malloc (sizeof *list );
	if ( NULL == list )
		return NULL;

	/* create dummy node */
	list->head = malloc (sizeof *list->head);
	if ( NULL == list->head )
	{
		free (list);
		return NULL;
	}

	/* point it's next and prev pointers to itself */
	list->head->next = list->head->prev = list->head;


	/* construct options --
	use user supplied options if possible
	else use default constructor and destructor */
	list->options = malloc (sizeof *list->options);
	if ( NULL == list->options )
	{
		free (list->head);
		free (list);
		return NULL;
	}
	if ( NULL == options )
	{
		list->options->hooks_construct = ll_defaultConstruct;
		list->options->hooks_destruct = ll_defaultDestruct;
	}
	else
	{
		if ( NULL == options->hooks_construct )
			list->options->hooks_construct = ll_defaultConstruct;
		else
			list->options->hooks_construct = options->hooks_construct;

		if ( NULL == options->hooks_destruct )
			list->options->hooks_destruct = ll_defaultDestruct;
		else
			list->options->hooks_destruct = options->hooks_destruct;
	}

	return list;
}

void
ll_disposeList (struct llist *list)
{
	struct lnode   *pointer;


	while (!ll_isListEmpty (list))
	{
		pointer = ll_removeTail (list);

		list->options->hooks_destruct (pointer->contents);
		free (pointer);
	}

	free (list->head);
	free (list->options);
	free (list);
}

void *
ll_returnNodeData (struct lnode * node)
{
	if ( NULL == node )
		return NULL;

	return node->contents;
}


struct lnode   *
ll_newNode (struct llist *list, void *data)
{
	struct lnode   *nn = {0};

	nn = malloc (sizeof *nn);
	if ( NULL == nn )
		return NULL;

	nn->contents = list->options->hooks_construct (data);
	if ( NULL == nn->contents )
	{
		free (nn);
		return NULL;
	}

	return nn;
}

bool
ll_newHead (struct llist * list, void *data)
{
	struct lnode   *nn;

	nn = malloc (sizeof *nn);
	if ( NULL == nn )
		return false;

	nn->contents = list->options->hooks_construct (data);
	if ( NULL == nn->contents )
	{
		free (nn);
		return false;
	}

	ll_addHead (list, nn);

	return true;
}

bool
ll_newTail (struct llist * list, void *data)
{
	struct lnode   *nn;

	nn = malloc (sizeof *nn);
	if ( NULL == nn )
		return false;

	nn->contents = list->options->hooks_construct (data);
	if ( NULL == nn->contents )
	{
		free (nn);
		return false;
	}

	ll_addTail (list, nn);

	return true;
}

void
ll_killNode (struct llist * list, struct lnode * node)
{
	ll_remove (list, node);
	list->options->hooks_destruct (node->contents);
	free (node);
}

bool
ll_killHead (struct llist * list)
{
	struct lnode   *nn;

	nn = ll_removeHead (list);
	if ( NULL == nn )
		return false;
	list->options->hooks_destruct (nn->contents);
	free (nn);

	return true;
}

bool
ll_killTail (struct llist * list)
{
	struct lnode   *nn;

	nn = ll_removeTail (list);
	if ( NULL == nn )
		return false;
	list->options->hooks_destruct (nn->contents);
	free (nn);
	return true;
}


void
ll_insert (struct llist *list, struct lnode *new_node, struct lnode *prev_node)
{
	new_node->prev = prev_node;
	new_node->next = prev_node->next;
	prev_node->next = new_node;
	new_node->next->prev = new_node;
}

void
ll_addTail (struct llist *list, struct lnode *new_node)
{
	ll_insert (list, new_node, list->head->prev);
}

void
ll_addHead (struct llist *list, struct lnode *new_node)
{
	ll_insert (list, new_node, list->head);
}

struct lnode *
ll_remove (struct llist *list, struct lnode *pointer)
{
	pointer->prev->next = pointer->next;
	pointer->next->prev = pointer->prev;

	return pointer;
}


struct lnode *
ll_removeTail (struct llist *list)
{
	if ( true == ll_isListEmpty (list) )
		return NULL;
	else
		return ll_remove (list, list->head->prev);
}


struct lnode *
ll_removeHead (struct llist *list)
{
	if ( true == ll_isListEmpty (list) )
		return NULL;
	else
		return ll_remove (list, list->head->next);
}


unsigned long
ll_countList (struct llist * list)
{
	unsigned long           ctr = 0;
	struct lnode   *pointer;


	for (pointer = ll_initialiseSearch (list);
	     !ll_isEndOfList (list, pointer);
	     pointer = ll_advancePointer (pointer), ctr++
		);

	return ctr;
}

struct lnode *
ll_initialiseSearch (struct llist *list)
{
	return list->head->next;
}

struct lnode *
ll_advancePointer (struct lnode *node)
{
	return node->next;
}

struct lnode *
ll_retreatPointer (struct lnode *node)
{
	return node->prev;
}

bool
ll_isListEmpty (struct llist * list)
{
	if ( list->head->next == list->head )
		return true;
	else
		return false;

}

bool
ll_isEndOfList (struct llist * list, struct lnode * node)
{
	if ( node == list->head )
		return true;
	else
		return false;
}



/* default hooks; assumes char * for data */
void *
ll_defaultConstruct (void *contents)
{
	char *          data;

	data = malloc ((strlen((char *)contents) + 1) * sizeof *data);
	if ( NULL == data )
		return NULL;

	strcpy (data, (char *) contents);

	return (void *) data;
}

void
ll_defaultDestruct (void *contents)
{
	free (contents);
}



/* MACRO VERSIONS OF SOME NODE FUNCTIONS
	 #define         ll_addTail(a,b)         ll_Insert((a),(b),(a)->head->prev)
	 #define         ll_addHead(a,b)         ll_Insert((a),(b),(a)->head)
	 #define         ll_advancePointer(a)    (a)->next
	 #define         ll_retreatPointer(a)    (a)->prev
	 #define         ll_initialiseSearch(a)  (a)->head->next
	 #define         ll_isListEmpty(a)       ((a)->head->next==(a)->head)?TRUE:FALSE
	 #define         ll_isEndOfList(a,b)     ((b)==(a)->head)?TRUE:FALSE
*/

