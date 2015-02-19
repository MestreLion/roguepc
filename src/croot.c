/*
 * Modified from the MANX croot to fit the rogue requirements
 */

/*@
 * As this originally does not include rogue.h, needed functions are
 * declared here
 */
void	_exit(int status);  //@ begin.asm, also <stdlib.h>, <unistd.h>
int 	write();  //@ fio.asm, also in <unistd.h> (returning ssize_t)
char	*sbrk(); //@ sbrk.asm, also in <unistd.h>
void	exit_croot(int code);  //@ implemented at end of file
void	unsetup(); //@ mach_dep.c
int 	main();  //@ main.c

#define STDERR_FILENO	2  //@ from <unistd.h>

static char **Argv;
static int Argc;

void
noper()
{
	return;
}

void (*cls_)() = noper;

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
