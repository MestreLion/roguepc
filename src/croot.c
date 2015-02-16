/*
 * Modified from the MANX croot to fit the rogue requirements
 */

/*@
 * As this originally does not include rogue.h, needed functions are
 * declared here
 */
extern void	_exit(int status);  //@ begin.asm, <stdlib.h>, <unistd.h>
extern int 	write();  //@ <unistd.h>. Actually return ssize_t
extern char	*sbrk(); //@ sbrk.asm, also in <unistd.h>
extern void	exit_croot(int code);  //@ implemented at end of file
extern void	unsetup(); //@ mach_dep.c
extern int 	main();  //@ main.c

#define STDERR_FILENO	2  //@ from <unistd.h>

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
				write(STDERR_FILENO, "Too many args.", 14);
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
	exit_croot(0);
}

//@ renamed from exit() to avoid conflict with <stdlib.h>
void exit_croot(code)
{
	(*cls_)();
#ifdef SDEBUG
	ComOff();  //@ not found
#endif
	unsetup();
	_exit(code);
}
