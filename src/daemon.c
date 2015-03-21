/*
 * Contains functions for dealing with things that happen in the
 * future.
 *
 * (#)daemon.c	5.2 (Berkeley) 6/18/82
 */

#include "rogue.h"
#include "curses.h"

#define EMPTY	0
#define FULL	1
#define DAEMON -1
#define MAXDAEMONS 20

/*@
 * struct delayed_action, as well as functions using it as return type such
 * as d_slot() and find_slot() are now marked static as the struct is not
 * declared in rogue.h, and they are only used in this file
 */

/*@
 * `int d_arg` member was removed as all fuses and daemons have no arguments,
 * and for the only one that did, turn_see(), the argument type is bool. It's
 * also now wrapped and no longer directly used as fuse, as its return type is
 * not void, making the argument member of this struct unneeded.
 *
 * A solution to handle generic functions of multiple return and argument types
 * would be a somewhat complex approach using unions to simulate overload, a
 * sophistication not needed for Rogue.
 */
static
struct delayed_action {
	void (*d_func)();
	int d_time;
} d_list[MAXDAEMONS];

/*
 * d_slot:
 *	Find an empty slot in the daemon/fuse list
 */
static
struct delayed_action *
d_slot(void)
{
	register struct delayed_action *dev;

	for (dev = d_list; dev < &d_list[MAXDAEMONS]; dev++)
		if (dev->d_func == EMPTY)
			return dev;
#ifdef DEBUG
	debug("Ran out of fuse slots");
#endif
	return NULL;
}

/*
 * find_slot:
 *	Find a particular slot in the table
 */
static
struct delayed_action *
find_slot(void (*func)())
{
	register struct delayed_action *dev;

	for (dev = d_list; dev < &d_list[MAXDAEMONS]; dev++)
	if (func == dev->d_func)
		return dev;
	return NULL;
}

/*
 * daemon:
 *	Start a daemon, takes a function.
 */
void
start_daemon(void (*func)())
{
	register struct delayed_action *dev;

	dev = d_slot();
	dev->d_func = func;
	dev->d_time = DAEMON;
}

/*
 * do_daemons:
 *	Run all the daemons, passing the argument to the function.
 */
void
do_daemons(void)
{
	register struct delayed_action *dev;

	/*
	 * Loop through the devil list
	 */
	for (dev = d_list; dev < &d_list[MAXDAEMONS]; dev++)
	{
		/*
		 * Executing each one, giving it the proper arguments
		 * @ Sorry, no more "arguments". And it was a single one.
		 */
		if (dev->d_time == DAEMON && dev->d_func != EMPTY)
		{
			(*dev->d_func)();
		}
	}
}

/*
 * fuse:
 *	Start a fuse to go off in a certain number of turns
 */
void
fuse(void (*func)(), int time)
{
	register struct delayed_action *wire;

	wire = d_slot();
	wire->d_func = func;
	wire->d_time = time;
}

/*
 * lengthen:
 *	Increase the time until a fuse goes off
 */
void
lengthen(void (*func)(), int xtime)
{
	register struct delayed_action *wire;

	if ((wire = find_slot(func)) == NULL)
		return;
	wire->d_time += xtime;
}

/*
 * extinguish:
 *	Put out a fuse
 */
void
extinguish(void (*func)())
{
	register struct delayed_action *wire;

	if ((wire = find_slot(func)) == NULL)
		return;
	wire->d_func = EMPTY;
}

/*
 * do_fuses:
 *	Decrement counters and start needed fuses
 */
void
do_fuses(void)
{
	register struct delayed_action *wire;

	/*
	 * Step though the list
	 */
	for (wire = d_list; wire < &d_list[MAXDAEMONS]; wire++) {
	/*
	 * Decrementing counters and starting things we want.  We also need
	 * to remove the fuse from the list once it has gone off.
	 */
		if (wire->d_func != EMPTY && wire->d_time > 0 && --wire->d_time == 0)
		{
			(*wire->d_func)();
			wire->d_func = EMPTY;
		}
	}
}
