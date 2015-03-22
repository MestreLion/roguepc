/*
 * Code for handling the various special properties of the slime
 *
 * slime.c	1.0		(A.I. Design 1.42)	1/17/85
 */

#include	"rogue.h"
#include	"curses.h"

/*
 * Slime_split:
 *	Called when it has been decided that A slime should divide itself
 */

static coord slimy;

static bool	new_slime(THING *tp);

void
slime_split(tp)
	THING *tp;
{
	register THING *nslime;

	if (!new_slime(tp) || (nslime = new_item()) == NULL)
		return;
	msg("The slime divides.  Ick!");
	new_monster(nslime, 'S', &slimy);
	if (cansee(slimy.y, slimy.x)) {
		nslime->t_oldch = chat(slimy.y, slimy.x);
		mvaddch(slimy.y, slimy.x, 'S');
	}
	start_run(&slimy);
}

static
bool
new_slime(tp)
	THING *tp;
{
	register int y, x, ty, tx;
	register bool ret;
	THING *ntp;
	coord sp;

	ret = FALSE;
	tp->t_flags |= ISFLY;
	if (!plop_monster((ty = tp->t_pos.y), (tx = tp->t_pos.x), &sp)) {
		/*
		 * There were no open spaces next to this slime, look for other
		 * slimes that might have open spaces next to them.
		 */
		for (y = ty -1; y <= ty+1; y++)
			for (x = tx-1; x <= tx+1; x++)
				if (winat(y, x) == 'S' && (ntp = moat(y, x))) {
					if (ntp->t_flags & ISFLY)
						continue;				/* Already done this one */
					if (new_slime(ntp)) {
						y = ty+2;
						x = tx +2;
					}
				}
	} else {
		ret = TRUE;
		slimy = sp;
	}
	tp->t_flags &= ~ISFLY;
	return ret;
}

/*@
 * Pick an appropriate spot around a central spot for a new monster to spawn
 * (r, c): row, col of central spot
 * cp: pointer to coordinate for the new monster, if any
 * Return FALSE if no suitable spot around (r, c) is found
 *
 * Original return value was somewhat an abuse of the bool convention,
 * used both as TRUE/FALSE and as an integer for calculating odds.
 * To avoid that, 'inv_odds' was created for the rnd() call,
 * and 'appear' is now "strictly" boolean
 */
bool
plop_monster(r, c, cp)
	int r, c;
	coord *cp;
{
	register int y, x, inv_odds = 0;
	bool appear = FALSE;
	byte ch;

	for (y = r-1; y <= r+1; y++)
		for (x = c-1; x <= c+1; x++) {
			/*
			 * Don't put a monster in top of the player.
			 */
			if ((y == hero.y && x == hero.x) || offmap(y,x))
				continue;
			/*
			 * Or anything else nasty
			 */
			if (step_ok(ch = winat(y, x))) {
				if (ch == SCROLL && find_obj(y, x)->o_which == S_SCARE)
					continue;
				/*@
				 * Get first available spot with 100% chance,
				 * then randomly change to next available spot, if any,
				 * with decreasing 1-to-n odds (50%, 33%, 25%, 20%,...)
				 */
				appear = TRUE;
				if (rnd(++inv_odds) == 0) {
					cp->y = y;
					cp->x = x;
				}
			}
		}
	return appear;
}
