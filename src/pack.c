#include "rogue.h"
#include "curses.h"

/*
 * Routines to deal with the pack
 *
 * pack.c	1.4 (A.I. Design)	12/14/84
 */

static
THING *
pack_obj(byte ch, byte *chp)
{
	register THING *obj;
	register byte och;

	for (obj = pack, och = 'a'; obj != NULL; obj = next(obj), och++)
		if (ch == och)
			return obj;
	*chp = och;
	return NULL;
}

/*
 * add_pack:
 *	Pick up an object and add it to the pack.  If the argument is
 *	non-null use it as the linked_list pointer instead of gettting
 *	it off the ground.
 */
void
add_pack(THING *obj, bool silent)
{
	register THING *op, *lp = NULL;
	register bool exact, from_floor;
	register byte floor;

	if (obj == NULL)
	{
		from_floor = TRUE;
		if ((obj = find_obj(hero.y, hero.x)) == NULL)
			return;
		cur_debug("Adding to pack: [%p]", obj);
	}
	else
		from_floor = FALSE;
	/*
	 * Link it into the pack.  Search the pack for a object of similar type
	 * if there isn't one, stuff it at the beginning, if there is, look for one
	 * that is exactly the same and just increment the count if there is.
	 * Food is always put at the beginning for ease of access, but it
	 * is not ordered so that you can't tell good food from bad.  First check
	 * to see if there is something in the same group and if there is then
	 * increment the count.
	 */

	/*@
	 *  bug in original Rogue: it didn't check proom != NULL, as is the case
	 *  when add_pack() is called from init_player(), which happens before
	 *  any room even exist. proom is set in enter_room(), which is first
	 *  called in new_level()
	 */
	floor = (proom != NULL && (proom->r_flags & ISGONE)) ? PASSAGE : FLOOR;
	if (obj->o_group)
	{
		for (op = pack; op != NULL; op = next(op))
		{
			if (op->o_group == obj->o_group)
			{
			/*
			 * Put it in the pack and notify the user
			 */
				op->o_count += obj->o_count;
				if (from_floor)
				{
					detach(lvl_obj, obj);
					mvaddch(hero.y, hero.x, floor);
					chat(hero.y, hero.x) = floor;
				}
				discard(obj);
				obj = op;
				goto picked_up;
			}
		}
	}
	/*
	 * Check if there is room
	 */
	if (inpack >= MAXPACK-1)
	{
		msg("you can't carry anything else");
		return;
	}
	/*
	 * Check for and deal with scare monster scrolls
	 */
	if (obj->o_type == SCROLL && obj->o_which == S_SCARE)
	{
		if (obj->o_flags & ISFOUND)
		{
			detach(lvl_obj, obj);
			mvaddch(hero.y, hero.x, floor);
			chat(hero.y, hero.x) = floor;
			msg("the scroll turns to dust%s.", noterse(" as you pick it up"));
			return;
		}
		else
			obj->o_flags |= ISFOUND;
	}

	inpack++;
	if (from_floor)
	{
		detach(lvl_obj, obj);
		mvaddch(hero.y, hero.x, floor);
		chat(hero.y, hero.x) = floor;
		cur_debug("Detached from floor");
	}
	/*
	 * Search for an object of the same type
	 */
	exact = FALSE;
	for (op = pack; op != NULL; op = next(op))
		if (obj->o_type == op->o_type)
			break;
	if (op == NULL)
	{
		/*
		 * Put it at the end of the pack since it is a new type
		 */
		for (op = pack; op != NULL; op = next(op))
		{
			if (op->o_type != FOOD)
				break;
			lp = op;
		}
	}
	else
	{
		/*
		 * Search for an object which is exactly the same
		 */
		while (op->o_type == obj->o_type)
		{
			if (op->o_which == obj->o_which)
			{
				exact = TRUE;
				break;
			}
			lp = op;
			if ((op = next(op)) == NULL)
				break;
		}
	}
	if (op == NULL)
	{
		/*
		 * Didn't find an exact match, just stick it here
		 */
		if (pack == NULL)
			pack = obj;
		else
		{
			lp->l_next = obj;
			obj->l_prev = lp;
			obj->l_next = NULL;
		}
		cur_debug("Placed on inventory");
	}
	else
	{
		/*
		 * If we found an exact match.  If it is a potion, food, or a
		 * scroll, increase the count, otherwise put it with its clones.
		 */
		if (exact && ISMULT(obj->o_type))
		{
			op->o_count++;
			discard(obj);
			obj = op;
			goto picked_up;
		}
		if ((obj->l_prev = prev(op)) != NULL)
		{
			obj->l_prev->l_next = obj;
		}
		else
		{
			pack = obj;
		}
		obj->l_next = op;
		op->l_prev = obj;
	}
picked_up:
	/*
	 * If this was the object of something's desire, that monster will
	 * get mad and run at the hero
	 */
	for (op = mlist; op != NULL; op = next(op))
	{
		/*
		 *  compiler bug: jll : 2-7-83
		 *		It is stupid because it thinks the obj... is not an lvalue
		 *		this may be true since there is no structure assignments,
		 *		but still it should let you have the address??!!
		 *
		if (&obj->_o._o_pos == op->t_dest)
		 *
		 *  the following should do the same
		 */
		/*@
		 * Another bug in Rogue: missed NULL check for t_dest. Monsters could
		 * be not chasing (sleeping, another room, Ice Monster, etc), so a
		 * destination could possibly have never been assigned.
		 */
		if (op->t_dest != NULL &&
		   (op->t_dest->x == obj->o_pos.x) && (op->t_dest->y == obj->o_pos.y))
			op->t_dest = &hero;
	}

	if (obj->o_type == AMULET)
	{
		amulet = TRUE;
		saw_amulet = TRUE;
	}
	/*
	 * Notify the user
	 */
	if (!silent)
		msg("%s%s (%c)",noterse("you now have "),
			inv_name(obj, TRUE), pack_char(obj));
}

/*
 * inventory:
 *	List what is in the pack
 */
byte
inventory(THING *list, int type, char *lstr)
{
	register byte ch;
	register int n_objs;
	char inv_temp[MAXSTR];

	n_objs = 0;
	for (ch = 'a'; list != NULL; ch++, list = next(list))
	{
		/*
		 * Don't print this one if:
		 *	the type doesn't match the type we were passed AND
		 *	it isn't a callable type AND
		 *	it isn't a zappable weapon
		 */
		if (type && type != list->o_type && !(type == CALLABLE &&
		  (list->o_type == SCROLL || list->o_type == POTION ||
		  list->o_type == RING || list->o_type == STICK)) &&
		  !(type == WEAPON && list->o_type == POTION) &&
		  !(type == STICK && list->o_enemy && list->o_charges))
			continue;
		n_objs++;
		sprintf(inv_temp, "%c) %%s", ch);
		add_line(lstr, inv_temp, inv_name(list, FALSE));
	}
	if (n_objs == 0)
	{
		msg(type == 0 ? "you are empty handed" :
					"you don't have anything appropriate");
		return 0;
	}
	return(end_line(lstr));
}

/*
 * pick_up:
 *	Add something to characters pack.
 */
void
pick_up(byte ch)
{
	register THING *obj;

	cur_debug("About to pick up: [%02x]", ch);
	switch (ch)
	{
	case GOLD:
		if ((obj = find_obj(hero.y, hero.x)) == NULL)
		return;
		money(obj->o_goldval);
		detach(lvl_obj, obj);
		discard(obj);
		proom->r_goldval = 0;
		break;
	default:
	case ARMOR:
	case POTION:
	case FOOD:
	case WEAPON:
	case SCROLL:
	case AMULET:
	case RING:
	case STICK:
		add_pack(NULL, FALSE);
		break;
	}
}

/*
 * get_item:
 *	Pick something out of a pack for a purpose
 */
THING *
get_item(char *purpose, int type)
{
	register THING *obj;
	register byte ch;
	byte och;
	static byte lch;
	static THING *wasthing = NULL;
	byte gi_state;	/* get item sub state */
	int once_only = FALSE;

	if (((!strncmp(s_menu,"sel",3) && strcmp(purpose,"eat")
	  && strcmp(purpose,"drop"))) || !strcmp(s_menu,"on"))
		once_only = TRUE;

	gi_state = again;
	if (pack == NULL)
		msg("you aren't carrying anything");
	else {
		ch = lch;
		for (;;) {
			/*
			 * if we are doing something AGAIN, and the pack hasn't
			 * changed then don't ask just give him the same thing
			 * he got on the last command.
			 */
			if (gi_state && wasthing == pack_obj(ch, &och))
				goto skip;
			if (once_only) {
				ch = '*';
				goto skip;
			}
			if (!terse && !expert)
				addmsg("which object do you want to ");
			msg("%s? (* for list): ",purpose);
			/*
			 * ignore any alt characters that may be typed
			 */
			ch = readchar();
			skip:
			mpos = 0;
			gi_state = FALSE;
			once_only = FALSE;
			if (ch == '*') {
				if ((ch = inventory(pack, type, purpose)) == 0) {
					after = FALSE;
					return NULL;
				}
				if (ch == ' ')
					continue;
				lch = ch;
			}
			/*
			 * Give the poor player a chance to abort the command
			 */
			if (ch == ESCAPE) {
				after = FALSE;
				msg("");
				return NULL;
			}
			if ((obj = pack_obj(ch, &och)) == NULL) {
				ifterse1("range is 'a' to '%c'","please specify a letter between 'a' and '%c'", och-1);
				continue;
			} else {
				/*
				 * If you find an object reset flag because
				 * you really don't know if the object he is getting
				 * is going to change the pack.  If he detaches the
				 * thing from the pack later this flag will get set.
				 */
				if (strcmp(purpose, "identify")) {
					lch = ch;
					wasthing = obj;
				}
				return obj;
		   }
		}
	}
	return NULL;
}

/*
 * pack_char:
 *	Return which character would address a pack object
 */
byte
pack_char(THING *obj)
{
	register THING *item;
	register byte c;

	c = 'a';
	for (item = pack; item != NULL; item = next(item))
		if (item == obj)
			return c;
		else
			c++;
	return '?';
}

/*
 * money:
 *	Add or subtract gold from the pack
 */
void
money(int value)
{
	register byte floor;

	floor = (proom->r_flags & ISGONE) ? PASSAGE : FLOOR;
	purse += value;
	mvaddch(hero.y, hero.x, floor);
	chat(hero.y, hero.x) = floor;
	if (value > 0)
	{
		msg("you found %d gold pieces", value);
	}
}
