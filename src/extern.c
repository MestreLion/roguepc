/*
 * global variable initializaton
 *
 * @(#)extern.c	5.2 (Berkeley) 6/16/82
 */

#include "rogue.h"
#include "curses.h"

#ifdef LOG
int captains_log = FALSE;
#endif //LOG
#ifdef SDEBUG
int is_com;
#endif //SDEBUG
/*
 * revno: current revision level
 * verno: current version of a particular rev
 */
int revno = REV;
int verno = VER;

/*
 * All this should be low as possible in memory so that
 * we can save the min
 */
char *w_names[MAXWEAPONS + 1] = {	/* Names of the various weapons */
	"mace",
	"long sword",
	"short bow",
	"arrow",
	"dagger",
	"two handed sword",
	"dart",
	"crossbow",
	"crossbow bolt",
	"spear",
	NULL				/* fake entry for dragon's breath */
};
char *a_names[MAXARMORS] = {		/* Names of armor types */
	"leather armor",
	"ring mail",
	"studded leather armor",
	"scale mail",
	"chain mail",
	"splint mail",
	"banded mail",
	"plate mail"
};

int a_chances[MAXARMORS] = {		/* Chance for each armor type */
	20,
	35,
	50,
	63,
	75,
	85,
	95,
	100
};
int a_class[MAXARMORS] = {		/* Armor class for each armor type */
	8,
	7,
	7,
	6,
	5,
	4,
	4,
	3
};

struct magic_item s_magic[MAXSCROLLS] = {
	{ "monster confusion",	 8, 140 },
	{ "magic mapping",		 5, 150 },
	{ "hold monster",		 3, 180 },
	{ "sleep",			 5,   5 },
	{ "enchant armor",		 8, 160 },
	{ "identify",		27, 100 },
	{ "scare monster",		 4, 200 },
	{ "food detection",		 4,  50 },
	{ "teleportation",		 7, 165 },
	{ "enchant weapon",		10, 150 },
	{ "create monster",		 5,  75 },
	{ "remove curse",		 8, 105 },
	{ "aggravate monsters",	 4,  20 },
	{ "blank paper",		 1,   5 },
	{ "vorpalize weapon",	 1, 300 }
};

struct magic_item p_magic[MAXPOTIONS] = {
	{ "confusion",		 8,   5 },
	{ "paralysis",		10,   5 },
	{ "poison",			 8,   5 },
	{ "gain strength",		15, 150 },
	{ "see invisible",		 2, 100 },
	{ "healing",		15, 130 },
#ifdef DEMO
	{ "advertisement",           6, 130 },
#else
	{ "monster detection",	 6, 130 },
#endif //DEMO
	{ "magic detection",	 6, 105 },
	{ "raise level",		 2, 250 },
	{ "extra healing",		 5, 200 },
	{ "haste self",		 4, 190 },
	{ "restore strength",	14, 130 },
	{ "blindness",		 4,   5 },
	{ "thirst quenching",	 1,   5 }
};

struct magic_item r_magic[MAXRINGS] = {
	{ "protection",		 9, 400 },
	{ "add strength",		 9, 400 },
	{ "sustain strength",	 5, 280 },
	{ "searching",		10, 420 },
	{ "see invisible",		10, 310 },
	{ "adornment",		 1,  10 },
	{ "aggravate monster",	10,  10 },
	{ "dexterity",		 8, 440 },
	{ "increase damage",	 8, 400 },
	{ "regeneration",		 4, 460 },
	{ "slow digestion",		 9, 240 },
	{ "teleportation",		 5,  30 },
	{ "stealth",		 7, 470 },
	{ "maintain armor",		 5, 380 }
};

struct magic_item ws_magic[MAXSTICKS] = {
	{ "light",			12, 250 },
	{ "striking",		 9,  75 },
	{ "lightning",		 3, 330 },
	{ "fire",			 3, 330 },
	{ "cold",			 3, 330 },
	{ "polymorph",		15, 310 },
	{ "magic missile",		10, 170 },
	{ "haste monster",		 9,   5 },
	{ "slow monster",		11, 350 },
	{ "drain life",		 9, 300 },
	{ "nothing",		 1,   5 },
	{ "teleport away",		 5, 340 },
	{ "teleport to",		 5,  50 },
	{ "cancellation",		 5, 280 }
};

#ifdef HELP
/*@
 * Original code used CP437 codes hard coded inside the help strings,
 * instead of the #define'd char constants for FLOOR, PLAYER etc.
 * To support the constants, H_*() macros were created and helpcoms/helpobjs
 * array type has changed from string to struct h_list.
 *
 * Ironically, struct h_list already existed in rogue.h, but it was unused in
 * code, so perhaps original authors either abandoned the idea or were halfway
 * through implementing it.
 */
#define H_STR(str)	{"", str}
#define H_CHSTR(ch, str)	{{ch, ':', ' ', '\0'}, str}
#define H_CH2STR(ch1, ch2, sep, str)	{{ch1, sep, ch2, ':', ' ', '\0'}, str}
#define H_END	{"", ""}
struct h_list helpcoms[] = {
	H_STR("F1     list of commands"),
	H_STR("F2     list of symbols"),
	H_STR("F3     repeat command"),
	H_STR("F4     repeat message"),
	H_STR("F5     rename something"),
	H_STR("F6     recall what's been discovered"),
	H_STR("F7     inventory of your possessions"),
	H_STR("F8     <dir> identify trap type"),
	H_STR("F9     The Any Key (definable)"),
	H_STR("Alt F9 defines the Any Key"),
	H_STR("F10    Supervisor Key (fake dos)"),
	H_STR("Space  Clear -More- message"),
	H_STR("\x11\xd9     the Enter Key"),
	H_STR("\x1b      left"),
	H_STR("\x19      down"),
	H_STR("\x18      up"),
	H_STR("\x1a      right"),
	H_STR("Home   up & left"),
	H_STR("PgUp   up & right"),
	H_STR("End    down & left"),
	H_STR("PgDn   down & right"),
	H_STR("Scroll Fast Play mode"),
	H_STR(".      rest"),
	H_STR(">      go down a staircase"),
	H_STR("<      go up a staircase"),
	H_STR("Esc    cancel command"),
	H_STR("d      drop object"),
	H_STR("e      eat food"),
	H_STR("f      <dir> find something"),
	H_STR("q      quaff potion"),
	H_STR("r      read paper"),
	H_STR("s      search for trap/secret door"),
	H_STR("t      <dir> throw something"),
	H_STR("w      wield a weapon"),
	H_STR("z      <dir> zap with a wand"),
	H_STR("B      run down & left"),
	H_STR("H      run left"),
	H_STR("J      run down"),
	H_STR("K      run up"),
	H_STR("L      run right"),
	H_STR("N      run down & right"),
	H_STR("U      run up & right"),
	H_STR("Y      run up & left"),
	H_STR("W      wear armor"),
	H_STR("T      take armor off"),
	H_STR("P      put on ring"),
	H_STR("Q      quit"),
	H_STR("R      remove ring"),
	H_STR("S      save game"),
	H_STR("^      identify trap"),
	H_STR("?      help"),
	H_STR("/      key"),
	H_STR("+      throw"),
	H_STR("-      zap"),
	H_STR("Ctrl t terse message format"),
	H_STR("Ctrl r repeat message"),
	H_STR("Del    search for something hidden"),
	H_STR("Ins    <dir> find something"),
	H_STR("a      repeat command"),
	H_STR("c      rename something"),
	H_STR("i      inventory"),
	H_STR("v      version number"),
	H_STR("!      Supervisor Key (fake DOS)"),
	H_STR("D      list what has been discovered"),
	H_END
};

struct h_list helpobjs[] = {
	H_CHSTR(FLOOR,   "the floor"),
	H_CHSTR(PLAYER,  "the hero"),
	H_CHSTR(FOOD,    "some food"),
	H_CHSTR(AMULET,  "the amulet of yendor"),
	H_CHSTR(SCROLL,  "a scroll"),
	H_CHSTR(WEAPON,  "a weapon"),
	H_CHSTR(ARMOR,   "a piece of armor"),
	H_CHSTR(GOLD,    "some gold"),
	H_CHSTR(STICK,   "a magic staff"),
	H_CHSTR(POTION,  "a potion"),
	H_CHSTR(RING,    "a magic ring"),
	H_CHSTR(0xB2,    "a passage"),  //@ surprisingly it's not PASSAGE
	/* make sure in 40 or 80 column none of line draw set connects */
	/* this is currently in column 1 for 80 */
	H_CHSTR(DOOR,    "a door"),
	H_CHSTR(ULWALL,  "an upper left corner"),
	H_CHSTR(TRAP,    "a trap"),
	H_CHSTR(HWALL,   "a horizontal wall"),
	H_CHSTR(LRWALL,  "a lower right corner"),
	H_CHSTR(LLWALL,  "a lower left corner"),
	H_CHSTR(VWALL,   "a vertical wall"),
	H_CHSTR(URWALL,  "an upper right corner"),
	H_CHSTR(STAIRS,  "a stair case"),
	H_CH2STR(MAGIC, BMAGIC, ',', "safe and perilous magic"),
	H_CH2STR('A',   'Z',    '-', "26 different monsters"),
	H_END
};
#endif //HELP
/*
 * Names of the various experience levels
 */

char *he_man[] = {
	"",
	"Guild Novice",
	"Apprentice",
	"Journeyman",
	"Adventurer",
	"Fighter",
	"Warrior",
	"Rogue",
	"Champion",
	"Master Rogue",
	"Warlord",
	"Hero",
	"Guild Master",
	"Dragonlord",
	"Wizard",
	"Rogue Geek",
	"Rogue Addict",
	"Schmendrick",
	"Gunfighter",
	"Time Waster",
	"Bug Chaser"
};

/*
 * Lattice C compiler funnies
 */
int maxitems = 0;
int reinit = FALSE;

bool after;				/* True if we want after daemons */
bool noscore;				/* Was a wizard sometime */
bool again;			/* The last command is repeated */
bool s_know[MAXSCROLLS];		/* Does he know what a scroll does */
bool p_know[MAXPOTIONS];		/* Does he know what a potion does */
bool r_know[MAXRINGS];			/* Does he know what a ring does */
bool ws_know[MAXSTICKS];		/* Does he know what a stick does */
bool amulet = FALSE;			/* He has the amulet */
bool saw_amulet = FALSE;	    /* He has seen the amulet */
/* bool askme = TRUE; */			/* Ask about unidentified things */
bool door_stop = FALSE;			/* Stop running when we pass a door */
bool fastmode = FALSE;			/* Run until you see something */
bool faststate = FALSE;			/* Toggle for find (see above) */
/* bool fight_flush = TRUE;	*/	/* True if toilet input */
bool firstmove = FALSE;			/* First move after setting door_stop */
/* bool jump = FALSE;	*/		/* Show running as series of jumps */
/* bool passgo = TRUE;	*/		/* Follow passages */
bool playing = TRUE;			/* True until he quits */
bool running = FALSE;			/* True if player is running */
bool save_msg = TRUE;			/* Remember last msg */
/* bool slow_invent = FALSE; */		/* Inventory one line at a time */
bool terse = FALSE;
bool expert = FALSE;
#ifdef ME
int is_me;
#endif
/*@
 * `was_trapped` was originally a bool, which in original code was typedef'd as
 * unsigned char. As it is used in ++ increment and > test, I've reverted it
 * to its original (real) type. See be_trapped() in move.c and look() in misc.c
 */
unsigned char was_trapped = FALSE;		/* Was a trap sprung */
#ifdef WIZARD
bool wizard = FALSE;			/* True if allows wizard commands */
#endif
bool bailout = FALSE;
char take;				/* Thing the rogue is taking */
char runch;				/* Direction player is running */
/* now names are associated with fixed pointers */
struct array s_names[MAXSCROLLS];			/* Names of the scrolls */
char *p_colors[MAXPOTIONS];		/* Colors of the potions */
char *r_stones[MAXRINGS];		/* Stone settings of the rings */
char *ws_made[MAXSTICKS];		/* What sticks are made of */
/* char *release;	*/			/* Release number of rogue */
char huh[BUFSIZE];				/* The last message printed */
char *s_guess[MAXSCROLLS];		/* Players guess at what scroll is */
char *p_guess[MAXPOTIONS];		/* Players guess at what potion is */
char *r_guess[MAXRINGS];		/* Players guess at what ring is */
char *ws_guess[MAXSTICKS];		/* Players guess at what wand is */
/* storage array for guesses */
struct array _guesses[MAXSCROLLS+MAXPOTIONS+MAXRINGS+MAXSTICKS];
int iguess = 0;
char *ws_type[MAXSTICKS];		/* Is it a wand or a staff */

int maxrow;			/* Last Line used for map  */
int max_level;				/* Deepest player has gone */
int ntraps;				/* Number of traps on this level */
int dnum;				/* Dungeon number */
int level = 1;				/* What level rogue is on */
int purse = 0;				/* How much gold the rogue has */
int mpos = 0;				/* Where cursor is on top line */
int no_move = 0;			/* Number of turns held in place */
int no_command = 0;			/* Number of turns asleep */
int inpack = 0;				/* Number of things in pack */
int total = 0;				/* Total dynamic memory bytes */
int no_food = 0;			/* Number of levels without food */
int count = 0;				/* Number of times to repeat command */
int fung_hit = 0;			/* Number of time fungi has hit */
int quiet = 0;				/* Number of quiet turns */
int food_left;				/* Amount of food in hero's stomach */
int group = 2;				/* Current group number */
int hungry_state = 0;			/* How hungry is he */
int cksum = CSUM;
long seed;				/* Random number seed */

int hit_mul = P_DAMAGE;
int goodchk = 1;
char *your_na = "Software Pirate";
char *kild_by = "Copy Protection Mafia";
char *_whoami;

/* WINDOW *hw;				 Used as a scratch window */

coord oldpos;				/* Position before last look() call */
coord delta;				/* Change indicated to get_dir() */

THING *cur_armor;			/* What a well dresssed rogue wears */
THING *cur_ring[2];			/* Which rings are being worn */
THING *cur_weapon;			/* Which weapon he is weilding */

struct room *oldrp;			/* Roomin(&oldpos) */
struct room rooms[MAXROOMS];		/* One for each room -- A level */

#define XX  {0, 0}
#define ___ {XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX} //@ 12 exits
struct room passages[MAXPASS] =		/* One for each passage */
{
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ },
	{ {0, 0}, {0, 0}, {0, 0}, 0, ISGONE|ISDARK, 0, ___ }
};
#undef ___
#undef XX


#define INIT_STATS { 16, 0, 1, 10, 12, "1d4", 12 }

struct stats max_stats = INIT_STATS;	/* The maximum for the player */

THING player;				/* The rogue */
THING *lvl_obj = NULL;			/* List of objects on this level */
THING *mlist = NULL;			/* List of monsters on the level */

/*@
 * Original code did not define a value for s_maxhp member of stats struct.
 * s_maxhp from this monster template is unused, just like s_hpt, as its value
 * was randomly chosen for each new generated monster. To make compilers happy,
 * value is now set to a dummy ___ value, the same convention used in original
 * code for s_hpt.
 */
#define ___ 1
#define XX 10
struct monster monsters[26] =
{
	/* Name		 CARRY	FLAG    str, exp, lvl, amr, hpt, dmg, maxhp */
	{ "aquator",	0,	ISMEAN,	{ XX, 20,   5,   2, ___, "0d0/0d0", ___ } },
	{ "bat",	 	0,	ISFLY,	{ XX,  1,   1,   3, ___, "1d2", ___ } },
	{ "centaur",	 15,	0,	{ XX, 25,   4,   4, ___, "1d6/1d6", ___ } },
	{ "dragon",	 100,	ISMEAN,	{ XX,6800, 10,  -1, ___, "1d8/1d8/3d10", ___ } },
	{ "emu",	 0,	ISMEAN,	{ XX,  2,   1,   7, ___, "1d2", ___ } },
		/* NOTE: the damage is %%% so that xstr won't merge this */
		/* string with others, since it is written on in the program */
	{ "venus flytrap",0,	ISMEAN,	{ XX, 80,   8,   3, ___, "%%%d0", ___ } },
	{ "griffin",	 20,	ISMEAN|ISFLY|ISREGEN,	{XX,2000, 13, 2,___, "4d3/3d5/4d3", ___ } },
	{ "hobgoblin",	 0,	ISMEAN,	{ XX,  3,   1,   5, ___, "1d8", ___ } },
	{ "ice monster", 0,	ISMEAN,	{ XX,  15,   1,   9, ___, "1d2", ___ } },
	{ "jabberwock",  70,	0,	{ XX,4000, 15,   6, ___, "2d12/2d4", ___ } },
	{ "kestral",	 0,	ISMEAN|ISFLY, { XX,  1,   1,   7, ___, "1d4", ___ } },
	{ "leprechaun",	 0,	ISGREED,	{ XX, 10,   3,   8, ___, "1d2", ___ } },
	{ "medusa",	 40,	ISMEAN,	{ XX,200,   8,   2, ___, "3d4/3d4/2d5", ___ } },
	{ "nymph",	 100,	0,	{ XX, 37,   3,   9, ___, "0d0", ___ } },
	{ "orc",	 15,	ISGREED,{ XX,  5,   1,   6, ___, "1d8", ___ } },
	{ "phantom",	 0,	ISINVIS,{ XX,120,   8,   3, ___, "4d4", ___ } },
	{ "quagga",	 30,	ISMEAN,	{ XX, 32,   3,   2, ___, "1d2/1d2/1d4", ___ } },
	{ "rattlesnake", 0,	ISMEAN,	{ XX,  9,   2,   3, ___, "1d6", ___ } },
	{ "slime",	 	 0,	ISMEAN,	{ XX,  1,   2,   8, ___, "1d3", ___ } },
	{ "troll",	 50,	ISREGEN|ISMEAN,{ XX, 120, 6, 4, ___, "1d8/1d8/2d6", ___ } },
	{ "ur-vile",	 0,	ISMEAN,	{ XX,190,   7,  -2, ___, "1d3/1d3/1d3/4d6", ___ } },
	{ "vampire",	 20,	ISREGEN|ISMEAN,{ XX,350,   8,   1, ___, "1d10", ___ } },
	{ "wraith",	 0,	0,	{ XX, 55,   5,   4, ___, "1d6", ___ } },
	{ "xeroc",	30,	0,	{ XX,100,   7,   7, ___, "3d4", ___ } },
	{ "yeti",	 30,	0,	{ XX, 50,   4,   6, ___, "1d6/1d6", ___ } },
	{ "zombie",	 0,	ISMEAN,	{ XX,  6,   2,   8, ___, "1d8", ___ } }
};
char f_damage[10];
#undef ___
#undef XX

/*@
 * Not to be confused with _things[], which is an array of THINGS on the level
 * This one serves to choose the type of random items. The actual probability
 * is redefined in init_things(), and the only user is new_thing().
 * To make compilers happy, the unused mi_worth is set using ___, as per
 * original code convention.
 */
#define ___ 1
struct magic_item things[NUMTHINGS] = {
	{ 0,			27, ___ },	/* potion */
	{ 0,			30, ___ },	/* scroll */
	{ 0,			17, ___ },	/* food */
	{ 0,			 8, ___ },	/* weapon */
	{ 0,			 8, ___ },	/* armor */
	{ 0,			 5, ___ },	/* ring */
	{ 0,			 5, ___ }	/* stick */
};
#undef ___

/*
 * Common strings
 */
char nullstr[] = "";
char *typebuf = nullstr;

char *intense = " of intense white light";
char *flashmsg = "your %s gives off a flash%s";
char *it = "it";
char *you = "you";
char *no_mem = "Not enough Memory";
//@ char *smsg = "\r\n*** Stack Overflow ***\r\n$"; //only used in csav.asm
