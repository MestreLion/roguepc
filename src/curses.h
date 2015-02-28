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

//@ Function "overrides"
#define	wclear	clear
#define	endwin		wclose
#define mvwaddch(w,a,b,c)	mvaddch(a,b,c)
#define getyx(a,b,c)	getxy(&b,&c)
#define getxy	getrc
#define	inch()	(0xff&curch())

/*@
 * Global variables declarations. All defined in curses.c
 */
extern int LINES, COLS;
extern int is_saved;
extern bool iscuron;
extern int old_page_no;
extern int no_check;
extern int scr_ds;
extern int svwin_ds;
extern int scr_type;

/*
 * we need to know location of screen being saved
 * @ used in save.c
 */
extern char savewin[];
