/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 */

#include	"extern.h"
#include	"curses_common.h"
#ifndef ROGUE_DOS_CURSES
#include	<curses.h>
#endif
#include	"curses_dos.h"
#include	"keypad.h"


/*
 *  Globals for curses
 *  (extern'ed in curses.h)
 */
int LINES=25, COLS=80;
int is_saved = FALSE;  //@ in practice, TRUE disables status updates in SIG2()
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
int ch_attr = A_DOS_NORMAL;
int page_no = 0;
bool init_curses = FALSE;  //@ if curses is active or not
#ifdef ROGUE_DOS_CURSES
int c_row, c_col;   /*  Save cursor positions so we don't ask dos */
int scr_row[25];
char savewin[2048 * sizeof(chtype)];  //@ originally 4096 bytes
#else
chtype	savewin[MAXLINES][MAXCOLS + 1];  // temp buffer to hold screen contents
int 	colors;  // number of basic colors
#endif


#define MAXATTR 17
byte color_attr[] = {
	A_DOS_NORMAL,  /*  0 normal         */
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
	A_DOS_NORMAL,  /*  0 normal         */
	A_DOS_NORMAL,  /*  1 green          */
	A_DOS_NORMAL,  /*  2 cyan           */
	A_DOS_NORMAL,  /*  3 red            */
	A_DOS_NORMAL,  /*  4 magenta        */
	A_DOS_NORMAL,  /*  5 brown          */
	A_DOS_NORMAL,  /*  6 dark grey      */
	A_DOS_NORMAL,  /*  7 light blue     */
	A_DOS_NORMAL,  /*  8 light green    */
	A_DOS_NORMAL,  /*  9 light red      */
	A_DOS_NORMAL,  /* 10 light magenta  */
	A_DOS_NORMAL,  /* 11 yellow         */
	17,            /* 12 uline          */
	A_DOS_NORMAL,  /* 13 blue           */
	120,           /* 14 reverse        */
	A_DOS_NORMAL,  /* 15 white/hight    */
	120,           /* 16 bold		   */
	0              /* no more           */
} ;

byte *at_table;

byte dbl_box[BX_SIZE] = {
	ULWALL, URWALL, LLWALL, LRWALL, VWALL, HWALL, HWALL
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

/*
 * Table for IBM extended key translation
 * moved from march_dep.c
 */
static struct xlate {
	int keycode;
	byte keyis;
} xtab[] = {
#ifdef ROGUE_DOS_CURSES
	{C_HOME,	'y'},
	{C_UP,		'k'},
	{C_PGUP,	'u'},
	{C_LEFT,	'h'},
	{C_RIGHT,	'l'},
	{C_END,		'b'},
	{C_DOWN,	'j'},
	{C_PGDN,	'n'},
	{C_INS,		'>'},
	{C_DEL,		's'},
	{C_F1,		'?'},
	{C_F2,		'/'},
	{C_F3,		'a'},
	{C_F4,		CTRL('R')},
	{C_F5,		'c'},
	{C_F6,		'D'},
	{C_F7,		'i'},
	{C_F8,		'^'},
	{C_F9,		CTRL('F')},
	{C_F10,		'!'},
	{ALT_F9,	'F'}
#else
	{KEY_HOME,	'y'},
	{KEY_FIND,	'y'},  //@ Keypad Home (7) in some terminals
	{KEY_A1,	'y'},  //@ Keypad upper left (7)
	{KEY_UP,	'k'},
	{KEY_PPAGE,	'u'},  //@ Page Up
	{KEY_A3,	'u'},  //@ Keypad upper right (9)
	{KEY_LEFT,	'h'},
	{KEY_RIGHT,	'l'},
	{KEY_END,	'b'},
	{KEY_SELECT,'b'},  //@ Keypad End (1) in some terminals
	{KEY_C1,	'b'},  //@ Keypad lower left (1)
	{KEY_DOWN,	'j'},
	{KEY_NPAGE,	'n'},  //@ Page Down
	{KEY_C3,	'n'},  //@ Keypad lower right (3)
	{KEY_IC,	'>'},  //@ Insert
	{KEY_DC,	's'},  //@ Delete
	{KEY_F(1),	'?'},
	{KEY_F(2),	'/'},
	{KEY_F(3),	'a'},
	{KEY_F(4),	CTRL('R')},
	{KEY_F(5),	'c'},
	{KEY_F(6),	'D'},
	{KEY_F(7),	'i'},
	{KEY_F(8),	'^'},
	{KEY_F(9),	CTRL('F')},
	{KEY_F(10),	'!'},
	{KEY_F(57),	'F'}
#endif
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


int
cur_getch(void)
{
#ifdef ROGUE_DOS_CURSES
	//@ not a true replacement, as asm version has no echo and no buffering
	return getchar();
#else
	return wgetch(stdscr);
#endif
}


/*@
 * Map a (possibly multi-byte or control) character to an 8-bit character using
 * the game translation table.
 *
 * Moved from mach_dep.c as part of readchar()
 */
byte
xlate_ch(int ch)
{
	struct xlate *x;
	/*
	 * Now read a character and translate it if it appears in the
	 * translation table
	 */
	for (x = xtab; x < xtab + (sizeof xtab) / sizeof *xtab; x++)
	{
		if (ch == x->keycode) {
			ch = x->keyis;
			break;
		}
	}
	return (byte)ch;
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
#ifdef ROGUE_DOS_CURSES
	c_row = row;
	c_col = col;

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


#ifdef ROGUE_DOS_CURSES
/*@
 * Put the given character on the screen
 *
 * Character is put at current (c_row, c_col) cursor position, and set with
 * current ch_attr attributes.
 *
 * Works as a stripped-down <curses.h> addch(), or as an improved <stdio.h>
 * putchar(): it uses attributes but always operate on current ch_attr instead
 * of extracting attributes from ch, and put at cursor position but does not
 * update its location, nor has any special CR/LF/scroll up handling for '\n'.
 *
 * A curses replacement could be:
 *    delch();
 *    insch(ch);
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
}
#endif


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
	return (byte)(A_CHARTEXT & winch(stdscr));
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
	wclear(stdscr);
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

#ifdef ROGUE_DOS_CURSES
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
#endif

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
	wclrtoeol(stdscr);
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
		if (ch_attr == A_DOS_NORMAL)
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
	/*@
	 * For '\n', perform a cursor CR+LF if not on last line (off-screen),
	 * or just CR and scroll the whole window content up.
	 * Otherwise just put the char and advance the cursor.
	 */
	getrc(&r,&c);
	if (chr == '\n') {
		if (r == LINES-1)
		{
			scroll_up(0, LINES-1, 1);
			cur_move(LINES-1, 0);
		}
		else
		{
			cur_move(r+1, 0);
		}
	}
	else
	{
		putchr(chr);
		cur_move(r,c+1);
	}
#else
	waddch(stdscr, attr_from_dos(ch_attr) | chr);
#endif
	ch_attr = old_attr;
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
byte swap_bits(
	byte data,
	unsigned i,      // positions of bit sequences to swap
	unsigned j,
	unsigned length  // number of consecutive bits in each sequence
)
{
	byte x = ((data >> i) ^ (data >> j)) & ((1U << length) - 1);
	return data ^ ((x << i) | (x << j));
}

attr_t
color_from_dos(byte dos_attr, bool fg)
{
	byte color = (dos_attr >> (fg ? A_DOS_FG_COLOR : A_DOS_BG_COLOR)) & \
			A_DOS_COLOR_MASK;

	// swap red and blue
	return swap_bits(color, 0, 2, 1);
}

/*
 * Convert a DOS/CGA character attribute to its curses equivalent
 */
attr_t
attr_from_dos(byte dos_attr)
{
	attr_t attr = A_NORMAL;
	int fg, bg;

	// shortcut to avoid setting (and calculating) a spurious color pair
	if (dos_attr == A_DOS_NORMAL)
		return attr;

	if (dos_attr & A_DOS_BLINK)
		attr |= A_BLINK;

	if (dos_attr & A_DOS_BRIGHT)
		attr |= A_BOLD;

	fg = color_from_dos(dos_attr, TRUE);
	bg = color_from_dos(dos_attr, FALSE);

	attr |= COLOR_PAIR_N(fg, bg);

	return attr;
}


void
init_curses_colors(void)
{
	int fg, dos_fg, dfg;
	int bg, dos_bg, dbg;

	start_color();

	/*
	 * DOS only uses 8 basic colors, bumped to 16 via bright attribute.
	 * For now, we do the same, but this could be changed to 16 (if the A_BOLD
	 * method does not work), or even 256. Always respecting color capability
	 * reported by curses via COLORS.
	 */
	colors = min(8, COLORS);

	dos_fg = color_from_dos(A_DOS_NORMAL, TRUE);
	dos_bg = color_from_dos(A_DOS_NORMAL, FALSE);

#ifdef NCURSES_VERSION
	use_default_colors();
	dfg = dbg = -1;
#else
	dfg = COLOR_WHITE;
	dbg = COLOR_BLACK;
#endif

	/*@
	 * Notes on color assumptions, mappings and pairs:
	 *
	 * - Foreground and background colors used by DOS are indexed from 0 to 7.
	 *   Index is actually an RGB bitmap, blue being the least significant bit,
	 *   so in all colors the Red and Blue components are swapped compared to
	 *   curses named constants (which are the ANSI color indexes).
	 *
	 * - Color pairs are set in a way the macro COLOR_PAIR_N(fg, bg) can
	 *   retrieve a curses color pair attribute by foreground and background
	 *   index instead of pair index.
	 *
	 * - Color pair 0 is neither initialized nor used, per portability
	 *   recommendation in curses documentation.
	 *
	 * - The default foreground and background colors used by DOS, as defined
	 *   by A_DOS_NORMAL, were mapped to (COLOR_WHITE, COLOR_BLACK). If the
	 *   curses implementation is ncurses, they are mapped to (-1, -1) instead,
	 *   the user default foreground and background terminal colors.
	 *
	 * - Only 8 colors are being used for now. It is expected, but not hard-
	 *   coded, that any color terminal has at least that. If 1983 CGA can, I'm
	 *   sure a 2015 terminal can as well ;)
	 */
	for (bg = 0; bg < colors; bg++)
	{
		for (fg = colors - (bg ? 2 : 1); fg >= 0; fg--)
		{
			init_pair(PAIR_INDEX(fg, bg),
					(fg == dos_fg) ? dfg : fg,
					(bg == dos_bg) ? dbg : bg);
		}
	}
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
	attrset(attr_from_dos(ch_attr));
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
	int lines = min(LINES, MAXLINES);
	int cols  = min(ROGUE_COLUMNS, MAXCOLS);

	if (init_curses)
		return;

	at_table = color_attr;

	//@ these still affect the game. the goal is to remove them all
	old_page_no = 0;
	scr_type = ROGUE_SCR_TYPE;
	scr_ds   = 0xB800;
	svwin_ds = 0;

	setenv("ESCDELAY", "25", FALSE);
	initscr();
	init_curses = TRUE;
	if ((LINES < lines) || (COLS < cols))
	{
		fatal("%u-column mode requires a %u x %u screen\n"
				"Your terminal size is %u x %u\n",
				cols, cols, lines, COLS, LINES);
	}
#ifdef ROGUE_DEBUG
	printw("Real terminal size: LINES: %u\tCOLS: %u", LINES, COLS);
	getch();
#endif
	if ((LINES != lines) || (COLS != cols))
	{
		if (resizeterm(lines, cols) == OK)
		{
			getch();  //@ eat up the generated KEY_RESIZE
		}
		else
		{
			fatal("Could not resize resize terminal to %u x %u\n",
					cols, lines);
		}
	}
	cbreak();  //@ do not buffer input until ENTER
	noecho();  //@ do not echo typed characters
	immedok(stdscr, TRUE);  //@ immediately refresh() screen on *add{ch,str}()
	nodelay(stdscr, FALSE); //@ use a blocking getch() (already the default)
	keypad(stdscr, TRUE);   //@ enable directional arrows, keypad, home, etc

	if (has_colors())
		init_curses_colors();
#endif
}


void
forcebw()
{
	at_table = monoc_attr;
}

#ifdef ROGUE_DOS_CURSES
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
#else
/*@
 * Dump the screen to the savewin buffer
 */
void
wdump(void)
{
	int line;
	int c_row, c_col;

	getyx(stdscr, c_row, c_col);
	for (line = 0; line < LINES; line++)
	{
		mvinchnstr(line, 0, savewin[line], COLS);
	}
	wmove(stdscr, c_row, c_col);

	is_saved = TRUE;
}

/*@
 * Restore the screen from the savewin buffer
 */
void
wrestor(void)
{
	int line;
	int c_row, c_col;

	getyx(stdscr, c_row, c_col);
	for (line = 0; line < LINES; line++)
	{
		mvaddchnstr(line, 0, savewin[line], COLS);
	}
	wmove(stdscr, c_row, c_col);

	is_saved = FALSE;
}
#endif

/*
 *   close the window file
 *   @renamed from wclose()
 */
void
cur_endwin()
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
	if (init_curses)
	{
		endwin();
		init_curses = FALSE;
#ifdef ROGUE_DEBUG
		printf("Curses window closed\n");
#endif
	}
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
repchr(byte chr, int cnt)
{
	while(cnt-- > 0) {
#ifdef ROGUE_DOS_CURSES
		putchr(chr);
		c_col++;
#else
		waddch(stdscr, chr);
#endif
	}
}

/*
 * Clear the screen in an interesting fashion
 */
void
implode()
{
#ifdef ROGUE_ASCII
	cur_clear();
#else
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
#endif
}

/*
 * drop_curtain:
 *	Close a door on the screen and redirect output to the temporary buffer
 *	@ requires a later call to raise_curtain()
 */
static int old_ds;
void
drop_curtain(void)
{
	register int r, j, delay;

	if (svwin_ds == -1)
		return;
	old_ds = scr_ds;
	dmain(savewin, LINES * COLS, scr_ds, 0);
	cursor(FALSE);
	/*@
	 * The different delay for mono and color adapters implies the BIOS call
	 * used by repchr()->putchr() is significantly faster under mono video mode,
	 */
	delay = (scr_type == 7 ? 3000 : 2000);
	green();
#ifdef ROGUE_ASCII
	vbox(dbl_box, 0, 0, LINES-1, COLS-1);
#else
	vbox(sng_box, 0, 0, LINES-1, COLS-1);
#endif
	yellow();
	for (r = 1; r < LINES-1; r++) {
		cur_move(r, 1);
		repchr(PASSAGE, COLS-2);
		for (j = delay; j--; )
			;
	}
	scr_ds = svwin_ds;
	cur_move(0,0);
	cur_standend();
}

void
raise_curtain(void)
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


/*
 * This routine reads information from the keyboard
 * It should do all the strange processing that is
 * needed to retrieve sensible data from the user
 *
 * @ "Strange processing" indeed:
 * - ESCAPE abort the input, set the first character of str to ESCAPE but leave
 *   all other typed characters there. It does *NOT* null-terminate str!!!
 *   Like in printw(), it couldn't care less about buffer exploits.
 *   Return ESCAPE.
 * - '\n' finishes input and null-terminate str. '\n' is not included in str.
 *   Return '\n'
 * - A non-ascii char (>127) also finishes input, but it *does* get included
 *   in str, probably unintentionally. srt is properly null-terminated.
 *   Return the non-ascii char.
 * - All other chars are accepted as normal input, including symbols (< 32).
 *
 * Original behavior is changed:
 * - Aborted input are null-terminated (ESCAPE + '\0')
 * - Only printable ASCII chars accepted (32 <= ch <= 126). This is universally
 *   compatible, until proper CP437 and UTF-8 support is implemented.
 *
 * In a sane, safe API this function would return a bool, FALSE if aborted
 * by ESCAPE and TRUE otherwise, and it would always null-terminate str
 * regardless of its initial contents. In case of abortion, str could either
 * keep typed string or set first char to '\0', effectively blanking str.
 *
 */
int
getinfo(str,size)
	char *str;
	int size;
{
	register char *retstr;
	int ch;
	int readcnt = 0;
	int wason, ret = 1;
#ifdef ROGUE_DOS_CURSES
	/*@
	 * Save the line state before typing begins, and restore it after user ends
	 * typing, effectively deleting from the screen only the user input. Seems
	 * a nice polishing touch, but it has some drawbacks: Cursor position was
	 * not restored; it is hard coded to line 0, so not useful for fakedos()
	 * and credits(); If fixed, it would disrupt fakedos() where user input
	 * should persist on screen. fakedos() starts on line 1, perhaps also
	 * because of this; And all other callers clear the whole line right after
	 * the call, defeating the whole purpose of this save/restore. All things
	 * considered, this should not be performed in the non-DOS curses version.
	 */
	char buf[160];

	dmain(buf, 80, scr_ds, 0);
#endif
	retstr = str;
	*str = 0;
	wason = cursor(TRUE);
	while(ret == 1)
	{
		switch(ch = getch()) {
			case ESCAPE:
				while(str != retstr) {
					backspace();
					readcnt--;
					str--;
				}
				//@ null-termination was not in original
				ret = *str++ = ESCAPE;
				*str = 0;
				cursor(wason);
				break;
#ifndef ROGUE_DOS_CURSES
			case KEY_BACKSPACE:
#endif
			case '\b':
				if (str != retstr) {
					backspace();
					readcnt--;
					str--;
				}
				break;
			default:
				if ( readcnt >= size) {
					beep();
					break;
				}
				if (!isprint(ch))
				{
					break;
				}
				readcnt++;
				addch(ch);
				*str++ = ch;
				break;
			case '\n':
				*str = 0;
				cursor(wason);
				ret = ch;  //@ any value different than ESCAPE or 1 would do.
				break;
		}
	}
#ifdef ROGUE_DOS_CURSES
	dmaout(buf, 80, scr_ds, 0);
#endif
	return ret;
}

/*@
 * No need of #ifdef ROGUE_DOS_CURSES here as the non-curses input of getch()
 * used by getinfo() is performed by <stdio.h> getchar(), which is line
 * buffered anyway, so a backspace char will never be seen and this will never
 * be called.
 */
void
backspace(void)
{
	int r, c;
	getyx(stdscr, r, c);
	if (c > 0)
		wmove(stdscr, r, c-1);
	wdelch(stdscr);
	winsch(stdscr, ' ');
}
