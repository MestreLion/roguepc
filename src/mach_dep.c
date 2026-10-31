/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"
#include	"curses.h"

#ifndef ROGUE_NO_X11
#include <X11/Xlib.h>

/*@
 * Pointer to X display, if running under X.  Used to query keyboard LED status.
 */
Display *xdisplay = NULL;
#endif

/*@
 * These were created for fakedos() to replace DOS INT 19h and 0Eh calls, and
 * are independent from env file s_drive[], just like the original. Created as
 * externs to allow future integration with env file and run-time selection.
 */
int current_drive = ROGUE_CURRENT_DRIVE;  //@ current fake drive (A=0, B=1, ...)
int last_drive = ROGUE_LAST_DRIVE;  //@ last available drive


byte swap_bits(
	byte data,
	unsigned i,      // positions of bit sequences to swap
	unsigned j,
	unsigned length  // number of consecutive bits in each sequence
)
{
	byte x = ((data >> i) ^ (data >> j)) & ((1U << length) - 1);
	return data ^ ((x << i) | (x << j));
}


int md_keyboard_leds(void)
{
	int state = 0;
#ifndef ROGUE_NO_X11
	XKeyboardState kbstate;

	if (xdisplay)
	{
		//@ terminal emulator under X, such as xterm / gnome-terminal
		XGetKeyboardControl(xdisplay, &kbstate);
		state = swap_bits(kbstate.led_mask, 0, 2, 1);
	}
	else
#endif
#ifdef __linux__
	{
		//@ TTY such as getty / linux console
		int fd;
		if ((fd = open("/dev/tty", O_RDONLY | O_NOCTTY)) == -1 ||
				ioctl(fd, KDGKBLED, &state) == -1)
		{
			state = 0;
		}
		close(fd);
	}
#endif
	return state << 4;
}


/*@
 * Checksum of the game executable
 *
 * Originally in dos.asm

 * Return a dummy value matching the expected CSUM value defined in rogue.h
 * to avoid triggering self-integrity checks.
 *
 * The probable workflow was this:
 *
 * - After executable was compiled it was test run using "The Grand Beeking" as
 *   the player name.
 *
 * - The "v" command (Version), when used with that player name, also prints
 *   the checksum computed by this function. See command()
 *
 * - The developer changed the #define CSUM value to match the one printed.
 *
 * - Code was compiled again, and only extern.c required rebuilding. It's not
 *   clear why the new value does not affect the computed checksum. Maybe it
 *   was based only on code segment.
 *
 * - On every new level except the first, checksum was computed again and
 *   checked against CSUM. If they didn't match, the PC was immediately halted.
 *   See new_level() and _halt()
 *
 * - This effectively prevents game from being played past level 1 with a
 *   tampered (most likely cracked) executable. Being checked on every new
 *   level also inhibits the use of debuggers to crack the game on-the-fly.
 *
 * - This check only happened if PROTECTED was #define'd, which also triggered
 *   several other copy protection and anti-tampering measures. See clock()
 *
 * To simulate original behavior in case of a tampered executable without
 * changing the source code, just compile with a different CSUM #defined
 */
int
csum()
{
	return -1632;
}


/*@
 * Immediately halt execution and hang the computer
 *
 * Originally in dos.asm
 *
 * Asm version triggered the nasty combination of CLI and HLT, effectively
 * hanging the PC. Now it uses a harmless pause()
 */
void
_halt()
{
	cur_endwin();
	printf("HALT!\n");
	pause();
}


/*
 * setup:
 *	Get starting setup for all games
 */
void
setup()
{
	terse = FALSE;
	maxrow = 23;
	if (COLS == 40) {
		maxrow = 22;
		terse = TRUE;
	}
	expert = terse;
#ifndef ROGUE_NO_X11
	if (getenv("DISPLAY"))
		xdisplay = XOpenDisplay(NULL);
#endif
}




/*@
 * Increment the global tick
 *
 * This was supposed to be called 18.2 times per second, to maintain the tick
 * rate found in DOS system timer expected by Rogue. The game originally relied
 * on clock() being periodically (and automatically) called via some triggering
 * mechanism such as an IRQ timer or signal, as made by clock_on(). If tick was
 * not incremented some Bad Things would happen: Rogue could _halt() on first
 * one_tick() call, or enter infinite loop on tick_pause().
 *
 * Originally in dos.asm, renamed from clock() to avoid conflict in <time.h>
 *
 * It also performed some anti-debugger checks and copy protection measures.
 * The copy-protection is fully reproduced to the extent of my knowledge.
 * The anti-debugger tests, if failed, lead to _halt(), and are only partially
 * reproduced here. See protect.c for details.
 *
 * With md_time(), tick is no longer used and this function now only serves to
 * unlock the copy protection on one_tick().
 */
void
md_clock()
{

	//@ anti debugging: halt after 20 ticks if no_step is set
	if (no_step && ++no_step > 20)
		_halt();

	/*@
	 * Unlock copy protection if floppy check succeeded: set tombstone strings
	 * (name, killed by) to actual player name and death reason, and restore
	 * hit multiplier. Only a single tick is required to unlock.
	 * See death()
	 */
	if (hit_mul != 1 && goodchk == 0xD0D)
	{
		kild_by = prbuf;
		your_na = whoami;
		hit_mul = 1;
	}
}

/*@
 * Return Epoch time as an integer, with second resolution
 * Simple wrapper to <time.h> time()
 */
long
md_time(void)
{
	return (long)time(NULL);
}


/*@
 * Return current local time as a pointer to a struct
 */
TM *
md_localtime()
{
	static TM md_local;
	time_t secs = time(NULL);
	struct tm *local = localtime(&secs);
	md_local.second = local->tm_sec;
	md_local.minute = local->tm_min;
	md_local.hour   = local->tm_hour;
	md_local.day    = local->tm_mday;
	md_local.month  = local->tm_mon;
	md_local.year   = local->tm_year + 1900;
	return &md_local;
}


/*@
 * Sleep for nanoseconds
 */
void
md_nanosleep(long nanoseconds)
{
	struct timespec ts = {0, nanoseconds};
	nanosleep(&ts, NULL);
}


/*@
 * Renamed from srand() to avoid collision with <stdlib.h>
 * Signature and usage completely different from srand()
 *
 * Call DOS INT 21h service 2C (Get Time) and return the sum of return
 * registers CX and DX, a combination of HH:MM:SS.ss with hundredths of a
 * second resolution as an integer.
 *
 * The portable version uses time() and return the seconds since epoch as an
 * integer. Note that not only numbers have a completely different meaning from
 * the DOS version, but also time() has only second resolution, and INT 21h/2C
 * has a 24-hour cycle.
 *
 * However, for an RNG seed both are suitable.
 */
/*
 * returns a seed for a random number generator
 */
int
md_srand()
{
#ifdef DEBUG
	return ++dnum;
#else
	/*
	 * Get Time
	 */
	return (int)md_time();
#endif  // DEMO
}


/*
 * flush_type:
 *	Flush typebuf for traps, etc.
 */
void
flush_type()
{
	typebuf = "";
}

/*@
 * I wonder why this is here instead of main.c (or *anywhere* else)
 * Granted, the staff and companies to credit vary by platform, but still...
 */
void
credits()
{
	#define ULINE() if(is_color) lmagenta();else uline();

	char tname[25];

	cursor(FALSE);
	clear();
	if (is_color)
		brown();
	box(0,0,LINES-1,COLS-1);
	bold();
	center(2,"ROGUE:  The Adventure Game");
	ULINE();
	center(4,"The game of Rogue was designed by:");
	high();
	center(6,"Michael Toy and Glenn Wichman");
	ULINE();
	center(9,"Various implementations by:");
	high();
	center(11,"Ken Arnold, Jon Lane and Michael Toy");
	ULINE();
#ifdef INTL
	center(14,"International Versions by:");
#else
	center(14,"Adapted for the IBM PC by:");
#endif
	high();
#ifdef INTL
	center(16,"Mel Sibony");
#else
	center(16,"A.I. Design");
#endif
	ULINE();
	if (is_color)
		yellow();
	center(19,"(C)Copyright 1985");
	high();
#ifdef INTL
	center(20,"AI Design");
#else
	center(20,"Epyx Incorporated");
#endif
	standend();
	if (is_color)
		yellow();
	center(21,"All Rights Reserved");
	if (is_color)
		brown();
	move(22, 0);
	addch(DVRIGHT);
	repchr(DHLINE, COLS-2);
	addch(DVLEFT);
	standend();
	mvaddstr(23,2,"Rogue's Name? ");
	is_saved = TRUE;		/*  status line hack @ to disable updates */
	high();
	getinfo(tname,23);
	if (*tname && *tname != ESCAPE)
		strcpy(whoami, tname);
	is_saved = FALSE;  //@ re-enable status line updates
	move(23, 0);
	//@ a single clrtobol(), if available, could replace the next 3 lines
	clrtoeol();
	move(24, 0);
	clrtoeol();
	if (is_color)
		brown();
	mvaddch(22,0,LLWALL);
	mvaddch(22,COLS-1,LRWALL);
	standend();
}


/*
 * readchar:
 *	Return the next input character, from the macro or from the keyboard.
 */
byte
readchar()
{
	int xch;
	byte ch;

	if (*typebuf) {
		SIG2();
		cur_refresh();  //@ macros
		return(*typebuf++);
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	do
	{
		SIG2();  /* Rogue spends a lot of time here @ you bet! */
		cur_refresh();  //@ command input
	}
	while ((xch = getch_timeout(250)) == NOCHAR);
	ch = xlate_ch(xch);
	if (ch == ESCAPE)
		count = 0;
	return ch;
}


/*@
 * newmem - memory allocater
 *        - motto: use malloc() like any sane software or die in 1985
 *
 * Clients should call free() for allocated objects
 */
char *
newmem(nbytes)
	unsigned int nbytes;
{
	void * newaddr;
	if ((newaddr = (char *) malloc(nbytes)) == NULL)
		fatal("No Memory");
	return (char *)newaddr;
}


void
unsetup()
{
#ifndef ROGUE_NO_X11
	if (xdisplay)
	{
		XCloseDisplay(xdisplay);
		xdisplay = NULL;
	}
#endif
}


/*@
 * Busy loop for 1 clock tick or _halt() if clock doesn't tick after a while
 *
 *      ... at least this seems to be the idea, judging by the usage in Rogue.
 *
 * But as it is, this function is a no-op: while loop condition starts at 0,
 * so it immediately breaks out without ever entering the loop. tick increment
 * is never checked, halt() is never executed. I'm not sure if this behavior
 * was intentional or not.
 *
 * Anyway, checking clock ticks with a busy loop is risky: the index is an int,
 * 16-bit in DOS, so it overflows to 0 after "only" 65536 iterations. Assuming
 * both i and j indexes start with 1, halt condition would happen after the
 * first outer loop cycle. And I think even in 1985 a PC could be fast enough
 * to execute such a simple inner loop 65536 times before the clock tick once.
 * 55ms is a long time, even for an 8MHz AT-286.
 *
 * So this could have been be deemed unsuitable as a check for enabled clocks,
 * dangerous as it could lead to a halt, and so it was intentionally disabled.
 *
 *       ... or it could be a bug.
 *
 * Now it pauses for half a tick (27ms), the average wait if intended behavior
 * was working, and tick the clock once only to unlock copy protection, as
 * tick is no longer used or extern'ed.
 */
void
one_tick()
{
/*@
	int otick = tick;
	int i=0,j=0;

	while(i++)
	{
		while (j++)
			if (otick != tick)
				return;
			else if (i > 2)
				_halt();
	}
*/
	msleep(27);
	md_clock();
}


/*@
 * Originally the message would never be seen, as it used printw() after an
 * endwin(), and there was no other blocking call after it, so any  messages
 * would be cleared instantly after display.
 */
/*
 *  fatal: exit with a message
 *  @ moved from main.c, changed to use varargs and actually print the message
 */
void
fatal(const char *msg, ...)
{
	va_list argp;

	cur_endwin();

	va_start(argp, msg);
	vprintf(msg, argp);
	va_end(argp);
	md_exit(EXIT_SUCCESS);
}


/*@
 * The single point of exit for Rogue
 * renamed from exit() to avoid conflict with <stdlib.h>
 * moved from croot.c
 */
void md_exit(int status)
{
	cur_endwin();
	unsetup();
	free_ds();
#ifdef ROGUE_DEBUG
	printf("Exited normally\n");
#endif
	exit(status);
}
