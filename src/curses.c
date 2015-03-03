/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 */

#include	"extern.h"
#include	"curses_common.h"
#ifndef ROGUE_DOS_CURSES
#include	<curses.h>
#endif
#include	"curses_dos.h"


/*
 *  Globals for curses
 *  (extern'ed in curses.h)
 */
int LINES=25, COLS=80;
int is_saved = FALSE;
int old_page_no;  //@ this is public, but page_no is not. Weird. See rip.c
int no_check = FALSE;
int scr_ds=0xB800;
int svwin_ds;
int scr_type = -1;
#ifdef ROGUE_DOS_CURSES
bool iscuron = TRUE;
#endif

//@ unused
int tab_size = 8;

//@ private
int ch_attr = 0x7;
char savewin[2048 * sizeof(chtype)];  //@ originally 4096 bytes
int page_no = 0;


#define MAXATTR 17
byte color_attr[] = {
	7,  /*  0 normal         */
	2,  /*  1 green          */
	3,  /*  2 cyan           */
	4,  /*  3 red            */
	5,  /*  4 magenta        */
	6,  /*  5 brown          */
	8,  /*  6 dark grey      */
	9,  /*  7 light blue     */
	10, /*  8 light green    */
	12, /*  9 light red      */
	13, /* 10 light magenta  */
	14, /* 11 yellow         */
	15, /* 12 uline          */
	1,  /* 13 blue           */
	112,/* 14 reverse        */
	15, /* 15 high intensity */
   112, /* bold				 */
	0   /* no more           */
} ;

byte monoc_attr[] = {
	  7,  /*  0 normal         */
	 7,  /*  1 green          */
	 7,  /*  2 cyan           */
	 7,  /*  3 red            */
	 7,  /*  4 magenta        */
	 7,  /*  5 brown          */
	 7,  /*  6 dark grey      */
	 7,  /*  7 light blue     */
	 7,  /*  8 light green    */
	 7,  /*  9 light red      */
	 7,  /* 10 light magenta  */
	 7,  /* 11 yellow         */
	 17,  /* 12 uline          */
	 7,  /* 13 blue           */
	120,  /* 14 reverse        */
	 7,  /* 15 white/hight    */
	120,  /* 16 bold		   */
	0     /* no more           */
} ;

byte *at_table;

int c_row, c_col;   /*  Save cursor positions so we don't ask dos */
int scr_row[25];

byte dbl_box[BX_SIZE] = {
	0xc9, 0xbb, 0xc8, 0xbc, 0xba, 0xcd, 0xcd
};

byte sng_box[BX_SIZE] = {
	0xda, 0xbf, 0xc0, 0xd9, 0xb3, 0xc4, 0xc4
};

byte fat_box[BX_SIZE] = {
	0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdf, 0xdc
};

byte spc_box[BX_SIZE] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

/*@
 * Beep a an audible beep, if possible
 *
 * Originally in dos.asm
 *
 * Used hardware port 0x61 (Keyboard Controller) for direct PC Speaker access.
 * This behavior is reproduced here, to the best of my knowledge. Comments
 * were copied from original.
 *
 * The debug code prints a BEL (0x07) character in standard output, which is
 * supposed to make terminals play a beep.
 */
void
cur_beep(void)
{
#ifdef ROGUE_DOS_CURSES
	byte speaker = 0x61;       //@ speaker port
	byte saved = in(speaker);  // input control info from keyboard/speaker port
	byte cmd = saved;
	int cycles = 300;          // count of speaker cycles
	int c;

	while (--cycles)
	{
		cmd &= 0x0fc;          // speaker off pulse (mask out bit 0 and 1)
		out(speaker, cmd);     // send command to speaker port
		for(c=50; c; c--) {;}  // kill time for tone half-cycle
		cmd |= 0x10;           // speaker on pulse (bit 1 on)
		out(speaker, cmd);     // send command to speaker port
		for(c=50; c; c--) {;}  // kill time for tone half-cycle
	}
	out(speaker, saved);       // restore speaker/keyboard port value
#ifdef ROGUE_DEBUG
	printf("\a");  //@ lame, I know... but it works
#endif
#else
	beep();
#endif
}


byte
cur_getch(void)
{
#ifdef ROGUE_DOS_CURSES
	//@ not a true replacement, as asm version has no echo and no buffering
	return (byte)getchar();
#else
	return (byte)getch();
#endif
}

/*@
 * Move the cursor to the given row and column
 *
 * Originally in zoom.asm
 *
 * As per original code, also updates C global variables c_col_ and c_row,
 * used as "cache" by curses. See getrc(). Actual cursor movement is only
 * performed if iscuron is set. See cursor().
 *
 * Used BIOS INT 10h/AH=02h to set the cursor on C variable page_no.
 * The BIOS call is now performed via swint()
 *
 * INT 10h/AH=02h - Set Cursor Position
 * BH = page number
 * DH = row
 * DL = col
 */
void
cur_move(row, col)
	int row;
	int col;
{
	c_row = row;
	c_col = col;

#ifdef ROGUE_DOS_CURSES
	if (iscuron)
	{
		regs->ax = HIGH(2);
		regs->bx = HIGH(page_no);
		regs->dx = HILO(row, col);
		swint(SW_SCR, regs);
	}
#else
	move(row, col);
#endif
}


/*@
 * Put the given character on the screen
 *
 * Character is put at current (c_row, c_col) cursor position, and set with
 * current ch_attr attributes.
 *
 * Works as a stripped-down <curses.h> addch(), or as an improved <stdio.h>
 * putchar(): it uses attributes but always operate on current ch_attr instead
 * of extracting attributes from ch, and put at cursor position but does not
 * update its location, nor has any special scroll handling for '\n'. It has
 * no direct counterpart and should not be used when working with <curses.h>.
 *
 * Currently credits() and backspace() do. credits() set ch_attr via set_attr()
 * macros, so for now it may be fine for this to call <curses.h> addch(),
 * but be aware that this makes putchr() do more than what it's supposed to.
 *
 * Originally in zoom.asm
 *
 * By my understanding, asm function works as follows: if cursor is on (via
 * iscuron C var), it invokes BIOS INT 10h/AH=09h to put char with attributes.
 * If not, it waits for video retrace (unless no_check C var was TRUE) and
 * then write directly in Video Memory, using C vars scr_row and scr_ds to
 * calculate position address.
 *
 * This function replicates this behavior using dmaout() and swint().
 *
 * BIOS INT 10h/AH=09h - Write character with attribute at cursor position
 * AL = character
 * BH = page number
 * BL = character attribute
 * CX = number of times to write character
 *
 */
void
putchr(byte ch)
{
#ifdef ROGUE_DOS_CURSES
	if (iscuron)
	{
		// Use BIOS call
		regs->ax = HILO(9, ch);
		regs->bx = HILO(page_no, ch_attr);
		regs->cx = 1;
		swint(SW_SCR, regs);
	}
	else
	{
		if (!no_check){;}  // "wait" for video retrace
		/*
		 * Write to video memory
		 * Each char uses 2 bytes in video memory, hence doubling c_col.
		 * scr_row[] array takes that into account, so we can use c_row
		 * directly. See winit().
		 */
		dmaout(HILO(ch_attr, ch), 1,
			scr_ds, scr_row[c_row] + 2 * c_col);
	}
#ifdef ROGUE_DEBUG
	putchar(ch);
#endif
#else
	addch(ch);
#endif
}


/*@
 * Return character (without any attributes) at current cursor position
 *
 * Wrapper to inch() that strips attributes
 *
 * Asm function returned character with attributes, but all callers stripped
 * out attributes via 0xFF-anding, considering only the character. With inch()
 * the proper stripping would require exporting A_CHARTEXT macro, and perhaps
 * chtype, to the public API, so to simplify both usage and implementation
 * stripping is now performed here, and it returns a character of type byte,
 * the type consistently used by Rogue to indicate a CP850 character.
 *
 * Originally in zoom.asm
 *
 * By my understanding, asm function works very similar to putchr():
 * If iscuron, invokes BIOS INT 10h/AH=02h to set cursor position from cache
 * and 10h/AH=08h to read character, else wait retrace (unless no_check) and
 * read directly from Video Memory. The set cursor BIOS call seems quite
 * redundant, as virtually all calls to curch(), from both the inch() macro and
 * the mvinch() wrapper, are preceded by a move().
 *
 * This function replicates this behavior, except the redundant move.
 *
 * BIOS INT 10h/AH=08h - Read character and attribute at cursor position
 * BH = page number
 * Return:
 * AH = attribute
 * AL = character
 */
byte
curch(void)
{
#ifdef ROGUE_DOS_CURSES
	chtype chrattr = 0;

	if (iscuron)
	{
		regs->ax = HIGH(8);
		regs->bx = HIGH(page_no);
		chrattr = swint(SW_SCR, regs);
	}
	else
	{
		if (!no_check){;}
		dmain(&chrattr, 1, scr_ds, scr_row[c_row] + 2 * c_col);
	}
	return (byte)LOW(chrattr);
#else
	return (byte)(A_CHARTEXT & inch());
#endif
}


#ifdef ROGUE_DOS_CURSES
/*@
 * Fills a buffer with "extended" chars (char + attribute)
 *
 * This is the n-bytes version of setmem(). There is no POSIX or ncurses
 * direct replacement other than a loop writing multiple bytes at a time.
 *
 * For DOS compatibility, chtype (and by proxy wsetmem()) is currently set to
 * operate on 16-bit words. But chtype size in <curses.h> may be set up to
 * a whooping 64-byte unsigned long, so make *really* sure buffer size and
 * count argument are consistent with chtype size!
 *
 * This function could be generic enough to be in mach_dep.c, but it was only
 * by curses.c to write a character + attribute to window buffer.
 *
 * Originally in dos.asm
 */
void
wsetmem(buffer, count, attrchar)
	void *buffer;
	int count;
	chtype attrchar;  // enforced to prevent misuse
{
	while (count--)
		((chtype *)buffer)[count] = (chtype)attrchar;
}
#endif

/*
 * clear screen
 */
void
cur_clear(void)
{
#ifdef ROGUE_DOS_CURSES
	if (scr_ds == svwin_ds)
		wsetmem(savewin, LINES*COLS, 0x0720);
	else
		blot_out(0,0,LINES-1,COLS-1);
#else
	clear();
#endif
}


/*
 *  Turn cursor on and off
 */
bool
cursor(bool ison)
{
#ifdef ROGUE_DOS_CURSES
	register bool oldstate;

	if (iscuron == ison)
		return ison;
	oldstate = iscuron;
	iscuron = ison;

	regs->ax = 0x100;
	if (ison)
	{
		regs->cx = (is_color ? 0x607 : 0xb0c);
		swint(SW_SCR, regs);
		cur_move(c_row, c_col);
	}
	else
	{
		regs->cx = 0xf00;
		swint(SW_SCR, regs);
	}
	return(oldstate);
#else
	/*@
	 * curs_set return values:
	 * ERR = terminal does not support the visibility requested
	 *   0 = invisible
	 *   1 = normal
	 *   2 = very visible
	 */
	int oldstate = curs_set(ison);

	if (oldstate == 0 || oldstate == ERR)
		return FALSE;
	else
		return TRUE;
#endif
}


/*
 * get curent cursor position
 */
void
getrc(rp,cp)
	int *rp, *cp;
{
#ifdef ROGUE_DOS_CURSES
	*rp = c_row;
	*cp = c_col;
#else
	getyx(stdscr, *rp, *cp);
#endif
}

void
real_rc(pn, rp,cp)
	int pn, *rp, *cp;
{
	/*
	 * pc bios: read current cursor position
	 */
	regs->ax = 0x300;
	regs->bx = pn << 8;

	swint(SW_SCR, regs);

	*rp = regs->dx >> 8;
	*cp = regs->dx & 0xff;
}

/*
 *	clrtoeol
 */
void
cur_clrtoeol(void)
{
#ifdef ROGUE_DOS_CURSES
	int r,c;

	if (scr_ds == svwin_ds)
		return;
	getrc(&r,&c);
	blot_out(r,c,r,COLS-1);
#else
	clrtoeol();
#endif
}

void
cur_mvaddstr(r,c,s)
	int r,c;
	char *s;
{
	cur_move(r, c);
	cur_addstr(s);
}

void
cur_mvaddch(int r, int c, byte chr)
{
	cur_move(r, c);
	cur_addch(chr);
}

byte
cur_mvinch(r, c)
	int r, c;
{
	cur_move(r, c);
	return curch();
}

/*
 * put the character on the screen and update the
 * character position
 */
void
cur_addch(byte chr)
{
#ifdef ROGUE_DOS_CURSES
	int r, c;
#endif
	byte old_attr;

	old_attr = ch_attr;

	if (at_table == color_attr)
	{
		/* if it is inside a room */
		if (ch_attr == 7)
		switch(chr)
		{
			case DOOR:
			case VWALL:
			case HWALL:
			case ULWALL:
			case URWALL:
			case LLWALL:
			case LRWALL:
				ch_attr = 6;  /* brown */
				break;
			case FLOOR:
				ch_attr = 10;  /* light green */
				break;
			case STAIRS:
				ch_attr = 160; /* black on green*/
				break;
			case TRAP:
				ch_attr = 5;  /* magenta */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = 14;  /* yellow */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = 9;
				break;
			case FOOD:
				ch_attr = 4;
				break;
		}
		/* if inside a passage or a maze */
		else if (ch_attr == 112)
		switch(chr)
		{
			case FOOD:
				ch_attr = 116;   /* red */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = 126;  /* yellow on white */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = 113;    /* blue on white */
				break;
		}
		else if (ch_attr == 15 && chr == STAIRS)
			ch_attr = 160;
	}

#ifdef ROGUE_DOS_CURSES
	getrc(&r,&c);
	if (chr == '\n') {
		if (r == LINES-1) {
			scroll_up(0, LINES-1, 1);
			cur_move(LINES-1, 0);
		} else
			cur_move(r+1, 0);
			ch_attr = old_attr;
		return;
	}
	putchr(chr);
	cur_move(r,c+1);
#else
	addch(attr_get_from_dos(ch_attr) | chr);
#endif
	ch_attr = old_attr;
	/*
	 * if you have gone of the screen scroll the whole window
	 * @ ... or don't, as no one checked the former c_row return value
	 */
	return;
}

void
cur_addstr(s)
	char *s;
{
#ifdef ROGUE_DEBUG
	print_int_calls = FALSE;
#endif
	while(*s)
		cur_addch(*s++);
#ifdef ROGUE_DEBUG
#ifdef ROGUE_DOS_CURSES
	printf("\n");
#endif
	print_int_calls = TRUE;
#endif
}

#ifndef ROGUE_DOS_CURSES
/*@
 * Convert a DOS/CGA character attribute to its curses equivalent
 * Dummy for now, always return A_NORMAL
 */
attr_t
attr_get_from_dos(byte attr)
{
	return A_NORMAL;
}
#endif

void
set_attr(bute)
	int bute;
{
	if (bute < MAXATTR)
		ch_attr = at_table[bute];
	else
		ch_attr = bute;
#ifndef ROGUE_DOS_CURSES
	attrset(attr_get_from_dos(ch_attr));
#endif
}

#ifdef ROGUE_DOS_CURSES
//@ unused, and proper varargs implementation would require cur_vprintw()
void
error(mline,msg,a1,a2,a3,a4,a5)
	int mline;
	char *msg;
	int a1,a2,a3,a4,a5;
{
	int row, col;

	getrc(&row,&col);
	cur_move(mline,0);
	cur_clrtoeol();
	cur_printw(msg,a1,a2,a3,a4,a5);
	cur_move(row,col);
}

//@ unused, and already stubbed in original
/*
 * Called when rogue runs to move our cursor to be where DOS thinks
 * the cursor is
 */
void
set_cursor(void)
{
/*
	regs->ax = 15 << 8;
	swint(SW_SCR, regs);
	real_rc(regs->bx >> 8, &c_row, &c_col);
*/
}
#endif

/*
 *  winit(win_name):
 *		initialize window -- open disk window
 *						  -- determine type of moniter
 *						  -- determine screen memory location for dma
 */
void
winit()
{
#ifdef ROGUE_DOS_CURSES
	register int i, cnt;
#endif

	/*
	 * Get monitor type
	 */
#ifdef ROGUE_DOS_SCREEN
	regs->ax = 15 << 8;
	swint(SW_SCR, regs);
	old_page_no = regs->bx >> 8;
	scr_type = regs->ax = 0xff & regs->ax;
#else
	old_page_no = 0;
	scr_type = ROGUE_SCR_TYPE;
#endif
	/*
	 * initialization is any good because restarting game
	 * has old values!!!
	 * So reassign defaults
	 */
	LINES   =  25;
	COLS    =  80;
	scr_ds  =  0xB800;
	at_table = monoc_attr;

	/*@
	 * BIOS INT 10h/AX=0Fh table for AL values used in the switch:
	 * 00h - Text 40x25 mono  - Only CGA, PCjr, Tandy
	 * 01h - Text 40x25 color - Only CGA, PCjr, Tandy
	 * 02h - Text 80x25 mono  - Only CGA, PCjr, Tandy
	 * 03h - Text 80x25 color - All adapters
	 * 07h - Text 80x25 mono  - MDA, Hercules, EGA, VGA
	 * Notes:
	 * - mono is actually 16 shades of gray
	 * - colors are always 16 except for EGA in mode 3, which could be 64
	 * - EGA in mode 3 could also be 80x43
	 * - VGA in mode 3 could also be 80x43 or 80x50
	 */
	switch (scr_type) {
		/*
		 *  It is a TV
		 */
		case 1:
			at_table = color_attr;
			/* no break */
		case 0:
			COLS = 40;
			break;

		/*
		 * Its a high resolution monitor
		 */
		case 3:
			at_table = color_attr;
			/* no break */
		case 2:
			break;
		case 7:
			scr_ds = 0xB000;
			no_check = TRUE;
			break;
		/*
		 * Just to save text space lets eliminate these
		 *
		case 4:
		case 5:
		case 6:
			move(24,0);
			fatal("Program can't be run in graphics mode");
		 */
		default:
			cur_move(24,0);
			fatal("Unknown screen type (%d)",regs->ax);
			break;
	}

#ifdef ROGUE_DOS_CURSES
	/*
	 * Read current cursor position
	 */
	real_rc(old_page_no, &c_row, &c_col);
	/*@ savewin is now a fixed size array.
	if ((savewin = sbrk(4096)) == (void *)-1) {
		svwin_ds = -1;
		savewin = (char *) _flags;
		if (scr_type == 7)
			fatal(no_mem);
	} else {
		savewin = (char *) (((intptr) savewin + 0xf) & 0xfff0);
		svwin_ds = (((intptr) savewin >> 4) & 0xfff) + _dsval;
	}
	*/
	svwin_ds = (((intptr) savewin >> 4) & 0xfff) + _dsval;

	for (i = 0, cnt = 0; i < 25; cnt += 2*COLS, i++)
		scr_row[i] = cnt;
	//@ newmem(2);  // no longer need memory alignment
	switch_page(3);
	if (old_page_no != page_no)
		cur_clear();
	cur_move(c_row, c_col);
	if (isjr())
		no_check = TRUE;
#else
	initscr();
	cbreak();
	noecho();
	//nodelay(stdscr, TRUE);
	//nonl();
	intrflush(stdscr, FALSE);
	keypad(stdscr, TRUE);
#endif
}

void
forcebw()
{
	at_table = monoc_attr;
}

/*
 *  wdump(windex)
 *		dump the screen off to disk, the window is save so that
 *		it can be retieved using windex
 */
void
wdump()
{
	sav_win();
	dmain(savewin,LINES*COLS,scr_ds,0);
	is_saved = TRUE;
}

char *
sav_win()
{
	/*@ savewin is now a fixed size array
	if (savewin == (char *)_flags)
		dmaout(savewin,LINES*COLS,0xb800,8192);
	*/
	return(savewin);
}

void
res_win()
{
	/*@ savewin is now a fixed size array
	if (savewin == (char *)_flags)
		dmain(savewin,LINES*COLS,0xb800,8192);
	*/
}

/*
 *	wrestor(windex):
 *		restor the window saved on disk
 */
void
wrestor()
{
	dmaout(savewin,LINES*COLS,scr_ds,0);
	res_win();
	is_saved = FALSE;
}

/*
 * wclose()
 *   close the window file
 */
void
wclose()
{
#ifdef ROGUE_DOS_CURSES
	/*
	 * Restor cursor (really you want to restor video state, but be carefull)
	 */
	if (scr_type >= 0)
		cursor(TRUE);
	if (page_no != old_page_no)
		switch_page(old_page_no);
#else
	endwin();
#endif
}

/*
 *  Some general drawing routines
 */

void
cur_box(int ul_r, int ul_c, int lr_r, int lr_c)
{
	vbox(dbl_box, ul_r, ul_c, lr_r, lr_c);
}

/*
 *  box:  draw a box using given the
 *        upper left coordinate and the lower right
 */
void
vbox(box, ul_r,ul_c,lr_r,lr_c)
	byte box[BX_SIZE];
	int ul_r,ul_c,lr_r,lr_c;
{
	register int i, wason;
	int r,c;

	wason = cursor(FALSE);
	getrc(&r,&c);

	/*
	 * draw horizontal boundry
	 */
	cur_move(ul_r, ul_c+1);
	repchr(box[BX_HT], i = (lr_c - ul_c - 1));
	cur_move(lr_r, ul_c+1);
	repchr(box[BX_HB], i);
	/*
	 * draw vertical boundry
	 */
	for (i=ul_r+1;i<lr_r;i++) {
		cur_mvaddch(i,ul_c,box[BX_VW]);
		cur_mvaddch(i,lr_c,box[BX_VW]);
	}
	/*
	 * draw corners
	 */
	cur_mvaddch(ul_r,ul_c,box[BX_UL]);
	cur_mvaddch(ul_r,lr_c,box[BX_UR]);
	cur_mvaddch(lr_r,ul_c,box[BX_LL]);
	cur_mvaddch(lr_r,lr_c,box[BX_LR]);

	cur_move(r,c);
	cursor(wason);
}

/*
 * center a string according to how many columns there really are
 */
void
center(row,string)
	int row;
	char *string;
{
	cur_mvaddstr(row,(COLS-strlen(string))/2,string);
}


/*@
 * Originally had this signature:
 * printw(msg,a1,a2,a3,a4,a5,a6,a7,a8)
 *   char *msg;
 *   int a1, a2, a3, a4, a5, a6, a7, a8;
 *
 * Guess there was no varargs in 1985...
 *
 * Also changed sprintf() to the more secure vsnprintf()
 * No buffer overflows in 85 either?
 *
 * Ieeeee indeed :)
 */
/*
 * printw(Ieeeee)
 */
void
cur_printw(const char *msg, ...)
{
	char pwbuf[132];
	va_list argp;

	va_start(argp, msg);
	vsnprintf(pwbuf, sizeof(pwbuf), msg, argp);
	va_end(argp);
	cur_addstr(pwbuf);
}

#ifdef ROGUE_DOS_CURSES
void
scroll_up(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	regs->ax = 0x600 + nlines;
	regs->bx = 0x700;
	regs->cx = start_row << 8;
	regs->dx = (end_row << 8) + COLS - 1;
	swint(SW_SCR,regs);
	cur_move(end_row,c_col);
}

//@ unused
void
scroll_dn(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	regs->ax = 0x700 + nlines;
	regs->bx = 0x700;
	regs->cx = start_row << 8;
	regs->dx = (end_row << 8) + COLS - 1;
	swint(SW_SCR,regs);
	cur_move(start_row,c_col);
}

//@ unused
void
scroll(void)
{
	scroll_up(0,24,1);
}

/*
 * blot_out region
 *    (upper left row, upper left column)
 *	  (lower right row, lower right column)
 */
void
blot_out(ul_row,ul_col,lr_row,lr_col)
	int ul_row,ul_col,lr_row,lr_col;
{
	regs->ax = 0x600;
	regs->bx = 0x700;
	regs->cx = (ul_row<<8) + ul_col;
	regs->dx = (lr_row<<8) + lr_col;
	swint(SW_SCR,regs);
	cur_move(ul_row,ul_col);
}

/*
 * try to fixup screen after we get a control break
 * @ unused
 */
void
fixup(void)
{
	blot_out(c_row,c_col,c_row,c_col+1);
}
#endif

void
repchr(chr,cnt)
	int chr, cnt;
{
	while(cnt-- > 0) {
		putchr(chr);
		c_col++;
	}
}

/*
 * Clear the screen in an interesting fashion
 */
void
implode()
{
	int j, delay, r, c, cinc = COLS/10/2, er, ec;

	er = (COLS == 80 ? LINES-3 : LINES-4);
	/*
	 * If the curtain is down, just clear the memory
	 */
	if (scr_ds == svwin_ds) {
#ifdef ROGUE_DOS_CURSES
		wsetmem(savewin, (er + 1) * COLS, 0x0720);
#else
		//@ actually should only clear up to the er-th line
		cur_clear();
#endif
		return;
	}
	delay = scr_type == 7 ? 500 : 10;
	for (r = 0,c = 0,ec = COLS-1; r < 10; r++,c += cinc,er--,ec -= cinc) {
		vbox(sng_box, r, c, er, ec);
		for (j = delay; j--; )
			;
		for (j = r+1; j <= er-1; j++) {
			cur_move(j, c+1); repchr(' ', cinc-1);
			cur_move(j, ec-cinc+1); repchr(' ', cinc-1);
		}
		vbox(spc_box, r, c, er, ec);
	}
}

/*
 * drop_curtain:
 *	Close a door on the screen and redirect output to the temporary buffer
 */
static int old_ds;
void
drop_curtain()
{
	register int r, j, delay;

	if (svwin_ds == -1)
		return;
	old_ds = scr_ds;
	dmain(savewin, LINES * COLS, scr_ds, 0);
	cursor(FALSE);
	delay = (scr_type == 7 ? 3000 : 2000);
	green();
	vbox(sng_box, 0, 0, LINES-1, COLS-1);
	yellow();
	for (r = 1; r < LINES-1; r++) {
		cur_move(r, 1);
		repchr(0xb1, COLS-2);
		for (j = delay; j--; )
			;
	}
	scr_ds = svwin_ds;
	cur_move(0,0);
	cur_standend();
}

void
raise_curtain()
{
	register int i, j, o, delay;

	if (svwin_ds == -1)
		return;
	scr_ds = old_ds;
	delay = (scr_type == 7 ? 3000 : 2000);
	for (i = 0, o = (LINES-1)*COLS*2; i < LINES; i++, o -= COLS*2) {
		dmaout(savewin + o, COLS, scr_ds, o);
		for (j = delay; j--; )
			;
	}
}

void
switch_page(pn)
	int pn;
{
	register int pgsize;

	if (scr_type == 7) {
		page_no = 0;
		return;
	}
	if (COLS == 40)
		pgsize = 2048;
	else
		pgsize = 4096;
	regs->ax = 0x0500 | pn;
	swint(SW_SCR, regs);
	scr_ds = 0xb800 + ((pgsize * pn) >> 4);
	page_no = pn;
}

byte
get_mode()
/*@
 * Get current video mode using software interrupt 10h, AH=0Fh
 *
 * Response is:
 * AL = Video Mode, AH = number of character columns, BH = active page
 * https://en.wikipedia.org/wiki/INT_10H
 * http://www.ctyme.com/intr/rb-0108.htm
 *
 * Return only AL, the low A byte representing Video Mode, as a 16-bit int
 * For EGA text, AL is 03h for color or 07h for monochrome
 */
{
	struct sw_regs regs;

	regs.ax = 0xF00;  //@ AH = 0Fh
	swint(SW_SCR,&regs);
	return 0xff & regs.ax;
}

/*@
 * Set current video mode using software interrupt 10h, AH=00h
 *
 * Response is:
 * AL = Video Mode or CRT controller
 * http://www.ctyme.com/intr/rb-0069.htm
 *
 * Return AL
 */
byte
video_mode(type)
	int type;
{
	struct sw_regs regs;

	regs.ax = type;
	swint(SW_SCR,&regs);
	return regs.ax;
}
