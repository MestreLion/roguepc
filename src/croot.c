/*
 * Modified from the MANX croot to fit the rogue requirements
 */

/*@
 * As this originally does not include rogue.h, needed functions are
 * declared here
 */
extern void _exit(int status);  //@ begin.asm, also found in <stdlib.h>
extern int write();  //@ <unistd.h>. Actually return ssize_t
extern void exit(int code);  //@ implemented at end of file
extern void unsetup(); //@ mach_dep.c
extern int main();  //@ main.c

static char **Argv;
static int Argc;

int
noper()
{
	return 0;
}

int (*cls_)() = noper;

void
Croot(cp, first)
	register char *cp;
	int first;
{
	register char **cpp;
	char *sbrk();

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

void exit(code)
{
	(*cls_)();
#ifdef SDEBUG
	ComOff();  //@ not found
#endif
	unsetup();
	_exit(code);
}
