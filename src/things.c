/*
 * Contains functions for dealing with things like potions, scrolls,
 * and other items.
 *
 * things.c	1.4 (AI Design)	12/14/84
 */

#include "rogue.h"
#include "curses.h"

static void	chopmsg(char *s, char *shmsg, char *lnmsg, ...);
static void	print_disc(byte type);
static void	set_order(short *order, int numthings);
static int	pick_one(struct magic_item *magic, int nitems);
static char	*nothing(byte type);

/*
 * inv_name:
 *	Return the name of something as it would appear in an
 *	inventory.
 */
char *
inv_name(THING *obj, bool drop)
{
	register int which = obj->o_which;
	register char *pb;

	pb = prbuf;
	switch (obj->o_type)
	{
	when SCROLL:
		if (obj->o_count == 1) {
			strcpy(pb, "A scroll ");
			pb = &prbuf[9];
		} else {
			sprintf(pb, "%d scrolls ", obj->o_count);
			pb = &prbuf[strlen(prbuf)];
		}
		if (s_know[which])
			sprintf(pb, "of %s", s_magic[which].mi_name);
		else if (*s_guess[which])
			sprintf(pb, "called %s", s_guess[which]);
		else
			chopmsg(pb, "titled '%.17s'","titled '%s'", &s_names[which]);
	when POTION:
		if (obj->o_count == 1)
		{
			strcpy(pb, "A potion ");
			pb = &prbuf[9];
		}
		else
		{
			sprintf(pb, "%d potions ", obj->o_count);
			pb = &pb[strlen(prbuf)];
		}
		if (p_know[which]) {
			chopmsg(pb, "of %s", "of %s(%s)",
				p_magic[which].mi_name, p_colors[which]);
		}
		else if (*p_guess[which]) {
			chopmsg(pb, "called %s","called %s(%s)", p_guess[which],
				p_colors[which]);
		}
		else if (obj->o_count == 1)
			sprintf(prbuf, "A%s %s potion", vowelstr(p_colors[which]),
				p_colors[which]);
		else
			sprintf(prbuf, "%d %s potions", obj->o_count, p_colors[which]);
	when FOOD:
		if (which == 1)
			if (obj->o_count == 1)
				sprintf(pb, "A%s %s", vowelstr(fruit), fruit);
			else
				sprintf(pb, "%d %ss", obj->o_count, fruit);
		else
			if (obj->o_count == 1)
				strcpy(pb, "Some food");
			else
				sprintf(pb, "%d rations of food", obj->o_count);
	when WEAPON:
		if (obj->o_count > 1)
			sprintf(pb, "%d ", obj->o_count);
		else
			sprintf(pb, "A%s ", vowelstr(w_names[which]));
		pb = &prbuf[strlen(prbuf)];
		if (obj->o_flags & ISKNOW)
			sprintf(pb, "%s %s", num(obj->o_hplus, obj->o_dplus, WEAPON),
				w_names[which]);
		else
			sprintf(pb, "%s", w_names[which]);
		if (obj->o_count > 1)
			strcat(pb, "s");
		if (obj->o_enemy && (obj->o_flags & ISREVEAL))
		{
			strcat(pb, " of ");
			strcat(pb, monsters[obj->o_enemy-'A'].m_name);
			strcat(pb, " slaying");
		}
	when ARMOR:
		if (obj->o_flags & ISKNOW)
			chopmsg(pb, "%s %s","%s %s [armor class %d]",
				num(a_class[which] - obj->o_ac, 0, ARMOR),
				a_names[which], -(obj->o_ac-11));
		else
			sprintf(pb, "%s", a_names[which]);
	when AMULET:
		strcpy(pb, "The Amulet of Yendor");
	when STICK:
		sprintf(pb, "A%s %s ", vowelstr(ws_type[which]),
		ws_type[which]);
		pb = &prbuf[strlen(prbuf)];
		if (ws_know[which])
			chopmsg(pb, "of %s%s", "of %s%s(%s)",
				ws_magic[which].mi_name,
				charge_str(obj), ws_made[which]);
		else if (*ws_guess[which])
			chopmsg(pb, "called %s", "called %s(%s)", ws_guess[which],
				ws_made[which]);
		else
			sprintf(pb = &prbuf[2], "%s %s", ws_made[which], ws_type[which]);
	when RING:
		if (r_know[which])
			chopmsg(pb, "A%s ring of %s", "A%s ring of %s(%s)", ring_num(obj),
				r_magic[which].mi_name, r_stones[which]);
		else if (*r_guess[which])
			chopmsg(pb, "A ring called %s", "A ring called %s(%s)",
				r_guess[which], r_stones[which]);
		else
			sprintf(pb, "A%s %s ring", vowelstr(r_stones[which]),
				r_stones[which]);
#ifdef DEBUG
	when GOLD:
		sprintf(pb, "Gold at %d,%d", obj->o_pos.y, obj->o_pos.x);
	otherwise:
		debug("Picked up someting bizzare %s", io_unctrl(obj->o_type));
		sprintf(pb, "Something bizarre %c(%d)", obj->o_type, obj->o_type);
#endif
		break;
	}
	if (obj == cur_armor)
		strcat(pb, " (being worn)");
	if (obj == cur_weapon)
		strcat(pb, " (weapon in hand)");
	if (obj == cur_ring[LEFT])
		strcat(pb, " (on left hand)");
	else if (obj == cur_ring[RIGHT])
		strcat(pb, " (on right hand)");
	if (drop && ismonster(prbuf[0]))
		prbuf[0] = tolower(prbuf[0]);
	else if (!drop && is_lower(*prbuf))
		*prbuf = toupper(*prbuf);
	return prbuf;
}

//@ changed original signature to use varargs
static
void
chopmsg(char *s, char *shmsg, char *lnmsg, ...)
{
	va_list argp;
	va_start(argp, lnmsg);
	vsnprintf(s, MAXSTR, (terse || expert) ? shmsg : lnmsg, argp);
	va_end(argp);
}

/*
 * drop:
 *	Put something down
 */
void
drop(void)
{
	register byte ch;
	register THING *nobj, *op;

	ch = chat(hero.y, hero.x);
	if (ch != FLOOR && ch != PASSAGE)
	{
		msg("there is something there already");
		return;
	}
	if ((op = get_item("drop", 0)) == NULL)
		return;
	if (!can_drop(op))
		return;
	/*
	 * Take it out of the pack
	 */
	if (op->o_count >= 2 && op->o_type != WEAPON)
	{
		if ((nobj = new_item()) == NULL)
		{
			msg("%sit appears to be stuck in your pack!",
				noterse("can't drop it, "));
			return;
		}
		op->o_count--;
		bcopy(*nobj,*op);
		nobj->o_count = 1;
		op = nobj;
		if (op->o_group != 0)
			inpack++;
	}
	else
		detach(pack, op);
	inpack--;
	/*
	 * Link it into the level object list
	 */
	attach(lvl_obj, op);
	chat(hero.y, hero.x) = op->o_type;
	bcopy(op->o_pos,hero);
	if (op->o_type == AMULET)
		amulet = FALSE;
	msg("dropped %s", inv_name(op, TRUE));
}

/*
 * can_drop:
 *	Do special checks for dropping or unweilding|unwearing|unringing
 */
bool
can_drop(THING *op)
{
	if (op == NULL)
		return TRUE;
	if (op != cur_armor && op != cur_weapon
		&& op != cur_ring[LEFT] && op != cur_ring[RIGHT])
		return TRUE;
	if (op->o_flags & ISCURSED) {
		msg("you can't.  It appears to be cursed");
		return FALSE;
	}
	if (op == cur_weapon)
		cur_weapon = NULL;
	else if (op == cur_armor) {
		waste_time();
		cur_armor = NULL;
	} else {
		register int hand;

		if (op != cur_ring[hand = LEFT])
			if (op != cur_ring[hand = RIGHT]) {
#ifdef DEBUG
				debug("Candrop called with funny thing");
#endif
				return TRUE;
			}
		cur_ring[hand] = NULL;
		switch (op->o_which) {
		case R_ADDSTR:
			chg_str(-op->o_ac);
			break;
		case R_SEEINVIS:
			unsee();
			extinguish(unsee);
			break;
		}
	}
	return TRUE;
}

/*
 * new_thing:
 *	Return a new thing
 */
THING *
new_thing(void)
{
	register THING *cur;
	register int j, k;

	if ((cur = new_item()) == NULL)
		return NULL;
	cur->o_hplus = cur->o_dplus = 0;
	cur->o_damage = cur->o_hurldmg = "0d0";
	cur->o_ac = 11;
	cur->o_count = 1;
	cur->o_group = 0;
	cur->o_flags = 0;
	cur->o_enemy = 0;
	/*
	 * Decide what kind of object it will be
	 * If we haven't had food for a while, let it be food.
	 */
	switch (no_food > 3 ? 2 : pick_one(things, NUMTHINGS))
	{
	when 0:
		cur->o_type = POTION;
		cur->o_which = pick_one(p_magic, MAXPOTIONS);
	when 1:
		cur->o_type = SCROLL;
		cur->o_which = pick_one(s_magic, MAXSCROLLS);
	when 2:
		no_food = 0;
		cur->o_type = FOOD;
		if (rnd(10) != 0)
			cur->o_which = 0;
		else
			cur->o_which = 1;
	when 3:
		cur->o_type = WEAPON;
		cur->o_which = rnd(MAXWEAPONS);
		init_weapon(cur, cur->o_which);
		if ((k = rnd(100)) < 10)
		{
			cur->o_flags |= ISCURSED;
			cur->o_hplus -= rnd(3) + 1;
		}
		else if (k < 15)
			cur->o_hplus += rnd(3) + 1;
	when 4:
		cur->o_type = ARMOR;
		for (j = 0, k = rnd(100); j < MAXARMORS; j++)
			if (k < a_chances[j])
				break;
#ifdef DEBUG
		if (j == MAXARMORS)
		{
		debug("Picked a bad armor %d", k);
		j = 0;
		}
#endif
		cur->o_which = j;
		cur->o_ac = a_class[j];
		if ((k = rnd(100)) < 20)
		{
			cur->o_flags |= ISCURSED;
			cur->o_ac += rnd(3) + 1;
		}
		else if (k < 28)
			cur->o_ac -= rnd(3) + 1;
	when 5:
		cur->o_type = RING;
		cur->o_which = pick_one(r_magic, MAXRINGS);
		switch (cur->o_which)
		{
		when R_ADDSTR:
		case R_PROTECT:
		case R_ADDHIT:
		case R_ADDDAM:
			if ((cur->o_ac = rnd(3)) == 0)
			{
				cur->o_ac = -1;
				cur->o_flags |= ISCURSED;
			}
		when R_AGGR:
		case R_TELEPORT:
			cur->o_flags |= ISCURSED;
			break;
		}
	when 6:
		cur->o_type = STICK;
		cur->o_which = pick_one(ws_magic, MAXSTICKS);
		fix_stick(cur);
#ifdef DEBUG
	otherwise:
		debug("Picked a bad kind of object");
		wait_for(' ');
#endif
		break;
	}
	return cur;
}

/*
 * pick_one:
 *	Pick an item out of a list of nitems possible magic items
 */
static
shint  //@ actually an offset, the element index in the array
pick_one(struct magic_item *magic, int nitems)
{
	register struct magic_item *end;
	register int i;
	register struct magic_item *start;

	start = magic;
	for (end = &magic[nitems], i = rnd(100); magic < end; magic++)
		if (i < magic->mi_prob)
			break;
	if (magic == end)
	{
#ifdef DEBUG
		if (wizard)
		{
			msg("bad pick_one: %d from %d items", i, nitems);
			for (magic = start; magic < end; magic++)
				msg("%s: %d%%", magic->mi_name, magic->mi_prob);
		}
#endif
		magic = start;
	}
	return magic - start;
}

/*
 * discovered:
 *	list what the player has discovered in this game of a certain type
 */
static int line_cnt = 0;

static bool newpage = FALSE;

static char *lastfmt, *lastarg;

void
discovered(void)
{
	print_disc(POTION);
	add_line(nullstr, " ", "");
	print_disc(SCROLL);
	add_line(nullstr, " ", "");
	print_disc(RING);
	add_line(nullstr, " ", "");
	print_disc(STICK);
	end_line(nullstr);
}

/*
 * print_disc:
 *	Print what we've discovered of type 'type'
 */

#define MAX(a,b,c,d) (a>b?(a>c?(a>d?a:d):(c>d?c:d)):(b>c?(b>d?b:d):(c>d?c:d)))

static
void
print_disc(byte type)
{
	register bool *know = NULL;
	register char **guess = NULL;
	register int i, maxnum = 0, num_found;
	static THING obj;
	static short order[MAX(MAXSCROLLS, MAXPOTIONS, MAXRINGS, MAXSTICKS)];

	switch (type)
	{
	case SCROLL:
		maxnum = MAXSCROLLS;
		know = s_know;
		guess = s_guess;
		break;
	case POTION:
		maxnum = MAXPOTIONS;
		know = p_know;
		guess = p_guess;
		break;
	case RING:
		maxnum = MAXRINGS;
		know = r_know;
		guess = r_guess;
		break;
	case STICK:
		maxnum = MAXSTICKS;
		know = ws_know;
		guess = ws_guess;
		break;
	}
	set_order(order, maxnum);
	obj.o_count = 1;
	obj.o_flags = 0;
	num_found = 0;
	for (i = 0; i < maxnum; i++)
		if (know[order[i]] || *guess[order[i]])
		{
			obj.o_type = type;
			obj.o_which = order[i];
			add_line(nullstr, "%s", inv_name(&obj, FALSE));
			num_found++;
		}
	if (num_found == 0)
		add_line(nullstr, nothing(type), "");
}

/*
 * set_order:
 *	Set up order for list
 */
static
void
set_order(short *order, int numthings)
{
	register int i, r, t;

	for (i = 0; i< numthings; i++)
		order[i] = i;

	for (i = numthings; i > 0; i--)
	{
		r = rnd(i);
		t = order[i - 1];
		order[i - 1] = order[r];
		order[r] = t;
	}
}

/*
 * add_line:
 *	Add a line to the list of discoveries
 *
 * VARARGS1
 */
byte
add_line(char *use, char *fmt, char *arg)
{
	int x, y;
	register byte retchar = ' ';
	if (line_cnt == 0)
	{
		wdump();
		clear();
	}
	if (line_cnt >= LINES - 1 || fmt == NULL)
	{
		move(LINES-1, 0);
		if (*use)
			printw("-Select item to %s. Esc to cancel-", use);
		else
			addstr("-Press space to continue-");
		do
			retchar = readchar();
		while (retchar != ESCAPE && retchar != ' ' && (!is_lower(retchar)));
		clear();
		newpage = TRUE;
		line_cnt = 0;
	}
	if (fmt != NULL && !(line_cnt == 0 && *fmt == '\0'))
	{
		move(line_cnt, 0);
		printw(fmt, arg);
		getxy(&x,&y);
		/*
		 * if the line wrapped but nothing was printed on this
		 * line you might as well use it for the next item
		 */
		if (y!=0)
			line_cnt = x + 1;
		lastfmt = fmt;
		lastarg = arg;
	}
	return(retchar);
}

/*
 * end_line:
 *	End the list of lines
 */
byte
end_line(char *use)
{
	register int retchar;

	retchar = add_line(use, NULL, "");
	wrestor();
	line_cnt = 0;
	newpage = FALSE;
	return(retchar);
}

/*
 * nothing:
 *	Set up prbuf so that message for "nothing found" is there
 */
static
char *
nothing(byte type)
{
	register char *sp, *tystr;

	sprintf(prbuf, "Haven't discovered anything");
	if (terse)
		sprintf(prbuf,"Nothing");
	sp = &prbuf[strlen(prbuf)];
	switch (type)
	{
		when POTION: tystr = "potion";
		when SCROLL: tystr = "scroll";
		when RING: tystr = "ring";
		when STICK: tystr = "stick";
		otherwise: tystr = NULL; //@ silence maybe-uninitialized warning
	}
	sprintf(sp, " about any %ss", tystr);
	return prbuf;
}
