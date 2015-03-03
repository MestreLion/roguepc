/*@
 * Header for curses-related objects used by both the game files and
 * the internal curses.c
 *
 * Some of the defines, the ones part of an informal group such as the colors,
 * are not actually used by both, and may even not be used at all, but were
 * kept together here for consistency and completeness.
 */

#define ROGUE_ASCII 1

/*@
 * This color/bw checks are inconsistent with each other:
 * scr_type 0 and 2 evaluate as TRUE for both (but they are mono),
 * scr_type 7 evaluate as FALSE for both (also mono)
 * See winit()
 */
#define is_color (scr_type!=7)
#define is_bw (scr_type==0 || scr_type==2)

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

/*
 * Things that appear on the screens
 * @ moved from rogue.h
 */
#ifdef ROGUE_ASCII
#define PASSAGE		'#'
#define DOOR		'+'
#define FLOOR		'.'
#define PLAYER		'@'
#define TRAP		'^'
#define STAIRS		'%'
#define GOLD		'*'
#define POTION		'!'
#define SCROLL		'?'
#define MAGIC		'$'
#define BMAGIC		'&'
#define FOOD		':'
#define STICK		'/'
#define ARMOR		']'
#define AMULET		','
#define RING		'='
#define WEAPON		')'
#define CALLABLE	-1

#define VWALL	'|'
#define HWALL	'-'
#define ULWALL	'1'
#define URWALL	'2'
#define LLWALL	'3'
#define LRWALL	'4'

#define VLEFT	'6'
#define VRIGHT	'5'
#else
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

#define VLEFT	(0xb9)  //@ 185
#define VRIGHT	(0xcc)  //@ 204
#endif

/*@
 * Function prototypes
 * Names with 'cur_' prefix were renamed to avoid conflict with <curses.h>
 */
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
void	wclose(void);
void	cur_box(int ul_r, int ul_c, int lr_r, int lr_c);
void	center(int row, char *string);
void	cur_printw(const char *msg, ...);
void	implode();
void	drop_curtain();
void	raise_curtain();
void	switch_page(int pn);
byte	get_mode();
byte	video_mode(int type);
#ifdef ROGUE_DOS_CURSES
void	blot_out(int ul_row, int ul_col, int lr_row, int lr_col);
#endif

//@ originally in dos.asm
void	cur_beep(void);
byte	cur_getch(void);

//@ originally in zoom.asm
void	cur_move(int row, int col);
void	putchr(byte ch);
byte	curch(void);
