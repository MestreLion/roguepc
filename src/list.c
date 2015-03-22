/*
 * Functions for dealing with linked lists of goodies
 * Functions with names starting with an "_" have compainion #defines
 * in rogue.h which take the address of the first argument and pass it on.
 *
 * list.c	1.4 (A.I. Design) 12/5/85
 */

#include "rogue.h"
#include "curses.h"

static void	*talloc(void);

/*
 * detach:
 *	Takes an item out of whatever linked list it might be in
 */
void
list_detach(list, item)
	register THING **list, *item;
{
	if (*list == item)
		*list = next(item);
	if (prev(item) != NULL) item->l_prev->l_next = next(item);
	if (next(item) != NULL) item->l_next->l_prev = prev(item);
	item->l_next = NULL;
	item->l_prev = NULL;
}

/*
 * _attach:
 *	add an item to the head of a list
 */
void
list_attach(list, item)
	register THING **list, *item;
{
	if (*list != NULL)
	{
		item->l_next = *list;
		(*list)->l_prev = item;
		item->l_prev = NULL;
	}
	else
	{
		item->l_next = NULL;
		item->l_prev = NULL;
	}
	*list = item;
}

/*
 * _free_list:
 *	Throw the whole blamed thing away
 */
void
list_free(ptr)
	register THING **ptr;
{
	register THING *item;

	while (*ptr != NULL)
	{
	item = *ptr;
	*ptr = next(item);
	discard(item);
	}
}

/*
 * new_item
 *	Get a new item with a specified size
 */
THING *
new_item()
{
	register THING *item;
#ifdef DEBUG
	if ((item = (THING *) talloc()) == NULL)
		if (me())msg("no more things!");
	else
#else
	if ((item = (THING *) talloc()) != NULL)
#endif //DEBUG
			 item->l_next = item->l_prev = NULL;
	return item;
}

/*
 * talloc: simple allocation of a THING
 */
static
void *  //@ maybe should be THING*, as this is a specialized malloc()
talloc()
{
	register int i;

	for (i=0;i<MAXITEMS;i++)
	{
		if (_t_alloc[i] == 0)
		{
			if (++total > maxitems)
			maxitems = total;
			_t_alloc[i]++;
			setmem(&_things[i],sizeof(THING),0);
			return &_things[i];
		}
	}
	return NULL;
}

/*
 * discard:
 *	Free up an item
 */
int
discard(item)
	register THING *item;
{
	register int i;

	for (i=0;i<MAXITEMS;i++)
	{
		if (item == &_things[i])
		{
			--total;
			_t_alloc[i] = 0;
			return 1;
		}
	}
	return 0;
}
