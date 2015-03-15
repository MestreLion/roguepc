/*@
 * Header for curses-related objects used by both the game files and
 * the internal curses.c
 *
 * Some of the defines, the ones part of an informal group such as the colors,
 * are not actually used by both, and may even not be used at all, but were
 * kept together here for consistency and completeness.
 */

// Available charsets
#define ASCII	1
#define CP437	2
#define UNICODE	3

// User-selected charset
#ifndef ROGUE_CHARSET
	// Factory default charset
	#define ROGUE_CHARSET	UNICODE
#endif
// UNICODE is subject to curses wide char availability
#if ROGUE_CHARSET == UNICODE && !defined (_XOPEN_CURSES)
	#undef  ROGUE_CHARSET
	#define ROGUE_CHARSET	ASCII
#endif


// Convenience defines
#if   ROGUE_CHARSET == ASCII
	#define ROGUE_ASCII   1
	#undef  ROGUE_CP437
	#undef  ROGUE_UNICODE
#elif ROGUE_CHARSET == CP437
	#undef  ROGUE_ASCII
	#define ROGUE_CP437   1
	#undef  ROGUE_UNICODE
#elif ROGUE_CHARSET == UNICODE
	#undef  ROGUE_ASCII
	#undef  ROGUE_CP437
	#define ROGUE_UNICODE 1
#endif

//@ Columns mode
#ifndef ROGUE_COLUMNS
#define ROGUE_COLUMNS 80
#endif

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 * @ Heed the warning! 80 and 25 are hard coded in many places... sigh
 * @ moved from rogue.h
 */
#define MAXSTR  	80	/* maximum length of strings */
#define MAXLINES	25	/* maximum number of screen lines used */
#define MAXCOLS 	80	/* maximum number of screen columns used */


/*@
 * This color/bw checks are inconsistent with each other:
 * scr_type 0 and 2 evaluate as TRUE for both (but they are mono),
 * scr_type 7 evaluate as FALSE for both (also mono)
 * See winit()
 */
#define is_color (scr_type!=7)
#define is_bw (scr_type==0 || scr_type==2)

//@ moved from rogue.h
#ifndef CTRL
#define CTRL(ch)	((ch) & 037)
#endif

#define cur_standend() set_attr(0)
#define green() set_attr(1)
#define cyan() set_attr(2)
#define red() set_attr(3)
#define magenta() set_attr(4)
#define brown() set_attr(5)
#define dgrey() set_attr(6)
#define lblue() set_attr(7)
#define lgrey() set_attr(8)
#define lred() set_attr(9)
#define lmagenta() set_attr(10)
#define yellow() set_attr(11)
#define uline() set_attr(12)
#define blue() set_attr(13)
#define cur_standout() set_attr(14)
#define high() set_attr(15)
#define bold() set_attr(16)

/*@
 * The DOS attribute set on standend(), also the initial value of ch_attr
 * Light Gray ("non-bright White") on Black
 */
#define A_DOS_NORMAL	0x07

/*
 * Things that appear on the screens
 * @ moved from rogue.h
 */
#define PASSAGE		(0xb1)
#define DOOR		(0xce)
#define FLOOR		(0xfa)
#define PLAYER		(0x01)
#define TRAP		(0x04)
#define STAIRS		(0xf0)
#define GOLD		(0x0f)
#define POTION		(0xad)
#define SCROLL		(0x0d)
#define MAGIC		'$'
#define BMAGIC		'+'
#define FOOD		(0x05)
#define STICK		(0xe7)
#define ARMOR		(0x08)
#define AMULET		(0x0c)
#define RING		(0x09)
#define WEAPON		(0x18)
#define CALLABLE	-1

#define VWALL	(0xba)
#define HWALL	(0xcd)
#define ULWALL	(0xc9)
#define URWALL	(0xbb)
#define LLWALL	(0xc8)
#define LRWALL	(0xbc)

//@ The following were not in original - values were hard-coded

//@ single-width box glyphs
#define HLINE	(0xc4)
#define VLINE	(0xb3)
#define CORNER	'+'  //@ unused, added just for completeness
#define ULCORNER	(0xda)
#define URCORNER	(0xbf)
#define LLCORNER	(0xc0)
#define LRCORNER	(0xd9)

//@ double-width box glyphs
#define DHLINE	HWALL  // 205 in credits()
#define DVLINE	VWALL
#define DCORNER	'#'  //@ also unused
#define DULCORNER	ULWALL
#define DURCORNER	URWALL
#define DLLCORNER	LLWALL
#define DLRCORNER	LRWALL

//@ only used in credits()
#define DVLEFT	(0xb9)  //@ 185
#define DVRIGHT	(0xcc)  //@ 204

#define FILLER	PASSAGE


//@ moved from rogue.h
#define ESCAPE	(27)

//@ same as ERR, but different semantics
#define NOCHAR	(-1)

//@ total time, in milliseconds, for each drop and raise curtain animation
#define CURTAIN_TIME	1500

/*@
 * Function prototypes
 * Names with 'cur_' prefix were renamed to avoid conflict with <curses.h>
 */
byte	xlate_ch(int ch);
void	cur_clear(void);
bool	cursor(bool ison);
void	getrc(int *rp, int *cp);
void	cur_clrtoeol(void);
void	cur_mvaddstr(int r, int c, char *s);
void	cur_mvaddch(int r, int c, byte chr);
byte	cur_mvinch(int r, int c);
void	cur_addch(byte chr);
void	cur_addstr(char *s);
void	set_attr(int bute);
void	winit(void);
void	forcebw(void);
void	wdump(void);
void	wrestor(void);
void	cur_endwin(void);
void	cur_box(int ul_r, int ul_c, int lr_r, int lr_c);
void	center(int row, char *string);
void	cur_printw(const char *msg, ...);
void	repchr(byte chr, int cnt);
void	implode(void);
void	drop_curtain(void);
void	raise_curtain(void);
byte	get_mode(void);
byte	video_mode(int type);
#ifdef ROGUE_DOS_CURSES
void	switch_page(int pn);
void	blot_out(int ul_row, int ul_col, int lr_row, int lr_col);
#endif

//@ originally in dos.asm
void	cur_beep(void);
int 	cur_getch_timeout(int msdelay);

//@ originally in zoom.asm
int	cur_move(int row, int col);
byte	cur_inch(void);

//@ moved from io.c
int 	getinfo(char *str, int size);
void	backspace(void);
