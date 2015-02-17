/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	5.1 (Berkeley) 5/11/82
 */

/*@
 * Also standard library includes, defines and "overrides",
 * assembly function declarations, and a bunch of global variables.
 *
 * The plan is to gradually move all platform-agnostic vars to rogue.h,
 * and keep here only the definitions for mach_dep.c (as originally intended)
 * and other platform-dependent modules such as curses, env, croot, and
 * perhaps also load, save, protect.
 *
 * Standard Library includes will also remain here, as original code did not
 * (explicitly) use any. After all, Rogue is pre-ANSI C.
 *
 * Assembly functions will be replaced either by standard library equivalents
 * or functions in mach_dep.
 *
 * When port is successful, maybe this will be renamed mach_dep.h to avoid
 * confusion with extern.c, which is the definition of global game vars
 * currently declared both here and in rogue.h
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#define MAXSTR		80	/* maximum length of strings */
#define MAXLINES	25	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */

//@ moved from rogue.h for assembly functions global variables declarations
/*
 *  MANX C compiler funnies
 */
typedef unsigned char byte;
typedef unsigned char bool;

//@ moved from curses.h so it's close to 'bool' definition
#define TRUE 	1
#define FALSE	0


/*
 * Now all the global variables
 */
extern int maxitems;
extern int maxrow;
extern char *end_sb, *end_mem, *startmem;
extern char *_top, *_base;
extern int LINES, COLS;
extern int is_saved;
extern int scr_type;
extern int reinit;
extern int revno, verno;
extern int is_me;
extern int iguess;
extern int bailout;

extern char s_menu[], s_name[], s_fruit[], s_score[], s_save[], s_macro[];
extern char s_drive[], s_screen[];
extern char nullstr[], *it, *tbuf, *you, *no_mem;

extern char *s_guess[], *p_guess[], *r_guess[], *ws_guess[];
extern char f_damage[];

extern bool amulet, after, again, askme, door_stop, expert, fastmode,
			faststate, fight_flush, firstmove, in_shell, jump,
			noscore, passgo, playing, running, save_msg, saw_amulet,
			slow_invent, terse, was_trapped, wizard;

extern bool p_know[], r_know[], s_know[], ws_know[];

extern char *a_names[], file_name[], fruit[], *flash,
		*he_man[], *helpcoms[], *helpobjs[],
		home[], huh[], macro[], *intense, outbuf[], *p_colors[],
		*prbuf, *r_stones[], *release, runch,
		*typeahead, take, *w_names[], whoami[],
		*ws_made[], *ws_type[];

extern byte *_level, *_flags;


extern int	a_chances[], a_class[], count, dnum, food_left,
		fung_hit, group, hungry_state, inpack, lastscore,
		level, max_level, mpos, no_command, no_food, no_move,
		ntraps, purse, quiet, total;

extern long	seed, *e_levels;

extern int hit_mul;
extern char *your_na, *kild_by;
extern int goodchk;
extern char *_whoami;
extern int cksum;

/*
 * Function types
 */
/*@
 * For the assembly files only. C functions were moved to rogue.h
 * Function signature was inferred from their standard counterparts, if any,
 * and their usage in rogue. This is just a stub for now, not an accurate
 * documentation on signature and sizes.
 */

//@ begin.asm
//@ no symbols directly referenced by C code other than croot.c.

//@ csav.asm
//@ it seems no symbols are directly referenced by any C code.

//@ dos.asm
void	dmain(), dmaout(), COFF(), beep(), out(), pokeb(), wsetmem(),
		_halt();
byte	getch();
bool	no_char();  //@ actually return only 0 or 1, so a "true" bool
int	peekb(), clock(), getds(), csum();

//@ sbrk.asm
char *brk(), *sbrk();

//@ zoom.asm
void	move(), putchr();
int	curch();


/*@
 * Functions and constants from libc
 */

//@ is{alpha,digit,upper,...}() and to{ascii,upper,lower}() families
#include <ctype.h>

//@ str{len,cat,cpy,cmp,chr}() and possibly others
#include <string.h>
#define bcopy(a,b)	memmove(&(b),&(a),sizeof(a))
#define stpchr	strchr
#define setmem	memset

//@ sprintf(), f{open,read,seek,write,close}(), remove()
#include <stdio.h>

//@ atoi(), NULL, EXIT_*
#include <stdlib.h>
#define exit	exit_croot	//@ (pretend to) use croot's exit() for now
#define setenv	setenv_file	//@ use env.c fake environment
#define srand	srand_time	//@ use its rogue's own RNG mechanics


#ifdef LOG
extern int captains_log;
#endif //LOG
