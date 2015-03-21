/*
 * Rogue definitions and variable declarations
 *
 * rogue.h	1.4 (AI Design) 12/14/84
 */


#include "extern.h"

/*
 *  Options set for PC rogue
 */

/*
 * copy protection
 */
#define PROTECTED
#define CSUM	-1632
#ifdef PROTECTED
#define P_DAMAGE 6
#else
#define P_DAMAGE 1
#endif //PROTECTED

/*
 * if DEBUG or WIZARD is changed
 * might as well recompile everything
 */
#define HELP
#ifdef ROGUE_DEMO
#define DEMO
#else
#undef DEMO
#endif
#define DEMOTIME 10
/*
 * DEMO
 *      recompile:
 *          save.c
 *	    rip.c
 *          io.c
 *          main.c
 */
#define REV 1
#define VER 48

/*
 * If CODECSUM is changed recompile extern.c
 */
#define SCOREFILE "rogue.scr"
#define SAVEFILE  "rogue.sav"
#define ENVFILE	  "rogue.opt"
#define IBM
#define MACROSZ 41

#define ifterse0 ifterse
#define ifterse1 ifterse
#define ifterse2 ifterse
#define ifterse3 ifterse
#define ifterse4 ifterse

/*
 * Maximum number of different things
 */
#define MAXROOMS	9
#define MAXTHINGS	9
#define MAXOBJ		9
#define MAXPACK		23
#define MAXTRAPS	10
#define AMULETLEVEL	26
#define	NUMTHINGS	7	/* number of types of things */
#define MAXPASS		13	/* upper limit on number of passages */
#define MAXNAME		20  /* Maximum Length of a scroll */
#define MAXITEMS	83  /* Maximum number of randomly generated things */
#define BUFSIZE		128 /*@ moved from curses.h */

/*
 * All the fun defines
 */
#define shint		int		/* short integer (for very small #s) */
#define when		break;case
#define otherwise	break;default
#define until(expr)	while(!(expr))
#define next(ptr)	(*ptr).l_next
#define prev(ptr)	(*ptr).l_prev
#ifdef UNIX
#define winat(y,x)	(moat(y,x) != NULL ? moat(y,x)->t_disguise : chat(y,x))
#define DISTANCE(y1,x1,y2,x2) (((x2)-(x1))*((x2)-(x1))+((y2)-(y1))*((y2)-(y1)))
#endif
#ifdef UNIX
#define ce(a,b)		((a).x == (b).x && (a).y == (b).y)
#else
#define ce(a,b)		_ce(&(a),&(b))
#endif
#define hero		player.t_pos
#define pstats		player.t_stats
#define pack		player.t_pack
#define proom		player.t_room
#define max_hp		player.t_stats.s_maxhp
#define attach(a,b)	_attach(&a,b)
#define detach(a,b)	_detach(&a,b)
#define free_list(a)	_free_list(&a)
#define max(a,b)	((a) > (b) ? (a) : (b))
#define on(thing,flag)	(((thing).t_flags & (flag)) != 0)
#define GOLDCALC	(rnd(50 + 10 * level) + 2)
#define ISRING(h,r)	(cur_ring[h] != NULL && cur_ring[h]->o_which == r)
#define ISWEARING(r)	(ISRING(LEFT, r) || ISRING(RIGHT, r))
#define ISMULT(type) 	(type==POTION || type==SCROLL || type==FOOD || type==GOLD)
#define chat(y,x)	(_level[INDEX(y,x)])
#define flat(y,x)	(_flags[INDEX(y,x)])
#define unc(cp)		(cp).y, (cp).x
#define isfloor(c)	((c) == FLOOR || (c) == PASSAGE)
#define isgone(rp)	(((rp)->r_flags&ISGONE) && ((rp)->r_flags&ISMAZE) == 0)
#ifdef WIZARD
#define debug		if (wizard) msg
#endif
/*@
 * And some new fun defines...
 */
#define ismonster(ch)	(((ch) >= 'A') && ((ch) <= 'Z'))

/*
 * Various constants
 */
#define BEARTIME	spread(3)
#define SLEEPTIME	spread(5)
#define HEALTIME	spread(30)
#define HOLDTIME	spread(2)
#define WANDERTIME	spread(70)
#define HUHDURATION	spread(20)
#define SEEDURATION	spread(300)
#define HUNGERTIME	spread(1300)
#define MORETIME	150
#define STOMACHSIZE	2000
#define STARVETIME	850
#define LEFT		0
#define RIGHT		1
#define BOLT_LENGTH	6
#define LAMPDIST	3

/*
 * Save against things
 */
#define VS_POISON	00
#define VS_PARALYZATION	00
#define VS_LUCK		01
#define VS_DEATH	00
#define VS_BREATH	02
#define VS_MAGIC	03

/*
 * Various flag bits
 */
/* flags for rooms */
#define ISDARK	 0x0001		/* room is dark */
#define ISGONE	 0x0002		/* room is gone (a corridor) */
#define	ISMAZE	 0x0004		/* room is a maze */

/* flags for objects */
#define ISCURSED 0x0001		/* object is cursed */
#define ISKNOW	 0x0002		/* player knows details about the object */
#define DIDFLASH 0x0004		/* has the vorpal weapon flashed */
#define ISEGO	 0x0008		/* weapon has control of player */
#define ISMISL	 0x0010		/* object is a missile type */
#define ISMANY	 0x0020		/* object comes in groups */
#define ISREVEAL 0x0040		/* Do you know who the enemy of the object is */

/* flags for creatures */
#define ISBLIND	 0x0001		/* creature is blind */
#define SEEMONST 0x0002		/* hero can detect unseen monsters */
#define ISRUN	 0x0004		/* creature is running at the player */
#define ISFOUND	 0x0008		/* creature has been seen (used for objects) */
#define ISINVIS	 0x0010		/* creature is invisible */
#define ISMEAN	 0x0020		/* creature can wake when player enters room */
#define ISGREED	 0x0040		/* creature runs to protect gold */
#define ISHELD	 0x0080		/* creature has been held */
#define ISHUH	 0x0100		/* creature is confused */
#define ISREGEN	 0x0200		/* creature can regenerate */
#define CANHUH	 0x0400		/* creature can confuse */
#define CANSEE	 0x0800		/* creature can see invisible creatures */
#define ISCANC	 0x1000		/* creature has special qualities cancelled */
#define ISSLOW	 0x2000		/* creature has been slowed */
#define ISHASTE	 0x4000		/* creature has been hastened */
#define ISFLY	 0x8000		/* creature is of the flying type */

/*
 * Flags for level map
 */
#define F_PASS		0x040		/* is a passageway */
#define F_MAZE		0x020		/* have seen this corridor before */
#define F_REAL		0x010		/* what you see is what you get */
#define F_PNUM		0x00f		/* passage number mask */
#define F_TMASK		0x007		/* trap number mask */

/*
 * Trap types
 */
#define T_DOOR	00
#define T_ARROW	01
#define T_SLEEP	02
#define T_BEAR	03
#define T_TELEP	04
#define T_DART	05
#define NTRAPS	6

/*
 * Potion types
 */
#define P_CONFUSE	0
#define P_PARALYZE	1
#define P_POISON	2
#define P_STRENGTH	3
#define P_SEEINVIS	4
#define P_HEALING	5
#define P_MFIND		6
#define	P_TFIND 	7
#define	P_RAISE		8
#define P_XHEAL		9
#define P_HASTE		10
#define P_RESTORE	11
#define P_BLIND		12
#define P_NOP		13
#define MAXPOTIONS	14

/*
 * Scroll types
 */
#define S_CONFUSE	0
#define S_MAP		1
#define S_HOLD		2
#define S_SLEEP		3
#define S_ARMOR		4
#define S_IDENT		5
#define S_SCARE		6
#define S_GFIND		7
#define S_TELEP		8
#define S_ENCH		9
#define S_CREATE	10
#define S_REMOVE	11
#define S_AGGR		12
#define S_NOP		13
#define S_VORPAL	14
#define MAXSCROLLS	15

/*
 * Weapon types
 */
#define MACE		0
#define SWORD		1
#define BOW		2
#define ARROW		3
#define DAGGER		4
#define TWOSWORD	5
#define DART		6
#define CROSSBOW	7
#define BOLT		8
#define SPEAR		9
#define FLAME		10	/* fake entry for dragon breath (ick) */
#define MAXWEAPONS	10	/* this should equal FLAME */

/*
 * Armor types
 */
#define LEATHER		0
#define RING_MAIL	1
#define STUDDED_LEATHER	2
#define SCALE_MAIL	3
#define CHAIN_MAIL	4
#define SPLINT_MAIL	5
#define BANDED_MAIL	6
#define PLATE_MAIL	7
#define MAXARMORS	8

/*
 * Ring types
 */
#define R_PROTECT	0
#define R_ADDSTR	1
#define R_SUSTSTR	2
#define R_SEARCH	3
#define R_SEEINVIS	4
#define R_NOP		5
#define R_AGGR		6
#define R_ADDHIT	7
#define R_ADDDAM	8
#define R_REGEN		9
#define R_DIGEST	10
#define R_TELEPORT	11
#define R_STEALTH	12
#define R_SUSTARM	13
#define MAXRINGS	14

/*
 * Rod/Wand/Staff types
 */

#define WS_LIGHT	0
#define WS_HIT		1
#define WS_ELECT	2
#define WS_FIRE		3
#define WS_COLD		4
#define WS_POLYMORPH	5
#define WS_MISSILE	6
#define WS_HASTE_M	7
#define WS_SLOW_M	8
#define WS_DRAIN	9
#define WS_NOP		10
#define WS_TELAWAY	11
#define WS_TELTO	12
#define WS_CANCEL	13
#define MAXSTICKS	14

/*
 * Now we define the structures and types
 */

/*
 * Help list
 * @ this was unused in original. Now improved and put to good use
 */
struct h_list {
	byte h_chstr[6];  //@ either (ch) or (ch,sep,ch2) appended with ": "
	char *h_desc;
};

/*
 * Coordinate data type
 */
typedef struct {
	shint x;
	shint y;
} coord;

/*@
 * Data type for strength values and modifiers
 * That's very generous from Rogue devs to allow full 16-bits (uint in 1985)
 * for strength, considering normal play would not get even remotely close to 8
 */
typedef unsigned int str_t;

/*
 * Stuff about magic items
 */

struct magic_item {
	char *mi_name;
	shint mi_prob;
	short mi_worth;
};

struct array {
	char storage[MAXNAME+1];
};

/*
 * Room structure
 */
struct room {
	coord r_pos;			/* Upper left corner */
	coord r_max;			/* Size of room */
	coord r_gold;			/* Where the gold is */
	int r_goldval;			/* How much the gold is worth */
	short r_flags;			/* Info about the room */
	shint r_nexits;			/* Number of exits */
	coord r_exit[12];			/* Where the exits are */
};

/*
 * Structure describing a fighting being
 */
struct stats {
	str_t s_str;			/* Strength */
	long s_exp;				/* Experience */
	shint s_lvl;			/* Level of mastery */
	shint s_arm;			/* Armor class */
	shint s_hpt;			/* Hit points */
	char *s_dmg;			/* String describing damage done */
	shint s_maxhp;			/* Max hit points */
};

/*
 * Structure for monsters and player
 */
union thing {
	struct {
	union thing *_l_next, *_l_prev;	/* Next pointer in link */
	coord _t_pos;			/* Position */
	char _t_turn;			/* If slowed, is it a turn to move */
	char _t_type;			/* What it is */
	byte _t_disguise;		/* What mimic looks like */
	byte _t_oldch;			/* Character that was where it was */
	coord *_t_dest;			/* Where it is running to */
	short _t_flags;			/* State word */
	struct stats _t_stats;		/* Physical description */
	struct room *_t_room;		/* Current room for thing */
	union thing *_t_pack;		/* What the thing is carrying */
	} _t;
	struct {
	union thing *_l_next, *_l_prev;	/* Next pointer in link */
	shint _o_type;			/* What kind of object it is */
	coord _o_pos;			/* Where it lives on the screen */
	char *_o_text;			/* What it says if you read it */
	char _o_launch;			/* What you need to launch it */
	char *_o_damage;		/* Damage if used like sword */
	char *_o_hurldmg;		/* Damage if thrown */
	shint _o_count;			/* Count for plural objects */
	shint _o_which;			/* Which object of a type it is */
	shint _o_hplus;			/* Plusses to hit */
	shint _o_dplus;			/* Plusses to damage */
	short _o_ac;			/* Armor class */
	short _o_flags;			/* Information about objects */
	char _o_enemy;			/* If it is enchanted, who it hates */
	shint _o_group;			/* Group number for this object */
	} _o;
};

typedef union thing THING;

#define l_next		_t._l_next
#define l_prev		_t._l_prev
#define t_pos		_t._t_pos
#define t_turn		_t._t_turn
#define t_type		_t._t_type
#define t_disguise	_t._t_disguise
#define t_oldch		_t._t_oldch
#define t_dest		_t._t_dest
#define t_flags		_t._t_flags
#define t_stats		_t._t_stats
#define t_pack		_t._t_pack
#define t_room		_t._t_room
#define o_type		_o._o_type
#define o_pos		_o._o_pos
#define o_text		_o._o_text
#define o_launch	_o._o_launch
#define o_damage	_o._o_damage
#define o_hurldmg	_o._o_hurldmg
#define o_count		_o._o_count
#define o_which		_o._o_which
#define o_hplus		_o._o_hplus
#define o_dplus		_o._o_dplus
#define o_ac		_o._o_ac
#define o_charges	o_ac
#define o_goldval	o_ac
#define o_flags		_o._o_flags
#define o_group		_o._o_group
#define o_enemy		_o._o_enemy

/*
 * Array containing information on all the various types of monsters
 */
struct monster {
	char *m_name;			/* What to call the monster */
	shint m_carry;			/* Probability of carrying something */
	unsigned short m_flags;			/* Things about the monster */
	struct stats m_stats;		/* Initial stats */
};

#define TOPSCORES	10
struct sc_ent {
	char sc_name[38];
	int sc_rank;
	int sc_gold;
	int sc_fate;
	int sc_level;
};

/*
 * External variables
 * @ all in extern.c unless noted (init.c, env.c, croot.c, main.c, protect.c)
 */
extern int maxitems;
extern int maxrow;
extern int reinit;
extern int revno, verno;
extern int is_me;
extern int iguess;
extern bool bailout;

//@ nullstr should probably be used in misc and wizard instead of (size_t)NULL
extern char nullstr[], *it, *you, *no_mem;

extern char *s_guess[], *p_guess[], *r_guess[], *ws_guess[];
extern char f_damage[];

extern bool amulet, after, again, door_stop, expert, fastmode, faststate,
			firstmove, noscore, playing, running, save_msg, saw_amulet, terse,
			was_trapped;
#ifdef WIZARD
bool wizard;
#endif

extern bool p_know[], r_know[], s_know[], ws_know[];

extern char *a_names[], *flashmsg, *he_man[], huh[],
		*intense, *p_colors[], *r_stones[], runch, *typebuf, take,
		*w_names[], *ws_made[], *ws_type[];

extern struct h_list helpcoms[], helpobjs[];

extern int	a_chances[], a_class[], count, dnum, food_left,
		fung_hit, group, hungry_state, inpack,
		level, max_level, mpos, no_command, no_food, no_move,
		ntraps, purse, quiet, total;

extern long seed;

//@ related to copy protection
extern int hit_mul;
extern char *your_na, *kild_by;
extern int goodchk;
extern char *_whoami;  //@ defined (no value set) but seems unused
extern int cksum;

extern THING *cur_armor, *cur_ring[], *cur_weapon,
		*lvl_obj, *mlist, player;

extern coord	delta, oldpos;

extern struct room	*oldrp, passages[], rooms[];

extern struct stats	max_stats;

extern struct monster	monsters[];

extern struct magic_item	p_magic[], r_magic[], s_magic[],
				things[], ws_magic[];

extern struct array s_names[], _guesses[];

#ifdef LOG
extern int captains_log;
#endif //LOG

/*@
 * Definition commented out:
 * extern bool askme, fight_flush, jump, passgo, slow_invent;
 * extern char *release;
 *
 * Not found:
 * extern bool in_shell;
 * extern char file_name[], home[], outbuf[];
 * extern int lastscore;
 */


//@ env.c
extern char s_menu[], s_fruit[], s_score[], s_save[], s_macro[];
extern char s_drive[], s_screen[];
extern char fruit[], macro[], whoami[];
//@ extern char s_name[];  //@ not found. Perhaps old name for whoami[]?


//@ init.c
extern char *tbuf, *prbuf;
extern byte *_level, *_flags;
extern long *e_levels;
extern char *msgbuf;
extern THING *_things;
extern int   *_t_alloc;
extern char *ring_buf;
//@ extern char *_top, *_base;  //@ not found
/*@
 * Deprecated:
 * extern char *end_mem;
 */


//@ main.c
extern char do_force;  //@ used in save.c
extern int bwflag;     //@ used in save.c


//@ protect.c
extern int no_step;  //@ used in clock(), originally by dos.asm


/*
 * Function types
 *
 * @ curses.c has its own header
 * @ mach_dep.c functions are declared in extern.h
 */

//@ armor.c
void	wear(), take_off(), waste_time();

//@ chase.c
void	runners(), do_chase(), chase(), start_run();
bool	see_monst(), diag_ok(), cansee();
struct room	*roomin();
coord	*find_dest();

//@ command.c
void	command(), show_count(), execcom();

//@ daemon.c
void	start_daemon(), do_daemons(), fuse(), lengthen(), extinguish(),
		do_fuses();

//@ daemons.c
void	doctor(), swander(), rollwand(), unconfuse(), unsee(), sight(),
		nohaste(), stomach();

//@ env.h
byte	peekc();
bool	setenv_from_file();
void	putenv_struct();

//@ fakedos.c
void	fakedos();
int	dodos();

//@ fight.c
bool	fight(), swing(), roll_em(), save_throw(), save(), is_magic();
void	attack(), check_level(), hit(), miss(), raise_level(), thunk(),
		remove_monster(), killed();
int	str_plus(), add_dam();

//@ init.c
void	init_player(), init_things(), init_colors(), init_names(),
		init_stones(), init_materials(), init_ds(), free_ds();
char	*getsyl(), rchr();

//@ io.c
void	ifterse(const char *tfmt, const char *fmt, ...);
void	msg(const char *fmt, ...);
void	vmsg(const char *fmt, va_list argp);
void	addmsg(const char *fmt, ...);
void	doadd(const char *fmt, va_list argp);
void	wait_msg(const char *msg);
void	endmsg(), more(), putmsg(), scrlmsg(), status(), wait_for(),
		show_win(), str_attr(), SIG2();
char	*io_unctrl(), *noterse();

//@ list.c
THING	*new_item();
void	*talloc(), _detach(), _attach(), _free_list();
int	discard();

//@ load.c
void	epyx_yuck(), scr_load(), bload();
int	find_drive();

//@ main.c
void	endit(), playit(), quit(), leave();
int	rnd(), roll();

//@ maze.c
void	draw_maze(), new_frontier(), add_frnt(), con_frnt(), splat();
bool	maze_at(), inrange();

//@ misc.c
void	look(), eat(), chg_str(), add_str(), aggravate(), call_it(),
		help(), search(), d_level(), u_level(), call(), do_macro();
THING	*find_obj();
bool	add_haste(), is_current(), get_dir(), find_dir(), step_ok(),
		_ce(), offmap();
char	*tr_name(), *vowelstr(), goodch();
shint	sign();
byte	winat();
int	spread(), DISTANCE(), INDEX();
#ifdef ME
bool	me();
#endif
#ifdef TEST
bool	istest();
#endif

//@ monsters.c
char	randmonster(), pick_mons();
void	new_monster(), f_restor(), wanderer(), give_pack();
THING	*wake_monster(), *moat();
int	exp_add();

//@ move.c
void	do_run(), do_move(), door_open(), descend(), rndmove();
byte	be_trapped();

//@ new_leve.c
void	new_level(), put_things(), treas_room();
int	rnd_room();

//@ pack.c
THING	*pack_obj(), *get_item();
void	add_pack(), pick_up(), money();
byte	inventory(), pack_char();

//@ passages.c
void	conn(), do_passages(), door(), passnum(), numpass(), psplat();

//@ potions.c
void	quaff(), invis_on(), th_effect();
bool	turn_see();

//@ protect.c
void	protect();

//@ rings.c
void	ring_on(), ring_off();
char	*ring_num();
int	gethand(), ring_eat();

//@ rip.c
void	score(), get_scores(), put_scores(), pr_scores(), death(),
		total_winner();
char *killname();
int	add_scores();
#ifdef DEMO
void	demo();
#endif //DEMO

//@ rooms.c
void	do_rooms(), draw_room(), vert(), horiz(), rnd_pos(), enter_room(),
		leave_room();

//@ save.c
void	save_game(), restore();
int	save_ds();

//@ scrolls.c
void read_scroll();

//@ slime.c
void	slime_split();
bool	new_slime(), plop_monster();

//@ sticks.c
void	fix_stick(), do_zap(), drain(), fire_bolt();
char	*charge_str();

//@ strings.c
bool	is_alpha(char ch);
bool	is_upper(char ch);
bool	is_lower(char ch);
bool	is_digit(char ch);
bool	is_space(char ch);
bool	is_print(char ch);
char	*stccpy(), *stpblk(), *endblk();
void	lcase();

//@ things.c
char	*inv_name(), *nothing();
void	chopmsg(char *s, char *shmsg, char *lnmsg, ...);
void	drop(), discovered(), print_disc(), set_order();
bool	can_drop();
THING	*new_thing();
byte	add_line(), end_line();
int	pick_one();

//@ weapons.c
void	missile(), do_motion(), fall(), init_weapon(), wield(),
		tick_pause();
char	*short_name(), *num();
bool	hit_monster();
int	fallpos();

//@ wizard.c
void	whatis();
int	teleport();
#ifdef WIZARD
void	create_obj(), show_map();
#ifdef UNIX
bool	passwd();
#endif //UNIX
#endif //WIZARD


/*@ functions declared but not found
int	auto_save(), tstp();
THING	*find_mons();
char	*balloc();
*/
