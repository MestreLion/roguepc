/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"
#include	"curses.h"

#ifdef ROGUE_DOS_CLOCK
#define TICK_ADDR 0x70  //@ RTC Interrupt handler. See clock_on()
static dosptr clk_vec[2];
/*@
 * Global tick counter
 * Automatically incremented by clock() 18.2 times per second
 * Originally set by dos.asm
 */
unsigned int tick = 0;
#endif
static int ocb;

/*
 * Permanent stack data
 * @ originally defined in main.c
 */
static struct sw_regs _treg;
struct sw_regs *regs = &_treg;

#ifdef ROGUE_DEBUG
/*
 * Used to suppress printing BIOS INT calls. Used by some curses calls to
 * prevent flooding output with INT 10h/2 (move cursor) and INT 10h/9h (write
 * character) debug messages when printing strings.
 *
 */
bool print_int_calls = TRUE;
#endif

#ifndef ROGUE_DOS_DRIVE
/*@
 * These were created for fakedos() to replace DOS INT 19h and 0Eh calls, and
 * are independent from env file s_drive[], just like the original. Created as
 * externs to allow future integration with env file and run-time selection.
 */
int current_drive = ROGUE_CURRENT_DRIVE;  //@ current fake drive (A=0, B=1, ...)
int last_drive = ROGUE_LAST_DRIVE;  //@ last available drive
#endif


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
	int fd;
	char *cmd, buf[BUFSIZE];
	FILE *fp;

	if (getenv("DISPLAY"))
	{
		//@ terminal emulator under X, such as xterm / gnome-terminal
		cmd = "xset -q | grep 'LED mask' | cut -d: -f4 2>/dev/null";
		if ((fp = popen(cmd, "r")) != NULL)
		{
			while (fgets(buf, sizeof(buf), fp) != NULL)
			{
				//@ swap num and scr lock
				state = swap_bits(atoi(buf), 0, 2, 1);
				break;
			}
			if(pclose(fp))
			{
				//@ command not found or exited with status != 0
				state = 0;
			}
		}
	}
	else
	{
#ifdef __linux__
		//@ TTY such as getty / linux console
		if ((fd = open("/dev/tty", O_RDONLY | O_NOCTTY)) == -1 ||
				ioctl(fd, KDGKBLED, &state) == -1)
		{
			state = 0;
		}
		close(fd);
#endif
	}
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
 * Write a byte to a segment:offset memory address
 *
 * Dummy no-op, obviously. It's 2015... protected mode and flat memory model
 * would make true poking either impossible or very dangerous.
 *
 * But hey, it's 2015... we can easily create a 1MB array of bytes and let
 * Rogue play all around in its own VM. Nah... this a port, not a DOSBox remake.
 * Still, this idea might be useful for debugging.
 *
 * Originally in dos.asm.
 *
 * value is typed as byte to make clear that the high byte is ignored.
 */
void
pokeb(offset, segment, value)
	int UNUSED(offset);
	int UNUSED(segment);
	byte UNUSED(value);
{
	;  // it was written, I promise!
}


/*@
 * Read a byte from a segment:offset memory address
 *
 * Dummy, always return 0
 *
 * Originally in dos.asm. It zeroed AH so return is explicitly typed as byte.
 *
 * Only used in load.c to read CGA (0xB800) and BIOS (0x40) data
 */
byte
peekb(offset, segment)
	int UNUSED(offset);
	int UNUSED(segment);
{
	return 0;  // we just rebooted, so...
}


/*@
 * Write a byte to an I/O port
 *
 * Dummy no-op
 *
 * Originally in dos.asm
 *
 * Asm equivalent function is as a wrapper to OUT x86 CPU instruction.
 * Only AL was sent, hence byte as argument type. Port must be 16-bit as it is
 * written to DX, so a type uint16_t could be used enforce this.
 */
void
out(port, value)
	int UNUSED(port);
	byte UNUSED(value);
{
	;  // and it's out! :)
}


/*@
 * Read from an I/O port
 *
 * A dummy wrapper to the x86 IN instruction. Return 0
 */
byte
in(port)
	int UNUSED(port);
{
	return 0;  // maybe it's not connected :P
}


/*@
 * Write data to memory starting at segment:offset address
 *
 * Length of data is measured in words (16-bit), the size of int in DOS
 *
 * Dummy no-op, see pokeb().
 *
 * Originally in dos.asm.
 *
 * While original is technically "direct memory access", the name is misleading
 * as it has nothing to do with DMA channels. And despite documentation on
 * dos.asm, it has no particular ties to video: it is a general use memory
 * writer that happens to be most often used to write to video memory address.
 * Asm works by setting the arguments and calling REP MOVSW
 */
#if defined(ROGUE_DOS_CURSES) && defined(ROGUE_DEBUG)
void
dmaout(data, wordlength, segment, offset)
	void * data;
	unsigned int wordlength;
	unsigned int segment;
	unsigned int offset;
{
	printf("dmaout(%p, %d, %04x:%04x)\n",
			data, wordlength, segment, offset);
}
#else
void
dmaout(data, wordlength, segment, offset)
	void UNUSED(*data);
	unsigned int UNUSED(wordlength);
	unsigned int UNUSED(segment);
	unsigned int UNUSED(offset);
{
		; // blazing fast!
}
#endif


/*@
 * Read memory starting at segment:offset address and store contents in buffer
 *
 * Length of buffer is measured in words (16-bit), the size of int in DOS
 *
 * Dummy no-op, leave buffer unchanged.
 *
 * Originally in dos.asm. See notes on dmaout()
 */
void
dmain(buffer, wordlength, segment, offset)
	void UNUSED(*buffer);
	unsigned int UNUSED(wordlength);
	unsigned int UNUSED(segment);
	unsigned int UNUSED(offset);
{
	;
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


/*@
 * Hook quit() as the ISR for CTRL-BREAK interrupts
 *
 * Originally in dos.asm
 *
 * Hook is performed using DOS INT 21h/AH=25h - Set Interrupt Vector
 * AL = interrupt number to hook - 23h for CTRL-BREAK
 * DS = handler function segment
 * DX = handler function offset
 *
 * Actually it did not hook quit() directly, instead it hooked an asm wrapper
 * that called quit(). For simplicity, it now "hooks" quit(), as this is bogus
 * code anyway. See notes on clock().
 */
void
COFF()
{
	struct sw_regs reg;
	reg.ax = 0x2523;  //@ hooking to INT 23h
	reg.ds = 0x33;  //@ dummy value for dos.asm's CS register
	reg.dx = (dosptr)(intptr)quit;  //@ see clock_on() for note on casting
	swint(SW_DOS, &reg);
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
	/*
	 * Vector CTRL-BREAK to call quit()
	 */
	COFF();
	ocb = set_ctrlb(0);
}


#ifdef ROGUE_DOS_CLOCK
/*@
 * No-op function, probably a stub for cls_ until it gets set to no_clock()
 * moved from croot.c
 */
void
noper()
{
	return;
}

void (*cls_)() = noper;


/*@
 * Hook clock() as the Interrupt Service Routine (ISR) for INT 70h,
 * saving the current handler in clk_vec.
 *
 * I honestly don't understand what is going on here: INT 70h is triggered by
 * RTC via IRQ8 only on IBM-AT/XT-286 onwards. The original IBM-PC and IBM-XT
 * had no RTC, so INT 70h was not regularly triggered. And clock(), originally
 * in dos.asm, incremented tick C var on every call, which was used all over.
 * Does this mean Rogue does not work on IBM-XT?
 *
 * But epyx_yuck() and SIG2() strongly suggests tick is incremented ~18 times
 * per, consistent with XT's original timer. The IBM-AT BIOS by default sets
 * the RTC rate to 1024 times per second, not 18.2.
 *
 * I could not find any clock rate reprogramming in Rogue, so I'm quite puzzled
 * on how tick works, and what its actual and expected rates are.
 *
 * In any case, if clock() was still being used for timing, this function
 * should provide a portable way of hooking it to a timer that does not rely on
 * ancient real mode ISR/IVT model.
 */
void
clock_on()
{
	/*@
	 * CS register value. Originally an extern set by begin.asm
	 * Set to dummy value of a "Hello World!" program as reported by gdb
	 */
	dosptr _csval = 0x33;

	/*@
	 * Craft the 4-byte CS:offset function pointer for clock()
	 * Array indexes are swapped (CS=1, offset=0) as it writes directly to IVT
	 *
	 * Using the actual clock() protected mode address and "casting" it to a
	 * DOS real mode 16-bit offset makes the compiler happy and produce a legit
	 * dmaout() call. But obviously the values in new_vec are completely bogus.
	 */
	dosptr new_vec[2];  //@ type must match clk_vec

	new_vec[0] = (dosptr)(intptr)clock;
	new_vec[1] = _csval;

	/*@
	 * I wonder why using IVT directly instead of the safer DOS INT 21h/25h,35h
	 * calls like the well behaved COFF() does?
	 */
	dmain(clk_vec, 2, 0, TICK_ADDR);
	dmaout(new_vec, 2, 0, TICK_ADDR);
	cls_ = no_clock;
}


/*@
 * Restore INT 70h ISR to its original value, as saved by clock_on()
 * clock() would no longer be called, and thus tick will not be updated.
 */
void
no_clock()
{
	dmaout(clk_vec, 2, 0, TICK_ADDR);
}
#endif  // ROGUE_DOS_CLOCK


/*@
 * Increment the global tick
 *
 * This was supposed to be called 18.2 times per second, to maintain the tick
 * rate found in DOS system timer expected by Rogue. The game originally relied
 * on clock() being periodically (and automatically) called via some triggering
 * mechanism such as an IRQ timer or signal, as made by clock_on(). If tick was
 * not incremented some Bad Things would happen: Rogue could _halt() on first
 * one_tick() call, or enter infinite loop on tick_pause() and epyx_yuck().
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
#ifdef ROGUE_DOS_CLOCK
	//@ tick the old clock
	tick++;
#endif

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
#ifdef ROGUE_DOS_CLOCK
	bdos(0x2C);
	return(regs->cx + regs->dx);
#else
	return (int)md_time();
#endif  // ROGUE_DOS_CLOCK
#endif  // DEMO
}


/*
 * flush_type:
 *	Flush typebuf for traps, etc.
 */
void
flush_type()
{
#ifdef CRASH_MACHINE
	regs->ax = 0xc06;		/* clear keyboard input */
	regs->dx = 0xff;		/* set input flag */
	swint(SW_DOS, regs);
#endif //CRASH_MACHINE
	typebuf = "";
}

//@ I wonder why this is here instead of main.c (or *anywhere* else)
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
#ifdef ROGUE_DOS_CURSES
	blot_out(23,0,24,COLS-1);
#else
	move(23, 0);
	//@ a single clrtobol(), if available, could replace the next 3 lines
	clrtoeol();
	move(24, 0);
	clrtoeol();
#endif
	if (is_color)
		brown();
	mvaddch(22,0,LLWALL);
	mvaddch(22,COLS-1,LRWALL);
	standend();
}


/*@
 * Non-blocking function that return TRUE if no key was pressed.
 *
 * Similar to ! kbhit() from DOS <conio.h>. POSIX has no (easy) replacement,
 * but this function will no longer be needed when ncurses getch() is set non-
 * blocking mode via nodelay() or timeout()
 *
 * Originally in dos.asm, calling a BIOS INT, which is reproduced here.
 *
 * But as sysint() is just a stub that returns ax = 0, this function will
 * always return FALSE, indicating a key was pressed.
 *
 * No longer used, as readchar() now uses non-blocking input internally.
 *
 * BIOS INT 16h/AH=1, Get Keyboard Status
 * Return:
 * ZF = 0 if a key pressed (even Ctrl-Break). Not tested, COFF() handles that.
 * AH = scan code. 0 if no key was pressed
 * AL = ASCII character. 0 if special function key or no key pressed
 * So AX = 0 for no key pressed

bool
no_char()
{
	struct sw_regs reg;
	reg.ax = HIGH(1);
	return !(swint(SW_KEY, &reg) == 0);
}
 */


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


int
bdos(fnum, dxval)
	int fnum, dxval;
{
	register struct sw_regs *saveptr;

	regs->ax = fnum << 8;
	regs->bx = regs->cx = 0;
	regs->dx = dxval;
	saveptr = regs;
	swint(SW_DOS,regs);
	regs = saveptr;
	return(0xff & regs->ax);
}

/*
 *  newmem - memory allocater
 *         - motto: allocate or die trying
 */
/*@ Deprecated, see the new newmem() below
char *
newmem(nbytes,clrflag)
	unsigned int nbytes;
	int clrflag;
{
	register char *newaddr;

	newaddr = sbrk(nbytes);
	if (newaddr == (void *)-1)
		fatal("No Memory");
	end_mem = newaddr + nbytes;
	if ((intptr)end_mem & 1)  //@ guarantee word (16-bit) alignment?
		end_mem = sbrk(1);
	return(newaddr);
}
*/

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


int
swint(intno, rp)
	int intno;
	struct sw_regs *rp;
{
	//@ DS register value. Originally an extern set by begin.asm, now a dummy
	int _dsval = 0x00;

	rp->ds = rp->es = _dsval;
	sysint(intno, rp, rp);
	return rp->ax;
}

/*@
 * sysint() - System Interrupt Call
 * This was available as a C library function in old DOS compilers
 * Created here as a stub: output general registers are zeroed,
 * index and segment register values are copied from input.
 * Return FLAGS register, or rather a dummy with reasonable values
 */
int
sysint(intno, inregs, outregs)
#if defined(ROGUE_DOS_CURSES) && defined(ROGUE_DEBUG)
	int intno;
#else
	int UNUSED(intno);
#endif
	struct sw_regs *inregs, *outregs;
{
#if defined(ROGUE_DOS_CURSES) && defined(ROGUE_DEBUG)
	if(print_int_calls)
		printf("INT %x,%2X\t"
				"al=%2X\t"
				"bx=%4X\t"
				"cx=%4X\t"
				"dx=%4X\t"
				"si=%4X\t"
				"di=%4X\t"
				"ds=%4X\t"
				"es=%4X\n",
				intno,
				HI(inregs->ax),
				LOW(inregs->ax),
				inregs->bx,
				inregs->cx,
				inregs->dx,
				inregs->si,
				inregs->di,
				inregs->ds,
				inregs->es);
#endif
	outregs->ax = 0;
	outregs->bx = 0;
	outregs->cx = 0;
	outregs->dx = 0;
	outregs->si = inregs->si;
	outregs->di = inregs->di;
	outregs->ds = inregs->ds;
	outregs->es = inregs->es;

	// reserved flags and IF set, all others unset
	return 0xF22A;
}

bool
set_ctrlb(state)
	bool state;
{
	struct sw_regs rg;
	int retcode;

	rg.ax = 0x3300;
	swint(SW_DOS,&rg);
	retcode = rg.dx &0xFF;

	rg.ax = 0x3300;  //@ shouldn't this be 0x3301? As it is it just reads again
	rg.dx = state;
	swint(SW_DOS,&rg);

	return retcode;
}

void
unsetup()
{
	set_ctrlb(ocb);
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
#ifdef ROGUE_DOS_CLOCK
	//@ restore the clock, it if was ever set
	(*cls_)();
#endif
	cur_endwin();
	unsetup();
	free_ds();
#ifdef ROGUE_DEBUG
	printf("Exited normally\n");
#endif
	exit(status);
}
