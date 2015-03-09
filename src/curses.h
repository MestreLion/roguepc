/*
 *  Cursor motion header for Monochrome display
 */

/*@
 * Contains only the curses-related declarations needed for the game files.
 *
 * Headers used only by curses.c were moved to curses_dos.h
 * Headers used by both curses.c and game files were moved to curses_common.h
 * Headers not related to curses or provided externally were moved elsewhere.
 * Unused headers were removed.
 *
 * The DOS curses implementation, curses.c, shall NOT include this header
 *
 * This is, along with the included curses_common.h header, is the curses
 * public API as used by the game.
 */

#include "curses_common.h"

#define stdscr	NULL
#define hw	stdscr
#define eatme	stdscr

//@ Original macros
#define	wclear	clear
#define mvwaddch(w,a,b,c)	mvaddch(a,b,c)
#define getyx(a,b,c)	getxy(&b,&c)
#define getxy	getrc

//@ Modified macros
#define inch	curch
#define standend	cur_standend
#define standout	cur_standout
#define endwin	cur_endwin

//@ Function mappings
#define beep	cur_beep
#define move	cur_move
#define clear	cur_clear
#define clrtoeol	cur_clrtoeol
#define mvaddstr	cur_mvaddstr
#define mvaddch	cur_mvaddch
#define mvinch	cur_mvinch
#define addch	cur_addch
#define addstr	cur_addstr
#define box	cur_box
#define printw	cur_printw
#define getch cur_getch


/*@
 * Global variables declarations. All defined in curses.c
 */
extern int LINES, COLS;
extern int is_saved;
extern int no_check;
extern int scr_type;
#ifdef ROGUE_DOS_CURSES
extern bool iscuron;
extern int old_page_no;
extern int scr_ds;
extern int svwin_ds;
#endif

/*
 * we need to know location of screen being saved
 * @ used in save.c
 */
extern char savewin[];
