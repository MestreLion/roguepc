/*@
 * Headers used only by the DOS curses implementation curses.c
 *
 * It is hereby considered private implementation details and as such it
 * should NOT be included curses.h or any game files.
 */

#ifndef ROGUE_SCR_TYPE
#define ROGUE_SCR_TYPE 3  //@ 80x25 Color. See winit() for other values
#endif

#define BX_UL	0
#define BX_UR	1
#define BX_LL	2
#define BX_LR	3
#define BX_VW	4
#define BX_HT	5
#define BX_HB	6
#define BX_SIZE	7

//@ max is also in rogue.h
#define max(a,b)	((a) > (b) ? (a) : (b))
#define min(a,b)	((a) < (b) ? (a) : (b))

#ifdef ROGUE_DOS_CURSES
//@ in ncurses, as unsigned long. see wsetmem()
typedef uint16_t	chtype;  // character with attributes
#endif


/*@
 * Function prototypes
 * Some are unused
 */
void	real_rc(int pn, int *rp, int *cp);
char	*sav_win(void);
void	res_win(void);
void	vbox(byte box[BX_SIZE], int ul_r, int ul_c, int lr_r, int lr_c);
void	repchr(int chr, int cnt);
#ifdef ROGUE_DOS_CURSES
void	error(int mline, char *msg, int a1, int a2, int a3, int a4, int a5);
void	set_cursor(void);
void	scroll_up(int start_row, int end_row, int nlines);
void	scroll_dn(int start_row, int end_row, int nlines);
void	scroll(void);
void	fixup(void);

//@ originally in dos.asm
void	wsetmem(void *buffer, int count, chtype attrchar);
#endif

//@ not in original Rogue
#ifndef ROGUE_DOS_CURSES
attr_t	attr_get_from_dos(byte attr);
#endif
