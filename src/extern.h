/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	5.1 (Berkeley) 5/11/82
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#define MAXSTR		80	/* maximum length of strings */
#define MAXLINES	25	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */


/*
 * procedure declarations
 */
char *balloc();
char *stpbrk();
char *stpblk();
char *sbrk();

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

extern struct array s_names[], _guesses[];
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

char	*brk(), *ctime(), *getenv(), *sbrk(), *strcat(), *strcpy();

//@ Get size_t and NULL
#include <stddef.h>

//@ From <stdio.h>
int	sprintf(char *str, const char *format, ...);

//@ From <string.h>
size_t	strlen(const char *s);

//@ From <stdlib.h>
#define	EXIT_FAILURE	1	/* Failing exit status.  */
#define	EXIT_SUCCESS	0	/* Successful exit status.  */
void	exit(int status);

//@ From <strings.h>
char	*index(const char *s, int c);


#ifdef LOG
extern int captains_log;
#endif //LOG
