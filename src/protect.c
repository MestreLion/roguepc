/*
 * C copy protection routines
 */

#include	"rogue.h"

/*@
 * no_step seems to be a flag designed to inhibit the use of debuggers during
 * protect() execution. When set, it triggers a timer in clock() that halts
 * the PC in 20 clock ticks (~1 second), unless no_step is unset again before
 * the timer expires.
 * The flag is set all over protect(), and unset only during drive reads. It
 * was also unset whenever exiting the function, successfully or not.
 */
int no_step;

#ifdef ROGUE_NOGOOD
#define UNDEFINED	0
#define DONTCARE	0

#define	CRC		0x10

static struct sw_regs rom_read = {
	0x206,
	0,
	0x2701,
	UNDEFINED,
	DONTCARE,
	DONTCARE,
	DONTCARE,
	0xF800
} ;

static struct sw_regs sig1_read = {
	0x201,
	UNDEFINED,
	0x2707,
	UNDEFINED,
	DONTCARE,
	DONTCARE,
	DONTCARE,
	UNDEFINED
} ;

static struct sw_regs sig2_read = {
	0x201,
	UNDEFINED,
	0x27F1,
	UNDEFINED,
	DONTCARE,
	DONTCARE,
	DONTCARE,
	UNDEFINED
} ;

/*@
 * Current value of the Data Segment register DS
 * Return a dummy value of 0
 * Originally in dos.asm
 */
int
getds(void)
{
	return 0;
}
#endif

#ifndef ROGUE_NOGOOD
void
protect(int UNUSED(drive))
{
	goodchk = 0xD0D;  //@ success marker: 0xD0D stands for "Dungeons Of Doom"
	no_step = 0;
}
#else
void
protect(int drive)
{
	int i, flags;
	struct sw_regs rgs;
	char buf2[512];
	char buf1[32];

	no_step++;
	rom_read.dx = sig1_read.dx = sig2_read.dx = drive;
	sig1_read.es = sig2_read.es = getds();
	sig1_read.bx = (dosptr)(intptr)(&buf1[0]);  //@ bogus address to fit bx
	sig2_read.bx = (dosptr)(intptr)(&buf2[0]);  //@ ditto

	//@ read sectors until first success, try up to 7 times
	for (i=0,flags=CF;i<7 && (flags&CF);i++)
	{
		rgs = rom_read;
		no_step = 0;
		flags = sysint(SW_DSK,&rgs,&rgs);
		no_step++;
	}
	//@ return if no success
	if (CF&flags)
	{
		no_step = 0;
		return;
	}
	//@ read sectors until first success, try up to 3 times
	for (i=0,flags=CF;i<3 && (flags&CF);i++)
	{
		rgs = sig1_read;
		no_step = 0;
		flags = sysint(SW_DSK,&rgs,&rgs);
		no_step++;
	}
	//@ return if no success
	if (CF&flags)
	{
		no_step = 0;
		return;
	}
	//@ try up to 4 times to get a CRC failure on read
	for (i=0;i<4;i++)
	{
		rgs = sig2_read;
		no_step = 0;
		flags = sysint(SW_DSK,&rgs,&rgs);
		no_step++;
		//@ failure read by bad CRC is expected and required for validation!
		if ((flags&CF) && HI(rgs.ax) == CRC)
		{
			if (memcmp(&buf1[0],&buf2[0x8c],32) == 0)
				goodchk = 0xD0D;
			no_step = 0;
			return;
		}
	}
	no_step = 0;
}
#endif
