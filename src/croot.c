/*
 * Modified from the MANX croot to fit the rogue requirements
 */

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
