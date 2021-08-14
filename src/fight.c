/*
 * All the fighting gets done here
 *
 * @(#)fight.c		1.43 (AI Design)		1/19/85
 */

#include "rogue.h"
#include "curses.h"

/*
 * fight:
 *	The player attacks the monster.
 */
bool
fight(coord *mp, char mn, THING *weap, bool thrown)
{
	register THING *tp;
	register char *mname;

	/*
	 * Find the monster we want to fight
	 */
	if ((tp = moat(mp->y, mp->x)) == 0)
		return FALSE;
	/*
	 * Since we are fighting, things are not quiet so no healing takes
	 * place.  Cancel any command counts so player can recover.
	 */
	count = quiet = 0;
	start_run(mp);
	/*
	 * Let him know it was really a mimic (if it was one).
	 */
	if (tp->t_type == 'X' && tp->t_disguise != 'X' && !on(player, ISBLIND)) {
		mn = tp->t_disguise = 'X';
		if (thrown)
			return FALSE;
		msg("wait! That's a Xeroc!");
	}
	mname = monsters[mn-'A'].m_name;
	if (on(player, ISBLIND))
		mname = it;
	if (roll_em(&player, tp, weap, thrown)||(weap && weap->o_type == POTION)) {
		bool did_huh = FALSE;

		if (thrown)
			thunk(weap, mname, "hits", "hit");
		else
			hit(NULL, mname);
		//@ original missed NULL check for weap
		if (weap && weap->o_type == POTION) {
			th_effect(weap, tp);
			if (!thrown) {
				if (weap->o_count > 1)
					weap->o_count--;
				else {
					detach(pack, weap);
					discard(weap);
				}
				cur_weapon = NULL;
			}
		}
		if (on(player, CANHUH)) {
			did_huh = TRUE;
			tp->t_flags |= ISHUH;
			player.t_flags &= ~CANHUH;
			msg("your hands stop glowing red");
		}
		if (tp->t_stats.s_hpt <= 0)
			killed(tp, TRUE);
		else if (did_huh && !on(player, ISBLIND))
			msg("the %s appears confused", mname);
		return TRUE;
	}
	if (thrown)
		thunk(weap, mname, "misses", "missed");
	else
		miss(NULL, mname);
	if (tp->t_type == 'S' && rnd(100) > 25)
		slime_split(tp);
	return FALSE;
}

/*
 * attack:
 *	The monster attacks the player
 */
void
attack(THING *mp)
{
	register char *mname;

	/*
	 * Since this is an attack, stop running and any healing that was
	 * going on at the time.
	 */
	running = FALSE;
	count = quiet = 0;
	if (mp->t_type == 'X' && !on(player, ISBLIND))
		mp->t_disguise = 'X';
	mname = monsters[mp->t_type-'A'].m_name;
	if (on(player, ISBLIND))
		mname = it;
	if (roll_em(mp, &player, NULL, FALSE)) {
		hit(mname, NULL);
		if (pstats.s_hpt <= 0)
			death(mp->t_type);	/* Bye bye life ... */
		if (!on(*mp, ISCANC))
			switch (mp->t_type)
		{
		when 'A':
			/*
			 * If a rust monster hits, you lose armor, unless
			 * that armor is leather or there is a magic ring
			 */
			if (cur_armor != NULL && cur_armor->o_ac < 9
			  && cur_armor->o_which != LEATHER)
			{
				if (ISWEARING(R_SUSTARM))
					msg("the rust vanishes instantly");
				else
				{
					msg("your armor weakens, oh my!");
					cur_armor->o_ac++;
				}
			}
		when 'I':
			/*
			 * When an Ice Monster hits you, you get unfrozen faster
			 */
			if (no_command > 1)
				no_command--;
			break;
		when 'R':
			/*
			 * Rattlesnakes have poisonous bites
			 */
			if (!save(VS_POISON))
			{
				if (!ISWEARING(R_SUSTSTR))
				{
					chg_str(-1);
					msg("you feel a bite in your leg%s",
						noterse(" and now feel weaker"));
				}
				else
					msg("a bite momentarily weakens you");
			}
		when 'W':
		case 'V':
			/*
			 * Wraiths might drain energy levels, and Vampires
			 * can steal max_hp
			 */
			if (rnd(100) < (mp->t_type == 'W' ? 15 : 30))
			{
			register int fewer;

			if (mp->t_type == 'W')
			{
				if (pstats.s_exp == 0)
				death('W');		/* All levels gone */
				if (--pstats.s_lvl == 0)
				{
				pstats.s_exp = 0;
				pstats.s_lvl = 1;
				}
				else
				pstats.s_exp = e_levels[pstats.s_lvl-1]+1;
				fewer = roll(1, 10);
			}
			else
				fewer = roll(1, 5);
			pstats.s_hpt -= fewer;
			max_hp -= fewer;
			if (pstats.s_hpt < 1)
				pstats.s_hpt = 1;
			if (max_hp < 1)
				death(mp->t_type);
			msg("you suddenly feel weaker");
			}
		when 'F':
			/*
			 * Violet fungi stops the poor guy from moving
			 */
			player.t_flags |= ISHELD;
			sprintf(mp->t_stats.s_dmg,"%dd1",++fung_hit);
		when 'L':
		{
			/*
			 * Leperachaun steals some gold
			 */
			register long lastpurse;

			lastpurse = purse;
			purse -= GOLDCALC;
			if (!save(VS_MAGIC))
			purse -= GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
			if (purse < 0)
			purse = 0;
			remove_monster(&mp->t_pos, mp, FALSE);
			if (purse != lastpurse)
			msg("your purse feels lighter");
		}
		when 'N':
		{
			register THING *obj, *steal;
			register int nobj;
			char *she_stole = "she stole %s!";

			/*
			 * Nymph's steal a magic item, look through the pack
			 * and pick out one we like.
			 */
			steal = NULL;
			for (nobj = 0, obj = pack; obj != NULL; obj = next(obj))
			if (obj != cur_armor && obj != cur_weapon
				&& obj != cur_ring[LEFT] && obj != cur_ring[RIGHT]
				&& is_magic(obj) && rnd(++nobj) == 0)
				steal = obj;
			if (steal != NULL)
			{
				remove_monster(&mp->t_pos, mp, FALSE);
				inpack--;
				if (steal->o_count > 1 && steal->o_group == 0)
				{
					register int oc;

					oc = steal->o_count--;
					steal->o_count = 1;
					msg(she_stole, inv_name(steal, TRUE));
					steal->o_count = oc;
				}
				else
				{
					detach(pack, steal);
					discard(steal);
					msg(she_stole, inv_name(steal, TRUE));
				}
			}
		}
		otherwise:
			break;
		}
	}
	else if (mp->t_type != 'I')
	{
	if (mp->t_type == 'F')
	{
		pstats.s_hpt -= fung_hit;
		if (pstats.s_hpt <= 0)
		death(mp->t_type);	/* Bye bye life ... */
	}
	miss(mname, NULL);
	}
	flush_type();
	count = 0;
	status();
}

/*
 * swing:
 *	Returns true if the swing hits
 */
bool
swing(int at_lvl, int op_arm, int wplus)
{
	register int res = rnd(20);
	register int need = (20 - at_lvl) - op_arm;

	return (res + wplus >= need);
}

/*
 * check_level:
 *	Check to see if the guy has gone up a level.
 */
void
check_level(void)
{
	register int i, add, olevel;

	for (i = 0; e_levels[i] != 0; i++)
	if (e_levels[i] > pstats.s_exp)
		break;
	i++;
	olevel = pstats.s_lvl;
	pstats.s_lvl = i;
	if (i > olevel)
	{
		add = roll(i - olevel, 10);
		max_hp += add;
		if ((pstats.s_hpt += add) > max_hp)
			pstats.s_hpt = max_hp;
		msg("and achieve the rank of \"%s\"", he_man[i-1]);
	}
}

/*
 * roll_em:
 *	Roll several attacks
 */
bool
roll_em(THING *thatt, THING *thdef, THING *weap, bool hurl)
{
	register struct stats *att, *def;
	char *cp;
	int ndice, nsides, def_arm;
	register bool did_hit = FALSE;
	register int hplus;
	register int dplus;
	register int damage;
	att = &thatt->t_stats;
	def = &thdef->t_stats;
	if (weap == NULL)
	{
		cp = att->s_dmg;
		dplus = 0;
		hplus = 0;
	}
	else
	{
		hplus = weap->o_hplus;
		dplus = weap->o_dplus;
		/*
		 * Check for vorpally enchanted weapon
		 */
		if (thdef->t_type == weap->o_enemy)
		{
			hplus += 4;
			dplus += 4;
		}
		if (weap == cur_weapon)
		{
			if (ISRING(LEFT, R_ADDDAM))
				dplus += cur_ring[LEFT]->o_ac;
			else if (ISRING(LEFT, R_ADDHIT))
				hplus += cur_ring[LEFT]->o_ac;
			if (ISRING(RIGHT, R_ADDDAM))
				dplus += cur_ring[RIGHT]->o_ac;
			else if (ISRING(RIGHT, R_ADDHIT))
				hplus += cur_ring[RIGHT]->o_ac;
		}
		cp = weap->o_damage;
		if (hurl && (weap->o_flags&ISMISL) && cur_weapon != NULL &&
			  cur_weapon->o_which == weap->o_launch)
		{
			cp = weap->o_hurldmg;
			hplus += cur_weapon->o_hplus;
			dplus += cur_weapon->o_dplus;
		}
		/*
		 * Drain a staff of striking
		 */
		if (weap->o_type == STICK && weap->o_which == WS_HIT
			&& --weap->o_charges < 0)
		{
			cp = weap->o_damage = "0d0";
			weap->o_hplus = weap->o_dplus = 0;
			weap->o_charges = 0;
		}
	}

	//@ New NULL check to prevent segfault on atoi()
	if (cp == NULL)
	{
		return FALSE;
	}

	/*
	 * If the creature being attacked is not running (alseep or held)
	 * then the attacker gets a plus four bonus to hit.
	 */
	if (!on(*thdef, ISRUN))
		hplus += 4;
	def_arm = def->s_arm;
	if (def == &pstats)
	{
		if (cur_armor != NULL)
			def_arm = cur_armor->o_ac;
		if (ISRING(LEFT, R_PROTECT))
			def_arm -= cur_ring[LEFT]->o_ac;
		if (ISRING(RIGHT, R_PROTECT))
			def_arm -= cur_ring[RIGHT]->o_ac;
	}
	for (;;)
	{
		ndice = atoi(cp);
		if ((cp = stpchr(cp, 'd')) == NULL)
			break;
		nsides = atoi(++cp);
		if (swing(att->s_lvl, def_arm, hplus + str_plus(att->s_str)))
		{
			register int proll;

			proll = roll(ndice, nsides);
			damage = dplus + proll + add_dam(att->s_str);
			/*
			 * special goodies for the commercial version of rogue
			 */
				if (thdef == &player && max_level == 1)
				 /*
				  * make it easier on level one
				  */
						damage = (damage+1) / 2;
				/*
				 * copy protection goodies
				 */
				if (thdef == &player)
					damage *= hit_mul;
			def->s_hpt -= max(0, damage);
			did_hit = TRUE;
		}
		if ((cp = stpchr(cp, '/')) == NULL)
			break;
		cp++;
	}
	return did_hit;
}

//@ No need to declare in rogue.h
/*
 * prname:
 *	The print name of a combatant
 */
char *
prname(char *who, bool upper)
{
	*tbuf = '\0';
	if (who == 0)
		strcpy(tbuf, you);
	else if (on(player, ISBLIND))
		strcpy(tbuf, it);
	else
	{
		strcpy(tbuf, "the ");
		strcat(tbuf, who);
	}
	if (upper)
		*tbuf = toupper(*tbuf);
	return tbuf;
}

/*
 * hit:
 *	Print a message to indicate a succesful hit
 */
void
hit(char *er, char *ee)
{
	register char *s = "";

	addmsg(prname(er, TRUE));
	switch ((terse || expert) ? 1 : rnd(4))
	{
		when 0: s = " scored an excellent hit on ";
		when 1: s = " hit ";
		when 2: s = (er == 0 ? " have injured " : " has injured ");
		when 3: s = (er == 0 ? " swing and hit " : " swings and hits ");
		break;
	}
	msg("%s%s",s,prname(ee, FALSE));
}

/*
 * miss:
 *	Print a message to indicate a poor swing
 */
void
miss(char *er, char *ee)
{
	register char *s = "";


	addmsg(prname(er, TRUE));
	switch ((terse || expert) ? 1 : rnd(4))
	{
		when 0: s = (er == 0 ? " swing and miss" : " swings and misses");
		when 1: s = (er == 0 ? " miss" : " misses");
		when 2: s = (er == 0 ? " barely miss" : " barely misses");
		when 3: s = (er == 0 ? " don't hit" : " doesn't hit");
		break;
	}
	msg("%s %s",s,prname(ee, FALSE));
}

/*
 * save_throw:
 *	See if a creature save against something
 */
bool
save_throw(int which, THING *tp)
{
	register int need;

	need = 14 + which - tp->t_stats.s_lvl / 2;
	return (roll(1, 20) >= need);
}

/*
 * save:
 *	See if he saves against various nasty things
 */
bool
save(int which)
{
	if (which == VS_MAGIC) {
		if (ISRING(LEFT, R_PROTECT))
			which -= cur_ring[LEFT]->o_ac;
		if (ISRING(RIGHT, R_PROTECT))
			which -= cur_ring[RIGHT]->o_ac;
	}
	return save_throw(which, &player);
}

/*
 * str_plus:
 *	Compute bonus/penalties for strength on the "to hit" roll
 */
int
str_plus(str_t str)
{
	register int add = 4;

	if (str < 8)
		return str - 7;
	if (str < 31)
		add--;
	if (str < 21)
		add--;
	if (str < 19)
		add--;
	if (str < 17)
		add--;
	return add;
}

/*
 * add_dam:
 *	Compute additional damage done for exceptionally high or low strength
 */
int
add_dam(str_t str)
{
	int add = 6;

	if (str < 8)
		return str - 7;
	if (str < 31)
		add--;
	if (str < 22)
		add--;
	if (str < 20)
		add--;
	if (str < 18)
		add--;
	if (str < 17)
		add--;
	if (str < 16)
		add--;
	return add;
}

/*
 * raise_level:
 *	The guy just magically went up a level.
 */
void
raise_level(void)
{
	pstats.s_exp = e_levels[pstats.s_lvl-1] + 1L;
	check_level();
}

/*
 * thunk:
 *	A missile hit or missed a monster
 */
void
thunk(THING *weap, char *mname, char *does, char *did)
{
	if (weap->o_type == WEAPON)
		addmsg("the %s %s ", w_names[weap->o_which], does);
	else
		addmsg("you %s ", did);
	if (on(player, ISBLIND))
		msg(it);
	else
		msg("the %s", mname);
}

//@ renamed from remove() to avoid conflict with <stdio.h>
/*
 * remove_monster:
 *	Remove a monster from the screen
 */
void
remove_monster(coord *mp, THING *tp, bool waskill)
{
	register THING *obj, *nexti;

	if (tp == NULL)
		return;

	for (obj = tp->t_pack; obj != NULL; obj = nexti)
	{
		nexti = next(obj);
		bcopy(obj->o_pos,tp->t_pos);
		detach(tp->t_pack, obj);
		if (waskill)
			fall(obj, FALSE);
		else
			discard(obj);
	}
	if (_level[INDEX(mp->y,mp->x)] == PASSAGE)
		standout();
	if (tp->t_oldch == FLOOR && !cansee(mp->y, mp->x))
		mvaddch(mp->y, mp->x, ' ');
	else if (tp->t_oldch != '@')
		mvaddch(mp->y, mp->x, tp->t_oldch);
	standend();
	detach(mlist, tp);
	discard(tp);
}

/*
 * is_magic:
 *	Returns true if an object radiates magic
 */
bool
is_magic(THING *obj)
{
	switch (obj->o_type)
	{
	case ARMOR:
		return obj->o_ac != a_class[obj->o_which];
	case WEAPON:
		return obj->o_hplus != 0 || obj->o_dplus != 0;
	case POTION:
	case SCROLL:
	case STICK:
	case RING:
	case AMULET:
		return TRUE;
	}
	return FALSE;
}

/*
 * killed:
 *	Called to put a monster to death
 */
void
killed(THING *tp, bool pr)
{
	pstats.s_exp += tp->t_stats.s_exp;
	/*
	 * If the monster was a violet fungi, un-hold him
	 */
	switch (tp->t_type)
	{
	when 'F':
		player.t_flags &= ~ISHELD;
		f_restor();
	when 'L':;
		register THING *gold;

		if ((gold = new_item()) == NULL)
			return;
		gold->o_type = GOLD;
		gold->o_goldval = GOLDCALC;
		if (save(VS_MAGIC))
			gold->o_goldval += GOLDCALC + GOLDCALC + GOLDCALC + GOLDCALC;
		attach(tp->t_pack, gold);
		break;
	}
	/*
	 * Get rid of the monster.
	 */
	remove_monster(&tp->t_pos, tp, TRUE);
	if (pr)
	{
	addmsg("you have defeated ");
	if (on(player, ISBLIND))
		msg(it);
	else
		msg("the %s", monsters[tp->t_type-'A'].m_name);
	}
	/*
	 * Do adjustments if he went up a level
	 */
	check_level();
}
