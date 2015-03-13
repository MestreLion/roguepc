/*
 * Various input/output functions
 *
 * io.c		1.4		(A.I. Design) 12/10/84
 */

#include	"rogue.h"
#include	"curses.h"

#define AC(a) (-((a)-11))
#define PT(i,j) ((COLS==40)?i:j)
/*
 * msg:
 *	Display a message at the top of the screen.
 */
static int newpos = 0;

/* VARARGS1 */
/*@ nope, it was not vargars. But now it is */
void
ifterse(const char *tfmt, const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	if (expert)
		vmsg(tfmt, argp);
	else
		vmsg(fmt, argp);

	va_end(argp);
}

//@ va_list variant of msg()
void
vmsg(const char *fmt, va_list argp)
{
	/*
	 * if the string is "", just clear the line
	 */
	if (*fmt == '\0')
	{
		move(0, 0);
		clrtoeol();
		mpos = 0;
		return;
	}
	/*
	 * otherwise add to the message and flush it out
	 */
	doadd(fmt, argp);
	endmsg();
}

//@ varargs variant, now a wrapper for vmsg()
void
msg(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	vmsg(fmt, argp);

	va_end(argp);
}
/* VARARGS1
 * @ now for real
 */
/*
 * addmsg:
 *	Add things to the current message
 */
void
addmsg(const char *fmt, ...)
{
	va_list argp;
	va_start(argp, fmt);

	doadd(fmt, argp);

	va_end(argp);
}

/*
 * endmsg:
 *	Display a new msg (giving him a chance to see the previous one
 *	if it is up there with the -More-)
 */
void
endmsg()
{
	if (save_msg)
		strcpy(huh, msgbuf);
	if (mpos) {
		look(FALSE);
		move(0,mpos);
		more(" More ");
	}
	/*
	 * All messages should start with uppercase, except ones that
	 * start with a pack addressing character
	 */
	if (islower(msgbuf[0]) && msgbuf[1] != ')')
		msgbuf[0] = toupper(msgbuf[0]);
	putmsg(0,msgbuf);
	mpos = newpos;
	newpos = 0;
}


/*
 *  More:  tag the end of a line and wait for a space
 */
void
more(msg)
	char *msg;
{
	int x, y;
	register int i, msz;
	char mbuf[80];
	int morethere = TRUE;
	int covered = FALSE;

	msz = strlen(msg);
	getxy(&x,&y);
	/*
	 * it is reasonable to assume that if the you are no longer
	 * on line 0, you must have wrapped.
	 */
	if (x != 0) {
		x=0;
		y=COLS;
	}
	if ((y+msz)>COLS) {
		move(x,y=COLS-msz);
		covered = TRUE;
	}

	for(i=0;i<msz;i++) {
		mbuf[i] = inch();
		if ((i+y) < (COLS-2))
			move(x,y+i+1);
		mbuf[i+1] = 0;
	}

	move(x,y);
	standout();
	addstr(msg);
	standend();

	while (readchar() != ' ') {
		if (covered && morethere) {
			move(x,y);
			addstr(mbuf);
			morethere = FALSE;
		}
		else if (covered)
		{
			move(x,y);
			standout();
			addstr(msg);
			standend();
			morethere = TRUE;
		}
	}
	move(x,y);
	addstr(mbuf);
}


/*@
* arguments changed from fixed ints to va_list.
* no need of a varargs version as this is only used internally by io.c
* varargs-aware functions
*/
/*
 * doadd:
 *	Perform an add onto the message buffer
 */
void
doadd(const char *fmt, va_list argp)
{

	vsnprintf(&msgbuf[newpos], BUFSIZE - newpos, fmt, argp);
	newpos = strlen(msgbuf);
}

/*
 * putmsg:
 *  put a msg on the line, make sure that it will fit, if it won't
 *  scroll msg sideways until he has read it all
 */
void
putmsg(msgline,msg)
	int msgline;
	char *msg;
{
	register char *curmsg, *lastmsg=0, *tmpmsg;
	int curlen;

	curmsg = msg;
	do {
		scrlmsg(msgline,lastmsg,curmsg);
		newpos = curlen = strlen(curmsg);
		if (curlen > COLS) {
			more(" Cont ");
			lastmsg = curmsg;
			do {
				tmpmsg = strpbrk(curmsg," ");
				/*
				 * If there are no blanks in line
				 */
				if ((tmpmsg==0 || tmpmsg>=&lastmsg[COLS]) && lastmsg==curmsg) {
					curmsg = &lastmsg[COLS];
					break;
				}
				if ((tmpmsg >= (lastmsg+COLS)) || ((signed)strlen(curmsg) < COLS))
					break;
				curmsg = tmpmsg + 1;
			} while (1);
		}
	} while (curlen > COLS);
}

/*
 * scrlmsg:  scroll a message accross the line
 * @ renamed to avoid conflict with <curses.h>.
 * @ Purpose is completely unrelated to curses
 */
void
scrlmsg(msgline,str1,str2)
	int msgline;
	char *str1, *str2;
{
	char *fmt;

	if (COLS > 40)
		fmt = "%.80s";
	else
		fmt = "%.40s";

	if (str1 == 0) {
		move(msgline,0);
		if ((signed)strlen(str2) < COLS)
			clrtoeol();
		printw(fmt,str2);
	}
	else
		while (str1 <= str2) {
			move(msgline,0);
			printw(fmt,str1++);
			if ((signed)strlen(str1) < (COLS-1))
				clrtoeol();
		}
}
/*
 * io_unctrl:
 *	Print a readable version of a certain character
 *	@ renamed to avoid conflict with <curses.h>
 *	@ same purpose but different behavior, so not using the curses version
 */
char *
io_unctrl(ch)
unsigned char ch;
{
	static char chstr[9];		/* Defined in curses library */

	if (isspace(ch))
		strcpy(chstr," ");
	else if (!isprint(ch))
		if (ch < ' ')
			sprintf(chstr, "^%c", ch + '@');
		else
			sprintf(chstr, "\\x%x",ch);
	else {
		chstr[0] = ch;
		chstr[1] = 0;
	}

	return chstr;
}

/*
 * status:
 *	Display the important stats line.  Keep the cursor where it was.
 */
void
status()
{
	int oy, ox;
	static int s_hungry;
	static int s_lvl, s_pur = -1, s_hp, s_ac = 0;
	static str_t s_str;
	static int s_elvl = 0;
	static char *state_name[] =
	{
		"      ", "Hungry", "Weak", "Faint","?"
	};

	SIG2();

	getyx(stdscr, oy, ox);
	if (is_color)
		yellow();

	/*@
	 * Rogue used a rudimentary custom sprintf() that didn't fully support
	 * the (quite sophisticated) numeric formatting strings used on status.
	 * As <stdio.h>'s sprintf() does, formatting was simplified so the output
	 * matches the original.
	 */

	/*
	 * Level:
	 */
	if (s_lvl != level)
	{
		s_lvl = level;
	move(PT(22,23),0);
	printw("Level:%-4d", level);
	}

	/*
	 * Hits:
	 */
	if (s_hp != pstats.s_hpt)
	{
		s_hp = pstats.s_hpt;
		move(PT(22,23),12);
		printw("Hits:%d(%d) ", pstats.s_hpt, max_hp);
		/* just in case they get wraithed with 3 digit max hits */
		if (pstats.s_hpt < 100)
			addch(' ');
	}

	/*
	 * Str:
	 */
	if (pstats.s_str != s_str)
	{
		s_str = pstats.s_str;
		move(PT(22,23),26);
		printw("Str:%d(%d) ", pstats.s_str, max_stats.s_str);
	}

	/*
	 * Gold
	 */
	if(s_pur != purse)
	{
		s_pur = purse;
		move(23, PT(0,40));
		printw("Gold:%-5u",purse);
	}

	/*
	 * Armor:
	 */
	if(s_ac != (cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm))
	{
		s_ac = (cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm);
		if (ISRING(LEFT,R_PROTECT))
			s_ac -= cur_ring[LEFT]->o_ac;
		if (ISRING(RIGHT,R_PROTECT))
			s_ac -= cur_ring[RIGHT]->o_ac;
		move(23,PT(12,52));
		printw("Armor:%-2d",
		AC(cur_armor != NULL ? cur_armor->o_ac : pstats.s_arm));
	}

	/*
	 * Exp:
	 */
	if (s_elvl != pstats.s_lvl)
	{
		s_elvl = pstats.s_lvl;
		move(23, PT(22, 62));
		printw("%-12s", he_man[s_elvl-1]);
	}

	/*
	 * Hungry state
	 */
	if (s_hungry != hungry_state)
	{
		s_hungry = hungry_state;
		move(24, PT(28,58));
		addstr(state_name[0]);
		move(24, PT(28,58));
		if (hungry_state)
		{
			bold();
			addstr(state_name[hungry_state]);
			standend();
		}
	}

	if (is_color)
		standend();

	move(oy, ox);
}

/*
 * wait_for
 *	Sit around until the guy types the right key
 */
void
wait_for(ch)
	char ch;
{
	/*@
	 * stdio and ncurses will map all stream line endings to '\n'
	 * Hooray ANSI! :)
	 *
	register char c;

	if (ch == '\n')
		while ((c = readchar()) != '\n' && c != '\r')
			continue;
	else
	 */
	while (readchar() != ch)
		continue;
}

/*@
 * Wait with a message until user press Enter
 * New function, used to block before leaving the game
 */
void
wait_msg(const char *msg)
{
	standend();
	move(LINES-1,0);
	cursor(TRUE);
	if (*msg)
	{
		printw("[Press Enter to %s]", msg);
	}
	else
	{
		printw("[Press Enter]");
	}
	flush_type();
	wait_for('\n');
	move(LINES-1,0);
}

/*
 * show_win:
 *	Function used to display a window and wait before returning
 *	@ a window? looks like a single message to me!
 */
void
show_win(scr, message)
	int *scr;
	char *message;
{
	mvaddstr(0,0,message);
	move(hero.y, hero.x);
	wait_for(' ');
}


/*
 * str_attr:  format a string with attributes.
 *
 *    formats:
 *        %i - the following character is turned inverse vidio
 *        %I - All characters upto %$ or null are turned inverse vidio
 *        %u - the following character is underlined
 *        %U - All characters upto %$ or null are underlined
 *        %$ - Turn off all attributes
 *
 *     Attributes do not nest, therefore turning on an attribute while
 *     a different one is in effect simply changes the attribute.
 *
 *     "No attribute" is the default and is set on leaving this routine
 *
 *     Eventually this routine will contain colors and character intensity
 *     attributes.  And I'm not sure how I'm going to interface this with
 *     printf certainly '%' isn't a good choice of characters.  jll.
 */
void
str_attr(str)
	char *str;
{
#ifdef LUXURY
	register int is_attr_on = FALSE, was_touched = FALSE;

	while(*str)
	{
		if (was_touched == TRUE)
		{
			standend();
			is_attr_on = FALSE;
			was_touched = FALSE;
		}
	if (*str == '%')
	{
		str++;
		switch(*str)
		{
		case 'u':
					was_touched = TRUE;
				case 'U':
			uline();
					is_attr_on = TRUE;
					str++;
					break;
				case 'i':
					was_touched = TRUE;
				case 'I':
					standout();
					is_attr_on = TRUE;
					str++;
					break;
				case '$':
					if (is_attr_on)
						was_touched = TRUE;
					str++;
					continue;
			 }
		}
		if ((*str == '\n') || (*str == '\r'))
		{
			str++;
			printw("\n");
		}
		else if (*str != 0)
			addch(*str++);
	}
	if (is_attr_on)
		standend();
#else
	while (*str)
	{
		if (*str == '%') {
			str++;
			standout();
		}
		addch(*str++);
		standend();
	}
#endif //LUXURY
}

/*
 * key_state:
 */
void
SIG2()
{
	static int key_init = TRUE;
	static int numl, capsl;
	static int nspot, cspot, tspot;
	register int new_numl, new_capsl, new_fmode;
	static int bighand, littlehand;
	int showtime = FALSE, spare;
	int x, y;
#ifdef DEMO
	static int tot_time = 0;
#endif //DEMO
#ifdef ROGUE_DOS_CLOCK
	static unsigned int ntick = 0;

	//@ only update every 6 ticks, ~3 times per second
	if (tick < ntick)
		return;
	ntick = tick + 6;
#else
	static long cur_time = 0;
	long new_time = md_time();
#endif

	/*@
	 * Do not update between wdump()/wrestor() operations
	 * (when the user is in a non-game screen like inventory or discoveries)
	 * Or if the screen is not yet initialized.
	 */
	if (is_saved || scr_type < 0)
		return;
#ifndef __linux__
	regs->ax = 0x200;
	swint(SW_KEY, regs);
	new_numl = regs->ax;
#else
	new_numl = md_keyboard_leds();
#endif
	new_capsl = new_numl & 0x40;
	new_fmode = new_numl & 0x10;  //@ scroll lock
	new_numl &= 0x20;
#ifdef ROGUE_DOS_CLOCK
	/*
	 * set up the clock the first time here
	 */
	/*@
	 * using DOS INT 21h/AH=2Ch - Get System Time
	 * CH = hour (0-23)
	 * CL = minutes (0-59)
	 */
	if (key_init) {
		regs->ax = 0x2c << 8;
		swint(SW_DOS, regs);
		bighand = (regs->cx >> 8) % 12;  //@ force 12-hour display format
		littlehand = regs->cx & 0xFF;
		showtime = TRUE;
	}
	//@ 1092 ticks = 1 minute @ 18.2 ticks per second rate
	if (tick > 1092) {
		/*
		 * time os call kills jr and others we keep track of it
		 * ourselves
		 */
		littlehand = (littlehand + 1) % 60;
		if (littlehand == 0)
			bighand = (bighand + 1) % 12;
		tick = tick - 1092;
		ntick = tick + 6;
		showtime = TRUE;
	}
#else
	if (new_time - cur_time >= 60)
	{
		TM *local = md_localtime();
		bighand = local->hour % 12;
		littlehand = local->minute;
		cur_time = new_time - local->second;
		showtime = TRUE;
	}
#endif

	/*
	 * this is built for speed so set up once first time this
	 * is executed
	 */
	if (key_init || reinit)
	{
		reinit = key_init = FALSE;
		if (COLS == 40)
		{
			nspot = 10;
			cspot = 19;
			tspot = 35;
		}
		else
		{
			nspot = 20;
			cspot = 39;
			tspot = 75;
		}
		/*
		 * this will force all fields to be updated first time through
		 */
		numl = !new_numl;
		capsl = !new_capsl;
		showtime++;
		faststate = !new_fmode;
	}

	getxy(&x, &y);

	if (faststate != new_fmode)
	{

		faststate = new_fmode;
		count = 0;
		show_count();
		running = FALSE;
		move(LINES-1,0);
		if (faststate)
		{
			bold();
			addstr("Fast Play");
			standend();
		}
		else
		{
			addstr("         ");
		}
	}

	if (numl != new_numl)
	{
		numl = new_numl;
		count = 0;
		show_count();
		running = FALSE;
		move(24,nspot);
		if (numl)
		{
			bold();
			addstr("NUM LOCK");
			standend();
		}
		else
			addstr("        ");
	}
	if (capsl != new_capsl)
	{
		capsl = new_capsl;
		move(24,cspot);
		if (capsl)
		{
			bold();
			addstr("CAP LOCK");
			standend();
		}
		else
			addstr("        ");
	}
	if (showtime)
	{
		showtime = FALSE;
#ifdef DEMO
		/*
		 * Don't let them get by level 10 because they might do something
		 * nasty like disable the clock
		 */
		if (((tot_time++ - max_level) > DEMOTIME) || max_level > 10)
			demo(DEMOTIME);
#endif //DEMO
		/* work around the compiler buggie boos */
		spare = littlehand % 10;
		move(24,tspot);
		bold();
		printw("%2d:%1d%1d",bighand?bighand:12,littlehand/10,spare);
		standend();
	}
	move(x, y);
}

char *
noterse(str)
	char *str;
{
	return( terse || expert ? nullstr : str);
}
