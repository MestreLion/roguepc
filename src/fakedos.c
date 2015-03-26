/*
 * routines for writing a fake dos
 *
 * @FIXME: Scrolling is bugged. Need support from curses.c to expose scrlok()
 *         or perhaps a higher-level API to properly enable and disable it.
 *
 * @FIXME: Originally it was possible to leave fakedos with a non-ASCII key
 *         as the first command char (for example, arrow keys or F1-F12). But
 *         getinfo() now ignores all non-ASCII input, so the only way to leave
 *         is to type the "command" `rogue`. Fixing this will be tricky...
 */

#include	"rogue.h"
#include	"curses.h"

static bool	dodos(char *com);
static int	select_drive(int drv);

void
fakedos(void)
{
	char comline[132];
	char savedir[] = "a:", *comhead;

	wdump();
	clear();
	move (0,0);
	cursor(TRUE);
#ifdef ROGUE_DOS_DRIVE
	*savedir = bdos(0x19,0) + 'A';
#else
	*savedir = current_drive + 'A';  //@ save current drive
#endif
	do {
		setmem(comline, sizeof(comline), 0);
#ifdef ROGUE_DOS_DRIVE
		printw("\n%c>",bdos(0x19,0)+'A');
#else
		printw("\n%c>",current_drive+'A');
#endif
		getinfo(comline,130);
		comhead = stpblk(comline);
		endblk(comhead);
	} while (dodos(comhead));
	dodos(savedir);  //@ restore current drive
	cursor(FALSE);
	clear();
	wrestor();
}

/*
 * execute a dos like command
 */
static
bool
dodos(com)
	char *com;
{
	int drv;

	if ((!isascii(*com)) || (strcmp(com, "rogue") == 0))
	{
		return FALSE;
	}
	if (com[1] == ':' && com[2] == 0)
	{
		//@ smart way to get toupper() and 'A'=>0 in a single strike
		drv = (*com & 0x1f) - 1;

		printw("\n");
		if ((!is_alpha(*com)) || drv >= select_drive(drv))
		{
			printw("Invalid drive specification\n");
		}
	}
	else if (com[0])
	{
		printw("\nBad command or file name\n");
	}
	return TRUE;
}

/*
 * Fake DOS INT 0Eh call. Could be in mach_dep.c, but it's only used here.
 * Original called bdos() directly in dodos()
 */
static
int
select_drive(int drv)
{
#ifdef ROGUE_DOS_DRIVE
	return bdos(0x0e, drv);
#else
	if (drv >= 0 && drv <= last_drive)
	{
		current_drive = drv;
	}
	return last_drive;
#endif
}
