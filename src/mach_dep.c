/*
 * Various installation dependent routines
 *
 * mach_dep.c	1.4 (A.I. Design) 12/1/84
 */

#include	"rogue.h"
#include	"curses.h"
#include	"keypad.h"

#define ULINE() if(is_color) lmagenta();else uline();
#define TICK_ADDR 0x70
static int clk_vec[2];
static int ocb;

/*@
 * Initial values of the CS and DS registers
 * Set to dummy values of a "Hello World!" program as reported by gdb
 */
int _dsval = 0x00;
int _csval = 0x33;


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
	return 0;  // we rebooted recently, so...
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

void
clock_on()
{
	/*@
	 * Using proper pointer size for the array and the following cast
	 * makes the compiler happy, but it would certainly cause trouble in the
	 * dmaout() call, as it expects 16-bit pointers. Not a big deal since soon
	 * the dma{in,out}() functions will be stubbed or replaced.
	 */
	void * new_vec[2];

	new_vec[0] = clock;
	new_vec[1] = (void *)(intptr)_csval;
	dmain(clk_vec, 2, 0, TICK_ADDR);
	dmaout(new_vec, 2, 0, TICK_ADDR);
	cls_ = no_clock;
}

void
no_clock()
{
	dmaout(clk_vec, 2, 0, TICK_ADDR);
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

#define PC	0xff
#define XT  0xfe
#define JR  0xfd
#define AT	0xfc

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
