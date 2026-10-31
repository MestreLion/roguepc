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
	*savedir = current_drive + 'A';  //@ save current drive
	do {
		setmem(comline, sizeof(comline), 0);
		printw("\n%c>",current_drive+'A');
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
 * Fake DOS "select disk" call. Original called bdos() directly in dodos()
 */
static
int
select_drive(int drv)
{
	if (drv >= 0 && drv <= last_drive)
	{
		current_drive = drv;
	}
	return last_drive;
}


/*@
 * Find the drive where game files are located, only used for copy protection.
 *
 * Return an int corresponding to the current drive or the "drive" value
 * from the fake environment (rogue.opt), where 0=A, 1=B, etc
 *
 * Also contained a no-op code which checked the existence of a file named
 * "jatgnas.8ys" in the root of such drive, but ignored the check result.
 */
int
find_drive(void)
{
	int drive = current_drive;
	char spec = s_drive[0];

	if (is_alpha(spec))
	{
		/*@
		 * It looks like this block could be replaced with:
		 * drive = tolower(spec) - 'a';
		 */
		if (is_upper(spec))
			drive = spec - 'A';
		else
			drive = spec - 'a';
	}
	/*@
	 * The following nonsense strongly indicates this is either a partial,
	 * work in progress function, or a leftover, or an already cracked version.
	 * access() is a useless call as its return code is not being checked.
	 * It is now disabled.
	char filename[30];
	strcpy(filename,"a:jatgnas.8ys");
	filename[0] += (char)drive;
	access(filename);
	 */

	return drive;
}
