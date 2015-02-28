/*@
 * Header for curses-related objects used by both the game files and
 * the internal curses.c
 *
 * Some of the defines, the ones part of an informal group such as the colors,
 * are not actually used by both, and may even not be used at all, but were
 * kept together here for consistency and completeness.
 */

/*@
 * This color/bw checks are inconsistent with each other:
 * scr_type 0 and 2 evaluate as TRUE for both (but they are mono),
 * scr_type 7 evaluate as FALSE for both (also mono)
 * See winit()
 */
#define is_color (scr_type!=7)
#define is_bw (scr_type==0 || scr_type==2)

#define standend() set_attr(0)
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
#define standout() set_attr(14)
#define high() set_attr(15)
#define bold() set_attr(16)

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

/*@
 * Function prototypes
 */
void	clear(void);
bool	cursor(bool ison); //@ curs_set()
void	getrc(int *rp, int *cp);
void	clrtoeol(void);
void	mvaddstr(int r, int c, char *s);
void	mvaddch(int r, int c, byte chr);
byte	mvinch(int r, int c);
int 	addch(byte chr);
void	addstr(char *s);
void	set_attr(int bute);
void	winit(void);
void	forcebw(void);
void	wdump(void);
void	wrestor(void);
void	wclose(void);
void	box(int ul_r, int ul_c, int lr_r, int lr_c);
void	center(int row, char *string);
void	printw(); //@ char *msg, int a1, ..., int a8
void	blot_out(int ul_row, int ul_col, int lr_row, int lr_col);
void	implode();
void	drop_curtain();
void	raise_curtain();
void	switch_page(int pn);
byte	get_mode();
byte	video_mode(int type);

//@ originally in dos.asm
void	beep();

//@ originally in zoom.asm
void	move();
void	putchr();
int 	curch();
