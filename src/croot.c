/*
 * Modified from the MANX croot to fit the rogue requirements
 */

extern void _exit(int status);  // From <stdlib.h> and begin.asm
void exit(int code);            // implemented at end of file

static char **Argv;
static int Argc;

noper()
{
	return 0;
}

int (*cls_)() = noper;

Croot(cp, first)
register char *cp;
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
	ComOff();
#endif
	unsetup();
	_exit(code);
}
