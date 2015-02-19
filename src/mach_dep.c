/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"
#include	"curses.h"
#include	"keypad.h"

#define ULINE() if(is_color) lmagenta();else uline();
#define TICK_ADDR 0x70  //@ RTC Interrupt handler. See clock_on()
static dosptr clk_vec[2];
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


/*@
 * Checksum of the game executable
 * Used as integrity check, see new_level() and command()
 *
 * Return a dummy value matching the expected CSUM value defined in rogue.h
 * to avoid triggering copy protection measures.
 *
 * In the future it could be controlled via command line options to simulate
 * original behavior in case of a tampered executable.
 *
 * Originally in dos.asm
 */
int
csum()
{
	return CSUM;
}


/*@
 * Current value of the Data Segment register DS
 * Used in copy protection, see protect()
 *
 * Return the initial (dummy) value of DS
 *
 * Originally in dos.asm
 */
int
getds()
{
	return _dsval;
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
#ifdef ROGUE_DEBUG
	if (segment != scr_ds)
		printf("dmaout(%p, %d, %04x:%04x)\n",
				data, wordlength, segment, offset);
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
	printf("HALT!");
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
 */
void
clock_on()
{
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
 * clock() will no longer be called, and thus tick will not be updated.
 */
void
no_clock()
{
	dmaout(clk_vec, 2, 0, TICK_ADDR);
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
 * Originally in dos.asm, renamed from clock() to avoid conflict in <time.h>
 *
 * It also performed some anti-debugger integrity checks and copy protection
 * measures. The anti-debugger tests, if failed, lead to _halt(), and so are
 * not reproduced here. The copy-protection measures are partially reproduced.
 */
void
md_clock()
{
	tick++;

	if (hit_mul != 1 && goodchk == 0xD0D)
	{
		kild_by = prbuf;
		your_na = whoami;
		hit_mul = 1;
	}
}


/*@
 * Renamed from srand() to avoid collision with <stdlib.h>
 * Signature and usage completely different from srand()
 *
 * Call DOS INT 21h service 2C (Get Time) and return the sum of return
 * registers CX and DX, a combination of HH:MM:SS.ss with hundredths of a
 * second resolution as an integer.
 *
 * This could be replaced with <time.h> (int) time(NULL), but not only numbers
 * have a completely different meaning, but also time() has only second
 * resolution, and DOS INT 21h/2C has a 24-hour cycle.
 *
 * However, for an RNG seed both are suitable.
 */
/*
 * returns a seed for a random number generator
 */
int
srand_time()
{
#ifdef DEBUG
	return ++dnum;
#else
	/*
	 * Get Time
	 */
	bdos(0x2C);
	return(regs->cx + regs->dx);
#endif
}


/*@
 * Return a seed for the RNG, POSIX version
 * Using time() as any sane software
 */
int
md_srand()
{
	return (int)time(NULL);
}


/*
 * flush_type:
 *	Flush typeahead for traps, etc.
 */
void
flush_type()
{
#ifdef CRASH_MACHINE
	regs->ax = 0xc06;		/* clear keyboard input */
	regs->dx = 0xff;		/* set input flag */
	swint(SW_DOS, regs);
#endif //CRASH_MACHINE
	typeahead = "";
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
		putchr(205);
	}
	mvaddch(22,0,204);
	mvaddch(22,COLS-1,185);
	standend();
	mvaddstr(23,2,"Rogue's Name? ");
	is_saved = TRUE;		/*  status line hack  */
	high();
	getinfo(tname,23);
	if (*tname && *tname != ESCAPE)
		strcpy(whoami, tname);
	is_saved = FALSE;
	blot_out(23,0,24,COLS-1);
	if (is_color)
		brown();
	mvaddch(22,0,0xc8);
	mvaddch(22,COLS-1,0xbc);
	standend();
}

/*
 * Table for IBM extended key translation
 */
static struct xlate {
	byte keycode, keyis;
} xtab[] = {
	{C_HOME,	'y'},
	{C_UP,		'k'},
	{C_PGUP,	'u'},
	{C_LEFT,	'h'},
	{C_RIGHT,	'l'},
	{C_END,		'b'},
	{C_DOWN,	'j'},
	{C_PGDN,	'n'},
	{C_INS,		'>'},
	{C_DEL,		's'},
	{C_F1,		'?'},
	{C_F2,		'/'},
	{C_F3,		'a'},
	{C_F4,		CTRL('R')},
	{C_F5,		'c'},
	{C_F6,		'D'},
	{C_F7,		'i'},
	{C_F8,		'^'},
	{C_F9,		CTRL('F')},
	{C_F10,		'!'},
	{ALT_F9,	'F'}
};


/*@
 * Non-blocking function that return TRUE if no key was pressed.
 *
 * Similar to ! kbhit() from DOS <conio.h>. POSIX has no (easy) replacement,
 * but this function will no longer be needed when ncurses getch() is set non-
 * blocking mode via nodelay() or timeout()
 *
 * Originally in dos.asm, calling a BIOS INT, which is reproduced here.
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
	register struct xlate *x;
	register byte ch;

	if (*typeahead) {
		SIG2();
		return(*typeahead++);
	}
	/*
	 * while there are no characters in the type ahead buffer
	 * update the status line at the bottom of the screen
	 */
	do
		SIG2();				/* Rogue spends a lot of time here */
	while (no_char());
	/*
	 * Now read a character and translate it if it appears in the
	 * translation table
	 */
	for (ch = getch(), x = xtab; x < xtab + (sizeof xtab) / sizeof *xtab; x++)
		if (ch == x->keycode) {
			ch = x->keyis;
			break;
		}
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
 * index registers are copied from input.
 * Return ax as status
 */
int
sysint(intno, inregs, outregs)
	int intno;
	struct sw_regs *inregs, *outregs;
{
#ifdef ROGUE_DEBUG
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

	return outregs->ax;
}

int
set_ctrlb(state)
{
	struct sw_regs rg;
	int retcode;

	rg.ax = 0x3300;
	swint(SW_DOS,&rg);
	retcode = rg.dx &0xFF;

	rg.ax = 0x3300;
	rg.dx = (state) ? 1 : 0;
	swint(SW_DOS,&rg);

	return retcode;
}

void
unsetup()
{
	set_ctrlb(ocb);
}

void
one_tick()
{
	int otick = tick;
	int i=0,j=0;

	while(i++)
		while (j++)
			if (otick != tick)
				return;
			else if (i > 2)
				_halt();
}
