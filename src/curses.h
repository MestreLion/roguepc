/*
 *  Cursor motion header for Monochrome display
 */

#ifndef ROGUE_SCR_TYPE
#define ROGUE_SCR_TYPE 3  //@ 80x25 Color. See winit() for other values
#endif

#define abs(x) ((x)<0?-(x):(x))

#define refresh	stub

#define	curscr	NULL
#define	stdscr	NULL
#define hw	NULL

#define	BUFSIZE	128

#define wmove(a,b,c)	move(b,c)

#define	wclear	clear
#define fputs(a,b)	addstr(a)
#define puts(s)	addstr(s)

#define wrefresh	stub
#define	clearok		stub
#define leaveok		stub
#define	endwin		wclose
#define touchwin	stub

#define gets	cgets

#define waddstr(w,s)	addstr(s)
#define mvwaddstr(w,a,b,c)	mvaddstr(a,b,c)
#define mvwaddch(w,a,b,c)	mvaddch(a,b,c)

#define getyx(a,b,c)	getxy(&b,&c)
#define getxy	getrc

#define	inch()	(0xff&curch())

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

#define BX_UL	0
#define BX_UR	1
#define BX_LL	2
#define BX_LR	3
#define BX_VW	4
#define BX_HT	5
#define BX_HB	6
#define BX_SIZE	7

//@ in ncurses, as unsigned long. see wmemset()
typedef uint16_t	chtype;  // character with attributes


/*@
 * Global variables declarations. All defined in curses.c
 */
extern int LINES, COLS;
extern int is_saved;
extern bool iscuron;
extern int ch_attr;  //@ only used in zoom.asm
extern int old_page_no;
extern int no_check;
extern int scr_ds;
extern int svwin_ds;
extern int scr_type;
extern int page_no;  //@ only used in zoom.asm
extern int scr_row[];  //@ only used in zoom.asm

/*
 * we need to know location of screen being saved
 * @ used in save.c
 */
extern char savewin[];

/*@
 * Function declarations
 */
void	clear();
bool	cursor(bool ison); //@ curs_set()
void	getrc(int *rp, int *cp);
void	real_rc(int pn, int *rp, int *cp);
void	clrtoeol();
void	mvaddstr(int r, int c, char *s);
void	mvaddch(int r, int c, char chr);
byte	mvinch(int r, int c);
int 	addch(byte chr);
void	addstr(char *s);
void	set_attr(int bute);
void	error();  //@ int mline, char *msg, int a1, ..., int a5
void	winit();
void	forcebw();
void	wdump();
char	*sav_win();
void	res_win();
void	wrestor();
void	wclose();
void	box(int ul_r, int ul_c, int lr_r, int lr_c);
void	vbox(byte box[BX_SIZE], int ul_r, int ul_c, int lr_r, int lr_c);
void	center(int row, char *string);
void	printw(); //@ char *msg, int a1, ..., int a8
void	scroll_up(int start_row, int end_row, int nlines);
void	scroll_dn(int start_row, int end_row, int nlines);
void	scroll();
void	blot_out(int ul_row, int ul_col, int lr_row, int lr_col);
void	repchr(int chr, int cnt);
void	fixup();
void	implode();
void	drop_curtain();
void	raise_curtain();
void	switch_page(int pn);
byte	get_mode();
byte	video_mode(int type);

//@ originally in dos.asm
void	beep();
void	wsetmem();

//@ originally in zoom.asm
void	move();
void	putchr();
int 	curch();
