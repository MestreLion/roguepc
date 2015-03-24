/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 */

#include	"extern.h"

#ifndef ROGUE_DOS_CURSES
/*@
 * Enables wide-character support ("complex renditions") in curses
 * In theory _XOPEN_SOURCE 500 would suffice, but it does not seem to work with
 * ncursesw even when including <features.h>. Also, most sources on the 'net
 * recommend including <ncursesw/curses.h> directly, but I feel this is a task
 * for the Makefile/build system, not source code.
 */
#ifndef _XOPEN_SOURCE_EXTENDED
#define _XOPEN_SOURCE_EXTENDED
#endif

//@ <curses.h> will set _XOPEN_CURSES if wide chars are available
#undef _XOPEN_CURSES
#include	<curses.h>
#endif  // ROGUE_DOS_CURSES

#include	"curses_common.h"
#include	"curses_dos.h"
#include	"keypad.h"


/*
 *  Globals for curses
 *  (extern'ed in curses.h)
 */
int is_saved = FALSE;  //@ in practice, TRUE disables status updates in SIG2()
int no_check = FALSE;
int scr_type = -1;
#ifdef ROGUE_DOS_CURSES
int LINES=25, COLS=80;
bool iscuron = TRUE;
int old_page_no;  //@ this is public, but page_no is not. Weird. See rip.c
int scr_ds=0xB800;
int svwin_ds = 0;
#else
// current terminal size as reported by curses. Will change on window resize
extern int LINES, COLS;

/*
 * The following are not used by the application, so not really extern'ed in
 * curses.h. But they could be, and some like change_colors should be set via
 * env file / options
 */

// Terminal size we *want*, not necessarily what we will get
int cur_LINES = min(25, MAXLINES);
int cur_COLS  = min(ROGUE_COLUMNS, MAXCOLS);

// if curses is initialized or not. If extern'ed, should be read-only
bool init_curses = FALSE;

/* Charset used. Could be initially set via env file, but should not be changed
 * mid-game unless we create a function to re-draw the screen. The code should
 * already graciously fallback to ASCII if UNICODE is requested here but not
 * available, either because it was not compiled with wide char support or the
 * current curses implementation does not support it. Both cases are tested
 * with _XOPEN_CURSES. ROGUE_CHARSET is a factory default that already accounts
 * for Unicode availability and compile-time options.
 */
int	charset = ROGUE_CHARSET;

/*
 * Number of colors we're working with, regardless if terminal has more colors
 * available. This is set by init_curses_colors() and should be the result of
 * many factors, not only terminal COLORS reported by curses but also `screen`
 * env file setting (for bw), ROGUE_SCR_TYPE, etc.
 *
 * Values can be:
 * -  0 for monochrome
 * -  8 for 8 basic colors (light versions will use BOLD text attribute)
 * - 16 if all DOS colors are directly indexable
 *
 * If extern'ed, should obviously be read-only
 */
int 	colors;

// if user allows us to redefine color palette to match original RGB
bool change_colors = TRUE;

// if user wants to use default terminal foreground / background color
bool use_terminal_fgbg = TRUE;
#endif

//@ unused
int tab_size = 8;

//@ private
static int ch_attr = A_DOS_NORMAL;
#ifdef ROGUE_DOS_CURSES
static int page_no = 0;
static int c_row, c_col;   /*  Save cursor positions so we don't ask dos */
static int scr_row[25];
#else
#ifndef _XOPEN_CURSES
static chtype	curtain[MAXLINES][MAXCOLS + 1];  // temp buffer for curtain animations
#else
static cchar_t	curtain[MAXLINES][MAXCOLS + 1];
static cchar_t	cctemp;
#endif // _XOPEN_CURSES
static int	KEY_MASK;
static wchar_t	ccunicode[2] = L" ";  // temp buffer
static CCODE	ccode = {'\0', ccunicode, '\0'};  // temp charcode
static bool	colors_changed = FALSE;  // if colors palette was redefined
#endif  // ROGUE_DOS_CURSES

/*@
 * "it's complicated" - extern'ed but should probably be static and have an API
 * savewin is used in game save/restore, and I'm still not sure if exposing it
 * is the best long term solution.
 */
#if   defined (ROGUE_DOS_CURSES)
char savewin[2048 * sizeof(chtype)];  //@ originally 4096 bytes
#elif defined (_XOPEN_CURSES)
cchar_t	savewin[MAXLINES][MAXCOLS + 1];  // temp buffer to hold screen contents
#else
chtype	savewin[MAXLINES][MAXCOLS + 1];  // temp buffer to hold screen contents
#endif

/*@
 * Original used decimal literals for both tables
 */
#define MAXATTR 17
static byte color_attr[] = {
	A_DOS_NORMAL,                  /*  0 normal         */
	A_DOS_GREEN,                   /*  1 green          */
	A_DOS_CYAN,                    /*  2 cyan           */
	A_DOS_RED,                     /*  3 red            */
	A_DOS_MAGENTA,                 /*  4 magenta        */
	A_DOS_BROWN,                   /*  5 brown          */
	A_DOS_BRIGHT | A_DOS_BLACK,    /*  6 dark grey      */
	A_DOS_BRIGHT | A_DOS_BLUE,     /*  7 light blue     */
	A_DOS_BRIGHT | A_DOS_GREEN,    /*  8 light green    */
	A_DOS_BRIGHT | A_DOS_RED,      /*  9 light red      */
	A_DOS_BRIGHT | A_DOS_MAGENTA,  /* 10 light magenta  */
	A_DOS_BRIGHT | A_DOS_BROWN,    /* 11 yellow         */
	A_DOS_BRIGHT | A_DOS_WHITE,    /* 12 uline          */
	A_DOS_BLUE,                    /* 13 blue           */
	A_DOS_STANDOUT,                /* 14 reverse        */
	A_DOS_BRIGHT | A_DOS_NORMAL,   /* 15 high intensity */
	A_DOS_STANDOUT,                /* bold              */
	0                              /* no more           */
} ;

/*@
 * Reverse and Bold (standout(), bold()) are set differently than their color
 * table counterparts, using dark gray ("light black") as foreground. Visually
 * the difference is minor, but perhaps it was also meant to circumvent the
 * cur_addch() processing of A_DOS_STANDOUT used for passages/mazes.
 *
 * And surprisingly high()/set_attr(15) is set to normal white (ie, light gray)
 */
static byte monoc_attr[] = {
	A_DOS_NORMAL,      /*  0 normal         */
	A_DOS_NORMAL,      /*  1 green          */
	A_DOS_NORMAL,      /*  2 cyan           */
	A_DOS_NORMAL,      /*  3 red            */
	A_DOS_NORMAL,      /*  4 magenta        */
	A_DOS_NORMAL,      /*  5 brown          */
	A_DOS_NORMAL,      /*  6 dark grey      */
	A_DOS_NORMAL,      /*  7 light blue     */
	A_DOS_NORMAL,      /*  8 light green    */
	A_DOS_NORMAL,      /*  9 light red      */
	A_DOS_NORMAL,      /* 10 light magenta  */
	A_DOS_NORMAL,      /* 11 yellow         */
	A_DOS_BW_ULINE,    /* 12 uline          */
	A_DOS_NORMAL,      /* 13 blue           */
	A_DOS_BW_STANDOUT, /* 14 reverse        */
	A_DOS_NORMAL,      /* 15 white/hight    */
	A_DOS_BW_STANDOUT, /* 16 bold           */
	0                  /* no more           */
} ;

static byte *at_table;

/*@
 * Changes in ASCII chars from Unix Rogue (and roguelike ASCII tradition):
 * AMULET: ',' to '&'. Not meant to be subtle in DOS
 * BMAGIC: '+' to '~'. '+' is ASCII for door. BMAGIC is a DOS-only extension.
 *
 * Note: Swapping '+' with '{' would yield a better visual result, as '{' is
 * great as door and it better matches DOS CP437 char 0xCE ╬. But I will not be
 * the one to break such a well-known convention, and get flamed for heresy.
 * You do it.
 */
static CCODE ctab[] = {
		/*
		 * Dungeon chars. If a char in this block is not unique, such as
		 * the ASCII for room corners, cur_inch() reverse search will map
		 * them back to a different DOS char. So choose them carefully.
		 */
		{'@', L"\x263A", PLAYER},     // ☺
		{'^', L"\x2666", TRAP},       // ♦
		{':', L"\x2663", FOOD},       // ♣
		{']', L"\x25D8", ARMOR},      // ◘
		{'=', L"\x25CB", RING},       // ○
		{'&', L"\x2640", AMULET},     // ♀
		{'?', L"\x266A", SCROLL},     // ♪
		{'*', L"\x263C", GOLD},       // ☼
		{')', L"\x2191", WEAPON},     // ↑
		{'!', L"\x00A1", POTION},     // ¡
		{'#', L"\x2592", PASSAGE},    // ▒
		{'+', L"\x256C", DOOR},       // ╬
		{'/', L"\x03C4", STICK},      // τ
		{'.', L"\x00B7", FLOOR},      // ·
		{'%', L"\x2261", STAIRS},     // ≡
//		{'$', L"$",      MAGIC},      // $, maps to itself
//		{'~', L"~",      BMAGIC},     // ~, maps to itself
		{'|', L"\x2551", VWALL},      // ║
		{'-', L"\x2550", HWALL},      // ═
		{'-', L"\x2554", ULWALL},     // ╔
		{'-', L"\x2557", URWALL},     // ╗
		{'-', L"\x255A", LLWALL},     // ╚
		{'-', L"\x255D", LRWALL},     // ╝

		// Title screen
		{'X', L"\x2563", DVLEFT},     // ╣
		{'X', L"\x2560", DVRIGHT},    // ╠

		// F1 help screen and save game message
		{'<', L"\x25C4", 0x11},       // ◄ 'Enter' char 1
		{'/', L"\x2518", 0xD9},       // ┘ 'Enter' char 2

		// F1 help screen
		{'^', L"\x2191", 0x18},       // ↑ up (same as WEAPON)
		{'v', L"\x2193", 0x19},       // ↓ down
		{'>', L"\x2192", 0x1A},       // → right
		{'<', L"\x2190", 0x1B},       // ← left

		{'`', L"`", 0}  // if ` appears on screen, something went wrong!
};

static CCODE btab[] = {
		// single-width box glyphs
		{'|', L"\x2502", VLINE},      // │
		{'-', L"\x2500", HLINE},      // ─
		{'.', L"\x250C", ULCORNER},   // ┌
		{'.', L"\x2510", URCORNER},   // ┐
		{'`', L"\x2514", LLCORNER},   // └
		{'\'',L"\x2518", LRCORNER},   // ┘

		// same as *WALL set, but used in boxes, with different ASCII
		{'H', L"\x2551", DVLINE},     // ║
		{'=', L"\x2550", DHLINE},     // ═
		{'#', L"\x2554", DULCORNER},  // ╔
		{'#', L"\x2557", DURCORNER},  // ╗
		{'#', L"\x255A", DLLCORNER},  // ╚
		{'#', L"\x255D", DLRCORNER},  // ╝

		// same as PASSAGE, used for curtain
		{'#', L"\x2592", FILLER},     // ▒

		{'\0', L"", 0}
};

/*@
 * Numpad keys missing from the terminfo data of some common terminals
 * See define_keys()
 *
 * CSI: Control Sequence Introducer: ESC [
 * SS3: Single Shift Select of G3 Character Set (SS3  is 0x8f): ESC O
 *      This affects next character only.
 *
 * xterm
 * *	SS3 j	* multiply
 * +	SS3 k	+ add
 * -	SS3 m	- minus
 * .	SS3 n	. period (VT220)
 * /	SS3 o	/ divide   ()
 * ENT	0527	KEY_ENTER
 * 7	0406	KEY_HOME
 * 5	0536	KEY_B2
 * 1	0550	KEY_END
 *
 * gnome-terminal TERM=xterm
 * .	.
 * 7	CSI 1 ~	Home (VT220)
 * 5	CSI E	5 begin (kb2/K2)
 * 1	CSI 4 ~	End (VT220)
 *
 * gnome-terminal TERM=gnome
 * ENT	SS3 M	CR, enter (kent/@8)
 * 7	0552	KEY_FIND
 * 1	0601	KEY_SELECT
 */
static TTYSEQ ttymap[] = {
		{TTY_SS3 "j", '*'},
		{TTY_SS3 "k", '+'},
		{TTY_SS3 "m", '-'},
		{TTY_SS3 "n", '.'},
		{TTY_SS3 "o", '/'},
		{TTY_SS3 "M",  KEY_ENTER},
		{TTY_CSI "E",  KEY_B2},
		{TTY_CSI "1~", KEY_HOME},
		{TTY_CSI "4~", KEY_END},
};

static byte dbl_box[BX_SIZE] = {
	DULCORNER, DURCORNER, DLLCORNER, DLRCORNER, DVLINE, DHLINE, DHLINE
};

static byte sng_box[BX_SIZE] = {
	ULCORNER, URCORNER, LLCORNER, LRCORNER, VLINE, HLINE, HLINE
};

/*@ unused
static byte fat_box[BX_SIZE] = {
	0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdf, 0xdc
};
*/
static byte spc_box[BX_SIZE] = {
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
	{KEY_ENTER,	'\n'}, //@ Keypad Enter
	{KEY_HOME,	'y'},
	{KEY_FIND,	'y'},  //@ Keypad Home (7) in some terminals
	{KEY_A1,	'y'},  //@ Keypad upper left (7)
	{KEY_UP,	'k'},
	{KEY_PPAGE,	'u'},  //@ Page Up
	{KEY_A3,	'u'},  //@ Keypad upper right (9)
	{KEY_LEFT,	'h'},
	{KEY_RIGHT,	'l'},
	{KEY_END,	'b'},
	{KEY_SELECT,	'b'},  //@ Keypad End (1) in some terminals
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
	{KEY_F(57),	'F'}  //@ ALT+F9
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


/*@
 * Read a character from user input. getch() with non-blocking capability
 *
 * msdelay has same meaning as delay in timeout(): If no key was pressed after
 * msdelay milliseconds, return ERR. Negative values will block until a key
 * is pressed.
 *
 * After the wgetch() call, input will always restore to blocking mode using
 * nodelay(FALSE);
 *
 * Return ERR on non-ASCII chars and on window resize.
 */
int
cur_getch_timeout(int msdelay)
{
#ifdef ROGUE_DOS_CURSES
	/*@
	 * getchar() is not a true replacement, as asm version has no echo and no
	 * buffering. Asm was also non-blocking, but only used after no_char(),
	 * a combination that effectively blocks until input.
	 */
	return getchar();
#else
	int ch = 0;

	wtimeout(stdscr, msdelay);

#ifdef _XOPEN_CURSES
	wint_t wchi;
	int ret;
	if ((ret = wget_wch(stdscr, &wchi)) == ERR || (ret == OK && !isascii(wchi)))
	{
		// we're only interested in ASCII input
		ch = ERR;
	}
	else
	{
		// KEY_* codes always fit int, so no need for any special test
		ch = (int)wchi;
	}
#else
	ch = wgetch(stdscr);
#endif  // _XOPEN_CURSES
	// mask-map custom keys
	if (ch != ERR)
	{
		ch = KEY_MASK & ch;
	}

	// window resize needs special handling. all others go through xlate/xtab
	if (ch == KEY_RESIZE)
	{
		resize_screen();
		ch = ERR;
	}
	nodelay(stdscr, FALSE);
	return ch;
#endif  // ROGUE_DOS_CURSES
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
int
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
	return wmove(stdscr, row, col);
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
 * Originally in zoom.asm by the name curch()
 *
 * By my understanding, asm function works very similar to putchr():
 * If iscuron, invokes BIOS INT 10h/AH=02h to set cursor position from cache
 * and 10h/AH=08h to read character, else wait retrace (unless no_check) and
 * read directly from Video Memory. The set cursor BIOS call seems quite
 * redundant, as virtually all calls to this, from both the inch() macro and
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
cur_inch(void)
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
	byte chd = 0;
	CCODE *ccp;
	wchar_t wch;  // character on screen
	wchar_t wcr;  // reference character on ctab mapping
#ifdef _XOPEN_CURSES
	cchar_t cch;
	wchar_t wcha[CCHARW_MAX + 1];
	attr_t dummya;
	short dummyc;

	win_wch(stdscr, &cch);
	getcchar(&cch, wcha, &dummya, &dummyc, NULL);
	wch = wcha[0];
#else

	wch = (wchar_t)(A_CHARTEXT & winch(stdscr));
#endif  // _XOPEN_CURSES
	// if not on mapping list, will report as itself
	chd = (byte)wch;

	if (charset == CP437)
	{
		return chd;
	}
	for(ccp = ctab; ccp->dos; ccp++)
	{
		switch (charset)
		{
		default:      wcr = L'\0';
		break;
		case ASCII:   wcr = (wchar_t)ccp->ascii;
		break;
		case UNICODE: wcr = *ccp->unicode;
		break;
		}
		if (wch == wcr)
		{
			chd = ccp->dos;
			break;
		}
	}
	return chd;
#endif  // ROGUE_DOS_CURSES
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
	return cur_inch();
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
		{
			switch(chr)
			{
			case DOOR:
			case VWALL:
			case HWALL:
			case ULWALL:
			case URWALL:
			case LLWALL:
			case LRWALL:
				ch_attr = A_DOS_BROWN;  /* brown */
				break;
			case FLOOR:
				ch_attr = A_DOS_GREEN | A_DOS_BRIGHT;  /* light green */
				break;
			case STAIRS:
				ch_attr = A_DOS_BLACK | A_DOS_BG(A_DOS_GREEN) | A_DOS_BLINK; /* black on green */
				break;
			case TRAP:
				ch_attr = A_DOS_MAGENTA;  /* magenta */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = A_DOS_YELLOW;  /* yellow */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = A_DOS_BLUE | A_DOS_BRIGHT;
				break;
			case FOOD:
				ch_attr = A_DOS_RED;
				break;
			}
		}
		/* if inside a passage or a maze */
		else if (ch_attr == A_DOS_STANDOUT)
		{
			switch(chr)
			{
			case FOOD:
				ch_attr = A_DOS_RED | A_DOS_STANDOUT ;  /* red @ on white */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = A_DOS_YELLOW | A_DOS_STANDOUT;  /* yellow on white */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = A_DOS_BLUE | A_DOS_STANDOUT;  /* blue on white */
				break;
			}
		}
		//@ I suspect STAIRS used with high() is a case that never happen...
		else if (ch_attr == (A_DOS_BRIGHT | A_DOS_NORMAL) && chr == STAIRS)
			ch_attr = A_DOS_BLACK | A_DOS_BG(A_DOS_GREEN) | A_DOS_BLINK;
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
	switch (charset)
	{
	default:
	case ASCII:
		chr = ascii_from_dos(chr, ctab);
		/* no break */
	case CP437:
		waddch(stdscr, chr | attr_from_dos(ch_attr));
		break;
#ifdef _XOPEN_CURSES
	case UNICODE:
		wadd_wch(stdscr, unicode_from_dos(chr, ch_attr, ctab));
		break;
#endif
	}
#endif  // ROGUE_DOS_CURSES
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
#ifdef _XOPEN_CURSES
cchar_t *
unicode_from_dos(byte chd, byte dos_attr, CCODE *mapping)
{
	short color;
	attr_t attrs;

	CCODE *ccp = charcode_from_dos(chd, mapping);
	attrw_from_dos(dos_attr, &attrs, &color);

	setcchar(&cctemp,
			ccp->unicode,
			attrs,
			color,
			NULL);
	return &cctemp;
}
#endif  // _XOPEN_CURSES


void
define_keys(void)
{
#ifdef NCURSES_VERSION
	int i;
	int shift;
	TTYSEQ *ptr, *max;

	// get the shift offset of the bit past KEY_MAX
	for (i=KEY_MAX, shift=1; i>>=1; shift++);

	// define the mask that will be used by cur_getch() and friends
	KEY_MASK = (1 << shift) - 1;

	// define keys. first key gets i>0 to leave room for user terminfo keys
	for (i=8, ptr=ttymap, max=ASIZE(ttymap); ptr < max; ptr++)
	{
		if(!key_defined(ptr->def))
		{
			define_key(ptr->def, ((i++) << shift) | ptr->dest);
		}
	}
#endif
}


byte
ascii_from_dos(byte chd, CCODE *mapping)
{
	return charcode_from_dos(chd, mapping)->ascii;
}


CCODE *
charcode_from_dos(byte chd, CCODE *mapping)
{
	CCODE *ccp;

	// Shortcut for "ordinary" chars that map to themselves
	if (chd == '\0' || chd == '\n' || (isascii(chd) && isprint(chd)))
	{
		ccode.ascii = ccode.dos = *ccode.unicode = chd;
		return &ccode;
	}

	for(ccp = mapping; ccp->dos; ccp++)
	{
		if (chd == ccp->dos)
		{
			return ccp;
		}
	}
	// if not found, will return the sentinel
	return ccp;
}

/*
 * Return a color index, based on DOS char attribute.
 * For now index is in range 0-7, and the interpretation of A_DOS_BRIGHT bit to
 * either `fg index += 8` or `color | [W]A_BOLD` is the caller's responsibility.
 * This could be changed in the future.
 */
short
color_from_dos(byte dos_attr, bool fg)
{
	byte color = (dos_attr >> (fg ? A_DOS_FG_COLOR : A_DOS_BG_COLOR)) & \
			A_DOS_COLOR_MASK;

	// swap red and blue components so DOS index match ANSI's
	return swap_bits(color, 0, 2, 1);
}


/*
 * Convert a DOS/CGA character attribute to its curses equivalent
 *
 * The attribute model used by addch() and attrset() has no distinct type for
 * attributes: addch() expects a chtype OR'ed with A_* constants, and attrset()
 * expects an int, hinting that all A_* constants, as well as any chtype AND'ed
 * with A_ATTRIBUTES, fit int range.
 *
 * Return type chtype was chosen for consistency with termattrs() and vidattr(),
 * which are the only known functions dealing with an isolated set of OR'ed A_*
 * attributes. int could also have been chosen, as per attrset() usage.
 *
 * Return type could be attr_t, a dedicated type for attributes. In ncurses it
 * is typedef'd to chtype, but they are semantically different: attr_t expects
 * to be manipulated using the WA_* constants, and it's not expected to contain
 * color pair information: functions with a attr_t argument also have another
 * argument for color pair of type short.
 *
 * The only functions that deal exclusively with the attr_t model and have no
 * counterpart using the old model are the ones working with cchar_t wide chars
 * ("complex renditions" in ncurses docs). For those there is attrw_from_dos()
 */
chtype
attr_from_dos(byte dos_attr)
{
	chtype attr = A_NORMAL | COLOR_PAIR(0);
	short fg, bg;

	// shortcut to avoid setting (and calculating) a spurious color pair
	if (dos_attr == A_DOS_NORMAL)
		return attr;

	if (dos_attr & A_DOS_BLINK)
		attr |= A_BLINK;

	fg = color_from_dos(dos_attr, TRUE);
	bg = color_from_dos(dos_attr, FALSE);

	if (dos_attr & A_DOS_BRIGHT)
	{
		if (colors < 16)
			attr |= A_BOLD;
		else
			fg += 8;
	}

#ifdef NCURSES_VERSION
	/*
	 * Set terminal reverse attribute when Rogue implies it
	 *
	 * CGA does not have a "reverse" mode, Rogue achieves it by manually
	 * setting a white background with black foreground (black by default,
	 * but foreground could also be set to other colors, see cur_addch()).
	 * Thus, A_DOS_STANDOUT require no special handling and could be treated
	 * like any other color pair, and this block is entirely optional.
	 *
	 * By activating the terminal reverse mode and swapping fg with bg to
	 * revert Rogue's reversal, we allow the default fg/bg terminal colors
	 * to be used instead of hard-coded black on white, making reversed
	 * A_DOS_STANDOUT text consistent with A_DOS_NORMAL text even if user's
	 * terminal color theme is different from Rogue's default.
	 *
	 * This does not work with 8-color terminals: on reversed mode, A_BOLD
	 * operates on the background color, making it impossible to get yellow
	 * as foreground.
	 */
	if (((dos_attr & A_DOS_STANDOUT) == A_DOS_STANDOUT)
			&& colors != 8
			&& use_terminal_fgbg)
	{
		attr |= A_REVERSE;

		short tmp = bg;
		bg = fg;
		fg = tmp;
	}
#endif

	/*
	 * BIG problem here: if colors == 0, we should not use color pairs at
	 * all, but map the entries in monoc_attr from original intentions to
	 * current curses A_* attributes like underline, bold, standout, etc.
	 */
	if (colors > 0)
		attr |= COLOR_PAIR_N(fg, bg);

	return attr;
}


#ifdef _XOPEN_CURSES
void
attrw_from_dos(byte dos_attr, attr_t *attrs, short *color_pair)
{
	/*
	 * A sloppy version could simply assume that attr_t is typedef'd to
	 * chtype and all WA_* == A_*, which is true for current ncurses,
	 * and this function would be simplified to:
	 *
	 * attr_t bute = attr_from_dos(dos_attr);
	 * *attrs = bute & A_ATTRIBUTES & ~A_COLOR,
	 * *color_pair = PAIR_NUMBER(bute);
	 *
	 * Tempting, but we shall not make such assumptions. By the book, boys!
	 */

	short fg, bg;

	*attrs = WA_NORMAL;
	*color_pair = 0;

	if (dos_attr == A_DOS_NORMAL)
		return;

	if (dos_attr & A_DOS_BLINK)
		*attrs |= WA_BLINK;

	fg = color_from_dos(dos_attr, TRUE);
	bg = color_from_dos(dos_attr, FALSE);

	if (dos_attr & A_DOS_BRIGHT)
	{
		if (colors < 16)
			*attrs |= WA_BOLD;
		else
			fg += 8;
	}

#ifdef NCURSES_VERSION
	if (((dos_attr & A_DOS_STANDOUT) == A_DOS_STANDOUT)
			&& colors != 8
			&& use_terminal_fgbg)
	{
		*attrs |= WA_REVERSE;

		short tmp = bg;
		bg = fg;
		fg = tmp;
	}
#endif

	if (colors > 0)
		*color_pair = PAIR_INDEX(fg, bg);
}
#endif  // _XOPEN_CURSES


void
init_curses_colors(void)
{
	int fg, dos_fg, dfg;
	int bg, dos_bg, dbg;
	int i, r, g, b, cube;
	int colormode;
	int cmap[16];

	/*
	 * Not sure if this test should include bwflag, as set via env file.
	 * Original winit() doesn't, as it only cares about actual *hardware*
	 * capabilities. But on modern machines bwflag is the only way for
	 * players to simulate a bw hardware monitor like TTL or IBM's MDA,
	 * which is very different from simply "using no colors": it had bright,
	 * underline, blink, etc. We may consider a way to set this "hardware
	 * bw monitor" mode even if terminal supports colors.
	 */
	if (!has_colors() || COLORS < 8)
	{
		colors = 0;
		return;
	}

	/*
	 * Some notes on colors and mappings:
	 *
	 * DOS only uses 8 basic colors, and the foreground could be bumped to
	 * 16 via bright attribute. So for all code outside this function,
	 * background color index range from 0-7, foreground from 0-7 or 15,
	 * so they only need at most 8 * 16 = 128 color pairs, always accessed
	 * via PAIR_INDEX(fg, bg) or COLOR_PAIR_N(fg, bg) macros.
	 *
	 * Color indexes in DOS (actually, in CGA/VGA) are an RGB bitmap, blue
	 * being the least significant bit, so DOS colors have the Red and Blue
	 * components swapped compared to ANSI colors, which the curses named
	 * constants derive from. color_from_dos() takes care of this, so fg/bg
	 * indexes should be interpreted by ANSI table, ie, 1=Red, 4=Blue, etc
	 *
	 * The actual colors mapped to each of this 16 color indexes depends on
	 * terminal color capabilities:
	 * - If only 8, we achieve the 16 via curses [W]A_BOLD attribute.
	 * - For 16 color terminals, we have a 1:1 mapping
	 * - 88 and 256, we may use the first 16 or remap some (or even all)
	 *   indexes to the 4x4x4 or 6x6x6 color cube. This is useful to make
	 *   color 3 (dark yellow in ANSI) actually look brown (as in CGA)
	 */

	// colormode is only used here, colors is global
	colors = 16;
	if      (COLORS >= 256) colormode = 256;
	else if (COLORS >=  88) colormode =  88;
	else if (COLORS >=  16) colormode =  16;
	else                    colormode = colors = 8;

	switch(colormode)
	{
	case 8:
	case 16:
		cube = 0;
		if (can_change_color() && change_colors)
		{
			colors_changed = TRUE;
		}
		break;
	case  88:
		cube = 4;
		break;
	case 256:
		cube = 6;
		break;
	}

	if (cube)
	{
		for (i = 0; i < colors; i++)
		{
			r = (cube - 1) * CGA_RED(i);
			g = (cube - 1) * CGA_GREEN(i);
			b = (cube - 1) * CGA_BLUE(i);
			cmap[i] = 16 + cube * cube * r + cube * g + b;
		}
	}
	else if (colors_changed)
	{
		for (i = 0; i < colors; i++)
		{
			init_color(i,
					1000 * CGA_RED(i),
					1000 * CGA_GREEN(i),
					1000 * CGA_BLUE(i));
			cmap[i] = i;  // 1:1 mapping
		}
	}
	else
	{
		for (i = 0; i < colors; i++)
		{
			cmap[i] = i;  // 1:1 mapping
		}
	}

	/*@
	 * More notes on color mappings and pairs:
	 *
	 * - Color pair 0 is not initialized, as per recommendation in curses
	 *   documentation, and it is only used when DOS attributes are set to
	 *   A_DOS_NORMAL, for example by cur_standend().
	 *
	 * - The default foreground and background colors used by DOS, as
	 *   defined by A_DOS_NORMAL, are mapped to (COLOR_WHITE, COLOR_BLACK).
	 *   If the curses implementation is ncurses and the user allows it,
	 *   they are mapped to (-1,-1), the default foreground and background
	 *   terminal colors.
	 */

	dos_fg = color_from_dos(A_DOS_NORMAL, TRUE);
	dos_bg = color_from_dos(A_DOS_NORMAL, FALSE);
	if ((A_DOS_NORMAL & A_DOS_BRIGHT) && colors > 8)
		dos_fg += 8;

	dfg = cmap[COLOR_WHITE];
	dbg = cmap[COLOR_BLACK];

#ifdef NCURSES_VERSION
	/*
	 * One could argue that change_colors == TRUE should imply
	 * use_terminal_fgbg == TRUE, but currently they are independent.
	 */
	if (use_terminal_fgbg)
	{
		use_default_colors();
		dfg = dbg = -1;
	}
#else
	use_terminal_fgbg = FALSE;
#endif

	for (bg = 0; bg < colors; bg++)
	{
		for (fg = colors - (bg ? 2 : 1); fg >= 0; fg--)
		{
			init_pair(PAIR_INDEX(fg, bg),
					(fg == dos_fg) ? dfg : cmap[fg],
					(bg == dos_bg) ? dbg : cmap[bg]);
		}
	}

#ifdef ROGUE_DEBUG
	int j;
	printw("COLOR TEST - Displayed colors should match [R,G,B] values\n");
	printw("Color mode: %d colors, using %s\n", colormode,
			cube ? "color cube" :
			colors_changed ? "RGB" :
			colors > 8 ? "ANSI 16" :
			"ANSI 8 + Bold");
	for (i = 0; i < 8; i++)
	{
		printw(" %d [%3d,%3d,%3d] #%02X%02X%02X ",
				i,
				(int)(255 * CGA_RED(i)),
				(int)(255 * CGA_GREEN(i)),
				(int)(255 * CGA_BLUE(i)),
				(int)(255 * CGA_RED(i)),
				(int)(255 * CGA_GREEN(i)),
				(int)(255 * CGA_BLUE(i))
		);
		for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i, 0));
		if (colors > 8)
			for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i + 8, 0));
		else
			for(j=15;j;j--) waddch(stdscr, '#' | COLOR_PAIR_N(i, 0) | A_BOLD);
		printw(" #%02X%02X%02X [%3d,%3d,%3d]\n",
				(int)(255 * CGA_RED(i+8)),
				(int)(255 * CGA_GREEN(i+8)),
				(int)(255 * CGA_BLUE(i+8)),
				(int)(255 * CGA_RED(i+8)),
				(int)(255 * CGA_GREEN(i+8)),
				(int)(255 * CGA_BLUE(i+8))
		);
	}
#endif
}


void
resize_screen()
{
	if ((LINES != cur_LINES) || (COLS != cur_COLS))
	{
		if (resizeterm(cur_LINES, cur_COLS) == OK)
		{
			flushinp();  //@ eat up the generated KEY_RESIZE
		}
		else
		{
			fatal("Could not resize resize terminal to %u x %u\n",
					cur_COLS, cur_LINES);
		}
	}
}
#endif  // ROGUE_DOS_CURSES


void
set_attr(bute)
	int bute;
{
	if (bute < MAXATTR)
		ch_attr = at_table[bute];
	else
		ch_attr = bute;
#ifndef ROGUE_DOS_CURSES
	/*@
	 * XOpen Curses standard and ncurses docs clearly says that both
	 * attrset() and attr_set() should operate on the same video attributes,
	 * regardless if current screen cell is chtype or cchar_t. So we still
	 * use attrset() for ncursesw
	 */
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
winit(void)
{
#ifdef ROGUE_DOS_CURSES
	register int i, cnt;

	/*
	 * Get monitor type
	 */
#ifdef ROGUE_DOS_SCREEN
	//@ if get_mode() also returned BH, it could be used here
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
	 * @ by "restarting" he means "restoring a saved game file"
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
	 * _dsval is an extern set by begin.asm with its DS register value
	 *
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
	if (init_curses)
		return;

	/*@
	 * ROGUE_SCR_TYPE should affect both columns and colors, and scr_type
	 * is also used by game in various contexts with ambiguous meanings.
	 *
	 * My current implementation is "messy", to say the least:
	 * ROGUE_SCR_TYPE is ignored, it does not affect neither columns
	 * (controlled by ROGUE_COLUMNS) nor colors, and scr_type will be
	 * inconsistent with if anything but 80-column color mode is used.
	 *
	 * I see 2 elegant approaches to solve this mess:
	 * - scr_type is "crafted" based on colors and columns, reversing the
	 *   logic in original winit() switch.
	 * - scr_type is completely removed, and all tests based on that are
	 *   changed to match original *intention*, if one can figure that out.
	 *
	 * Not to mention cur_COLS itself should not be defined only at
	 * compile-time, but perhaps also subject to initial terminal size
	 * and/or env file setting.
	 */
	scr_type = ROGUE_SCR_TYPE;

	setenv("ESCDELAY", "25", FALSE);
	initscr();
	init_curses = TRUE;
	if ((LINES < cur_LINES) || (COLS < cur_COLS))
	{
		fatal("%u-column mode requires at least a %u x %u screen\n"
				"Your terminal size is %u x %u\n",
				cur_COLS, cur_COLS, cur_LINES, COLS, LINES);
	}
#ifdef ROGUE_DEBUG
	printw("Real terminal size:  %3u x %3u\n", LINES, COLS);
	printw("Setting up Rogue to: %3u x %3u\n", cur_LINES, cur_COLS);
#endif
	start_color();
	cbreak();  //@ do not buffer input until ENTER
	noecho();  //@ do not echo typed characters
	nodelay(stdscr, FALSE); //@ use a blocking getch() (already the default)
	keypad(stdscr, TRUE);   //@ enable directional arrows, keypad, home, etc

	/*@
	 * Immediately refresh() screen on *add{ch,str}() and friends.
	 * Needed for arrow/bolt animations. See tick_pause() and its callers.
	 * May also be needed in other places, as original did not have refresh()
	 * If this is ever removed, adjust {drop,raise}_curtain() accordingly.
	 */
	immedok(stdscr, TRUE);

	resize_screen();
	define_keys();
	init_curses_colors();

	at_table = colors ? color_attr : monoc_attr;
#ifdef ROGUE_DEBUG
	wgetch(stdscr);
#endif
#endif  // ROGUE_DOS_CURSES
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
		cur_mvinchnstr(line, 0, savewin[line], COLS);
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
		cur_mvaddchnstr(line, 0, savewin[line], COLS);
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

		/*
		 * Curses resets color RGB based on terminfo, which is somewhat
		 * useless, as (1) few terminals have terminfo default colors
		 * entries (linux does, xterm does not), and (2) those terminfo
		 * colors might not be the current ones before game start: user
		 * might have themed the terminal in .bashrc, .Xresources, etc.
		 *
		 * So we have 2 choices: we can redefine colors back to ANSI's
		 * default RGB, which is also useless on (2), or we can try
		 * `system("type reset 2>/dev/null && reset");`, which reset
		 * colors on some terminals (xterm, but not gnome-terminal)
		 *
		 * There's also a 3rd choice: do nothing! After all, user told
		 * us to change_colors = TRUE, didn't they? So we did it :)
		 *
		 * For now, I'll settle with #3 ;)
		 *
		if (colors_changed)
		{
			// ?
		}
		 */
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
int
cur_line(byte chd, int length, bool orientation)
{
	chtype ch;
#ifdef _XOPEN_CURSES
	cchar_t *cch;
#endif

	switch (charset)
	{
	default:
	case ASCII:
		ch = ascii_from_dos(chd, btab);
		if (ch == '\0')
			ch = ascii_from_dos(chd, ctab);
		chd = (byte)ch;
		/* no break */
	case CP437:
		ch = chd | attr_from_dos(ch_attr);
		if (orientation == VERTICAL)
			wvline(stdscr, ch, length);
		else
			whline(stdscr, ch, length);
		break;
#ifdef _XOPEN_CURSES
	case UNICODE:
		cch = unicode_from_dos(chd, ch_attr, btab);
		if (cch->chars[0] == L'\0')
			cch = unicode_from_dos(chd, ch_attr, ctab);
		if (orientation == VERTICAL)
			wvline_set(stdscr, cch, length);
		else
			whline_set(stdscr, cch, length);
		break;
#endif
	}
	return OK;
}

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
	bool wason;
	int i;
	int r,c;

	wason = cursor(FALSE);
	getrc(&r,&c);

#ifdef ROGUE_DOS_CURSES
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
#else
	cur_mvhline(ul_r, ul_c+1, box[BX_HT], i = (lr_c - ul_c - 1));
	cur_mvhline(lr_r, ul_c+1, box[BX_HB], i);
	cur_mvvline(ul_r+1, ul_c, box[BX_VW], i = (lr_r - ul_r - 1));
	cur_mvvline(ul_r+1, lr_c, box[BX_VW], i);

	//@ corners - do not go through cur_addch(), different mapping
	cur_mvhline(ul_r,ul_c,box[BX_UL], 1);
	cur_mvhline(ul_r,lr_c,box[BX_UR], 1);
	cur_mvhline(lr_r,ul_c,box[BX_LL], 1);
	cur_mvhline(lr_r,lr_c,box[BX_LR], 1);
#endif
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


/*@
 * Repeat a character cnt times, advancing the cursor
 * Use current attribute, and do not go through cur_addch() processing
 */
void
repchr(byte chr, int cnt)
{
#ifdef ROGUE_DOS_CURSES
	while(cnt-- > 0) {
		putchr(chr);
		c_col++;
	}
#else
	int c_row, c_col;
	getyx(stdscr, c_row, c_col);
	cur_hline(chr, cnt);
	wmove(stdscr, c_row, c_col + cnt);
#endif
}

/*
 * Clear the screen in an interesting fashion
 */
void
implode()
{
	int j, delay, r, c, cinc = COLS/10/2, er, ec;

	er = (COLS == 80 ? LINES-3 : LINES-4);
#ifdef ROGUE_DOS_CURSES
	/*
	 * If the curtain is down, just clear the memory
	 */
	if (scr_ds == svwin_ds) {
		wsetmem(savewin, (er + 1) * COLS, 0x0720);
		return;
	}
	delay = scr_type == 7 ? 500 : 10;
#else
	delay = 50;
#endif
	for (r = 0,c = 0,ec = COLS-1; r < 10; r++,c += cinc,er--,ec -= cinc) {
		vbox(sng_box, r, c, er, ec);
		wrefresh(stdscr);
		msleep(delay);
		for (j = r+1; j <= er-1; j++) {
#ifdef ROGUE_DOS_CURSES
			cur_move(j, c+1); repchr(' ', cinc-1);
			cur_move(j, ec-cinc+1); repchr(' ', cinc-1);
#else
			cur_mvhline(j, c+1, ' ', cinc-1);
			cur_mvhline(j, ec-cinc+1, ' ', cinc-1);
#endif
		}
		vbox(spc_box, r, c, er, ec);
	}
}


#ifdef ROGUE_DOS_CURSES
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
	vbox(sng_box, 0, 0, LINES-1, COLS-1);
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
#else
/*@
 * I am breaking a tradition here by replacing the whole function with a new
 * one instead of changing just the necessary bits of the original.
 * But with <curses.h> and md_nanosleep() the original would be severely
 * mutilated with at least 4 additional #ifdefs blocks, destroying readability,
 * for very little gain. So new it is.
 *
 * Note on the new version: "proper" drop/raise curtain routine should actually
 * redirect all output from addch() and possibly many others, to a buffer such
 * as savewin, just like the original, or to a new curses window/panel/screen.
 * This would require a somewhat complex mechanism of an "effective WINDOW"
 * pointer much similar to the very scr_ds / svwin_ds / old_page_no low level
 * juggling I'm trying to remove from the project.
 *
 * So, for simplicity's sake, currently drop_curtain() merely disables screen
 * (immediate) refresh, and all drawing is still performed stdscr, the curses
 * virtual screen. raise_curtain() takes care of the rest.
 */
/*@
 * Display a curtain down animation and disable screen refresh
 */
void
drop_curtain(void)
{
	int r;
	int delay = CURTAIN_TIME / LINES;

	immedok(stdscr, FALSE);

	cursor(FALSE);
	green();
	vbox(sng_box, 0, 0, LINES-1, COLS-1);
	cur_mvinchnstr(0, 0, curtain[0], COLS);
	wrefresh(stdscr);
	yellow();
	for (r = 1; r < LINES-1; r++) {
		cur_mvhline(r, 1, FILLER, COLS-2);
		cur_mvinchnstr(r, 0, curtain[r], COLS);
		wrefresh(stdscr);
		msleep(delay);
	}
	cur_mvinchnstr(LINES-1, 0, curtain[LINES-1], COLS);
	cur_move(0,0);
	cur_standend();
	wclear(stdscr);
}


/*@
 * Display a curtain up animation and re-enable screen refresh
 */
void
raise_curtain(void)
{
	int line;
	int c_row, c_col;
	int delay = CURTAIN_TIME / LINES;

	// save current screen
	getyx(stdscr, c_row, c_col);
	wdump();

	// restore and display the curtain
	for (line = 0; line < LINES; line++)
	{
		cur_mvaddchnstr(line, 0, curtain[line], COLS);
	}
	wrefresh(stdscr);

	// progressively restore screen
	for (line = LINES-1; line >= 0; line--)
	{
		cur_mvaddchnstr(line, 0, savewin[line], COLS);
		wrefresh(stdscr);
		msleep(delay);
	}
	wmove(stdscr, c_row, c_col);
	is_saved = FALSE;

	immedok(stdscr, TRUE);
}
#endif


#ifdef ROGUE_DOS_CURSES
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
#endif


byte
get_mode(void)
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
 * by ESCAPE and TRUE otherwise. In case of abortion, str could either
 * keep typed string or set first char to '\0', effectively blanking str.
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
#ifdef ROGUE_DOS_CURSES
		if((ch = cur_getch()) == EOF)
		{
			ch = '\n';
		}
#else
		//@ Blocking getch() is fine, as SIG2() is not called anyway
		while ((ch = wgetch(stdscr)) == ERR);
		if (ch > KEY_MIN)
		{
			ch = KEY_MASK & ch;
		}
#endif
		switch(ch)
		{
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
			case KEY_RESIZE:
				resize_screen();
				break;
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
#ifndef ROGUE_DOS_CURSES
			case KEY_ENTER:
#endif
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
