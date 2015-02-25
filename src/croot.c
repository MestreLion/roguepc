/*
 * Modified from the MANX croot to fit the rogue requirements
 */

/*@
 * As this originally does not include rogue.h, needed functions are
 * declared here
 */
//@ exit()
#include <stdlib.h>

void	unsetup(); //@ mach_dep.c
void	free_ds(); //@ init.c

/*@
 * No-op function, probably a stub for cls_ until it gets set to no_clock()
 */
void
noper()
{
	return;
}

void (*cls_)() = noper;

/*@ Former point of entry, called from begin.asm.
 *  Now Rogue starts with main()
 *
 *  Original location for functions called:
void	_exit(); //@ begin.asm
int 	write(); //@ fio.asm
char	*sbrk(); //@ sbrk.asm,
void	exit();  //@ former croot_exit()

static char **Argv;
static int Argc;

void
Croot(cp, first)
	register char *cp;
	int first;
{
	register char **cpp;

	Argv = (char **)sbrk((first+1)*sizeof(char *));
	Argv[0] = "";
	cpp = &Argv[Argc = first];
	for (;;) {
		while (*cp == ' ' || *cp == '\t')
			++cp;
		if (*cp == 0)
			break;
		{
			*cpp++ = cp;
			Argc++;
			if (sbrk(sizeof(char *)) == (char *)-1) {
				write(2, "Too many args.", 14);
				_exit(200);
			}
			while (*++cp)
				if (*cp == ' ' || *cp == '\t') {
					*cp++ = 0;
					break;
				}
		}
	}
	*cpp = 0;
	main(Argc,Argv);
	exit(0);
}
*/

/*@
 * The single point of exit for Rogue
 * renamed from exit() to avoid conflict with <stdlib.h>
 */
void croot_exit(status)
	int status;
{
	(*cls_)();
#ifdef SDEBUG
	//@ ComOff();  //@ not found
#endif
	unsetup();
	free_ds();
	exit(status);
}
