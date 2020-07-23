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

#ifndef LLISTLIB_H
#define LLISTLIB_H

/* Generic, double linked list routines */


#include    <types_lib.h>


struct llist;
struct lnode;

/*
	 An options structure is passed to ll_newList(). newList always makes a
	 local copy of this structure so we need not worry about maintaining it
	 until the ll_disposeList()
 */
struct ll_Options
{
	/*
	   The construct hook may not always be necessary, depending on how
	   you structure your main code. It's purpose is to copy contents to
	   a private instance of that data that is then returned to the calling
	   function. Tentatively, the ll_new*() functions
	 */
	void           *(*hooks_construct) (void *contents);
	/*
	   The destruct hook is more useful as it allows the automatic destruction
	   of data in a node, particularly in ll_disposeList(). But also ll_kill*()
	 */
	void            (*hooks_destruct) (void *contents);
};


struct llist   *ll_newList (struct ll_Options *);
void            ll_disposeList (struct llist *);

void *			ll_returnNodeData (struct lnode * node);

struct lnode   *ll_newNode (struct llist *, void *data);
bool            ll_newHead (struct llist *, void *data);
bool            ll_newTail (struct llist *, void *data);

	/* the kill functions return FALSE only if list is empty
	there is no problem with ignoring this return value */
void            ll_killNode (struct llist *, struct lnode *);
bool            ll_killHead (struct llist *);
bool            ll_killTail (struct llist *);

void            ll_insert (struct llist *, struct lnode *new_node, struct lnode *prev_node);
void            ll_addTail (struct llist *, struct lnode *);
void            ll_addHead (struct llist *, struct lnode *);
struct lnode   *ll_remove (struct llist *, struct lnode *);
struct lnode   *ll_removeTail (struct llist *);
struct lnode   *ll_removeHead (struct llist *);

unsigned long   ll_countList (struct llist *);
struct lnode   *ll_initialiseSearch (struct llist *);
struct lnode   *ll_advancePointer (struct lnode *);
struct lnode   *ll_retreatPointer (struct lnode *);
bool            ll_isListEmpty (struct llist *);
bool            ll_isEndOfList (struct llist *, struct lnode *);

#endif /* LLIST_H */
