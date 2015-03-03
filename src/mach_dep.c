/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"
#include	"curses.h"

#define ULINE() if(is_color) lmagenta();else uline();
#ifdef ROGUE_DOS_CLOCK
#define TICK_ADDR 0x70  //@ RTC Interrupt handler. See clock_on()
static dosptr clk_vec[2];
#else
/*
 * time_t constant to disable the clock
 * Chosen to match error value returned by time()
 */
#define CLOCK_OFF	-1
/*
 * Clock rate in ticks per second, as expected by Rogue
 * Matches the clock rate of the IBM-PC 8253 PIT as set by BIOS and used in DOS
 * Actual frequency is 3579545Hz / 3 / 65536 = 18.206507365
 */
#define CLOCK_RATE	18.2
/*
 * Current wall time, used to update in-game ticks
 */
time_t	current_time = CLOCK_OFF;
#endif
static int ocb;

/*@
 * Initial values of the CS and DS registers
 * Set to dummy values of a "Hello World!" program as reported by gdb
 * Originally set by begin.asm
 */
int _dsval = 0x00;
dosptr _csval = 0x33;  /*@ dummy */

/*@
 * Global tick counter
 * Automatically incremented by clock() 18.2 times per second
 * Originally set by dos.asm
 */
unsigned int tick = 0;

/*
 * Permanent stack data
 * @ originally defined in main.c
 */
struct sw_regs *regs;

#ifdef ROGUE_DEBUG
/*
 * Used to suppress printing BIOS INT calls. Used by some curses calls to
 * prevent flooding output with INT 10h/2 (move cursor) and INT 10h/9h (write
 * character) debug messages when printing strings.
 *
 */
bool print_int_calls = TRUE;
#endif

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
 * Current value of the Data Segment register DS
 * Used in copy protection, see protect()
 *
 * Return the current (dummy) value of DS
 *
 * Originally in dos.asm
 */
int
getds()
{
	return _dsval;  // hey, it's still the same!
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
pokeb(segment, offset, value)
	int segment;
	int offset;
	byte value;
{
	;  // it was written, I promise!
}


/*@
 * Read a byte from a segment:offset memory address
 *
 * Dummy, always return 0
 *
 * Originally in dos.asm. It zeroed AH so return is explicitly typed as byte.
 */
byte
peekb(segment, offset)
	int segment;
	int offset;
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
	int port;
	byte value;
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
	int port;
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
void
dmaout(data, wordlength, segment, offset)
	void * data;
	unsigned int wordlength;
	unsigned int segment;
	unsigned int offset;
{
#ifdef ROGUE_DOS_CURSES
#ifdef ROGUE_DEBUG
	if (segment != scr_ds || wordlength > 1)  // if not single char to screen
		printf("dmaout(%p, %d, %04x:%04x)\n",
				data, wordlength, segment, offset);
#endif
#endif
	; // blazing fast!
}


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
	void * buffer;
	unsigned int wordlength;
	unsigned int segment;
	unsigned int offset;
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
	endwin();
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
	reg.ax = 0x2523;  // hooking to INT 23h
	reg.ds = _csval;  // technically this should be CS from dos.asm or main.c
	reg.dx = (dosptr)(intptr)quit;  // see clock_on() for note on casting
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
#endif


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
 * In any case, this function must be replaced with a portable way of hooking
 * clock() to a timer that does not rely on ancient real mode ISR/IVT model.
 * Meanwhile, md_clock() should be manually called in key loops such as waiting
 * for (or after) user input.
 */
void
clock_on()
{
#ifdef ROGUE_DOS_CLOCK
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
#else
	current_time = time(NULL);
#endif
}


/*@
 * Restore INT 70h ISR to its original value, as saved by clock_on()
 * clock() will no longer be called, and thus tick will not be updated.
 */
void
no_clock()
{
#ifdef ROGUE_DOS_CLOCK
	dmaout(clk_vec, 2, 0, TICK_ADDR);
#else
	current_time = CLOCK_OFF;
#endif
}


/*@
 * Increment the global tick
 *
 * This is supposed to be called 18.2 times per second, to maintain the tick
 * rate found in DOS system timer expected by Rogue. The game heavily relies
 * on clock() being periodically (and automatically) called via some triggering
 * mechanism such as an IRQ timer or signal, as made by clock_on(). If tick is
 * not incremented some Bad Things will happen: Rogue will _halt() on the first
 * one_tick() call, or enter infinite loop on tick_pause() and epyx_yuck().
 *
 * This should be changed to a sane API where code explicitly calls clock() to
 * get current tick instead of expecting a value to be magically updated by
 * an external interrupt.
 *
 * Besides, a C function could never be directly assigned as an ISR, as calling
 * and return conventions for an interrupt handler are different from a normal
 * function (requires IRET, preserving AX, etc)
 *
 * Meanwhile, tick is updated based on real time via time(), so it does not
 * require being automatically called at regular intervals. It can be called at
 * any time, will calculate elapsed time since last call and adjust tick
 * accordingly. Currently, with time resolution of a second, for maximum
 * portability.
 *
 * Originally in dos.asm, renamed from clock() to avoid conflict in <time.h>
 *
 * It also performed some anti-debugger checks and copy protection measures.
 * The copy-protection is fully reproduced to the extent of my knowledge.
 * The anti-debugger tests, if failed, lead to _halt(), and are only partially
 * reproduced here. See protect.c for details.
 */
void
md_clock()
{
#ifdef ROGUE_DOS_CLOCK
	//@ tick the old clock
	tick++;
#else
	time_t	new_time;
	if (current_time != CLOCK_OFF)
	{
		new_time = time(NULL);
		tick += (new_time - current_time) * CLOCK_RATE;
		current_time = new_time;
	}
#endif

	//@ anti debugging: halt after 20 ticks if no_step is set
	if (no_step && ++no_step > 20)
		_halt();

	/*@
	 * copy protection: set tombstone strings (name, killed by) to actual
	 * player name and death reason, and restore hit multiplier, only if
	 * floppy check succeeded. Only a single (successful) tick was required.
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
 * Return current local time as a pointer to a struct
 */
struct tm *
md_localtime()
{
	time_t secs = time(NULL);
	return localtime(&secs);
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
#ifdef ROGUE_DOS_TIME
	bdos(0x2C);
	return(regs->cx + regs->dx);
#else
	return (int)time(NULL);
#endif  // ROGUE_DOS_TIME
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

void
credits()
{
	int i;
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
	for(i=1;i<(COLS-1);i++) {
		move(22,i);
		putchr(HWALL);
	}
	mvaddch(22,0,VRIGHT);
	mvaddch(22,COLS-1,VLEFT);
	standend();
	mvaddstr(23,2,"Rogue's Name? ");
	is_saved = TRUE;		/*  status line hack  */
	high();
	getinfo(tname,23);
	if (*tname && *tname != ESCAPE)
		strcpy(whoami, tname);
	is_saved = FALSE;
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
 * always return FALSE, indicating a key was pressed. Not a problem as the main
 * caller, readchar(), is a blocking function.
 *
 * BIOS INT 16h/AH=1, Get Keyboard Status
 * Return:
 * ZF = 0 if a key pressed (even Ctrl-Break). Not tested, COFF() handles that.
 * AH = scan code. 0 if no key was pressed
 * AL = ASCII character. 0 if special function key or no key pressed
 * So AX = 0 for no key pressed
 */
bool
no_char()
{
	struct sw_regs reg;
	reg.ax = HIGH(1);
	return !(swint(SW_KEY, &reg) == 0);
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
		return(*typebuf++);
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	do
		SIG2();				/* Rogue spends a lot of time here */
	while ((xch = getch()) == NOCHAR);
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


#define PC  0xff
#define XT  0xfe
#define JR  0xfd
#define AT  0xfc
/*@
 * Return TRUE if the system is identified as an IBM PCJr ("PC Junior")
 *
 * It is only used for setting no_check in curses winit().
 *
 * 0xF000:0xFFFE 1  IBM computer-type code; see also BIOS INT 15h/C0h
 *  0xFF = Original PC
 *  0xFE = XT or Portable PC
 *  0xFD = PCjr
 *  0xFC = AT (or XT model 286) (or PS/2 Model 50/60)
 *  0xFB = XT with 640K motherboard
 *  0xFA = PS/2 Model 30
 *  0xF9 = Convertible PC
 *  0xF8 = PS/2 Model 80
 */
bool
isjr()
{
	static int machine = 0;

	if (machine == 0) {
		dmain(&machine,1,0xf000,0xfffe);
		machine &= 0xff;
	}
	return machine == JR;
}

int
swint(intno, rp)
	int intno;
	struct sw_regs *rp;
{
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
	int intno;
	struct sw_regs *inregs, *outregs;
{
#ifdef ROGUE_DOS_CURSES
#ifdef ROGUE_DEBUG
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
 */
void
one_tick()
{
	int otick = tick;
	int i=0,j=0;

	while(i++)
	{
#ifndef ROGUE_DOS_CLOCK
		md_clock();
#endif
		while (j++)
			if (otick != tick)
				return;
			else if (i > 2)
				_halt();
	}
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
