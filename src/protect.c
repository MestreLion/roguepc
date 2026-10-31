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

void
protect(int UNUSED(drive))
{
	goodchk = 0xD0D;  //@ success marker: 0xD0D stands for "Dungeons Of Doom"
	no_step = 0;
}
