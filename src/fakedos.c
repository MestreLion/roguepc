/*
 * routines for writing a fake dos
 *
 * @FIXME: replace bdos() calls with appropriate fake ones, possibly reading
 *         from s_drive[] instead of 19h call (unlike the original, which
 *         ignores s_drive here), and perhaps creating a s_maxdrive[] option
 *         instead of 0Eh.
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

void
fakedos(void)
{
	char comline[132];
	char savedir[] = "a:", *comhead;

	wdump();
	clear();
	move (0,0);
	cursor(TRUE);
	*savedir = bdos(0x19,0) + 'A';
	do {
		setmem(comline, sizeof(comline), 0);
		printw("\n%c>",bdos(0x19,0)+'A');
		getinfo(comline,130);
		comhead = stpblk(comline);
		endblk(comhead);
	} while (dodos(comhead));
	dodos(savedir);
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
		if ((!is_alpha(*com)) || drv >= bdos(0x0e, drv))
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
