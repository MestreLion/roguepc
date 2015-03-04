/*
 * save and restore routines
 *
 * save.c	1.32	(A.I. Design)	12/13/84
 */

/*
 *  routines for saving a program in any given state.
 *  This is the first pass of this, so I have no idea
 *  how it is really going to work.
 *  The two basic functions here will be "save" and "restor".
 */

#include "rogue.h"
#include "curses.h"

/*@
 * Addresses used to save and restore statically sized global vars.
 * Changed from char to actual pointers, so we can set to a dummy address
 * Originally externs set by begin.asm
 */
char dummy[1234];  //@ look how tiny Rogue data space is! :)
char * _lowmem = dummy;  /* Adresss of first save-able memory */
char * _Uend = dummy + sizeof(dummy);  /* Address of end of user data space */

/*@
 * Addresses used to save and restore dynamically allocated vars in heap,
 * the ones allocated via newmem(), which used sbrk().
 * Originally externs set by init.c on init_ds()
 *
 * end_sb was the first newmem() call, so its address actually marked the begin
 * the heap. startmem was the first call newmem() to the temporary printing
 * buffers, so its address marked the end heap data that was worth saving, as
 * printing buffers were discarded and not saved or restored in save files.
 *
 * If you think names are reversed, ask someone that truly understands x86
 * 16-bit real mode segmented memory, DS, heap and stack. Cos I surely don't ;)
 * See notes on restore()
 */
char dummier[4321];  //@ Also tiny :)
char *end_sb = dummier;  /* Pointer to the end of static base */
char *startmem = dummier + sizeof(dummier);  /* Pointer to the start of static memory */


#define MIDSIZE 10
char *msaveid = "AI Design";

/*
 * BLKSZ: size of block read/written on each io operation
 *        Has to be less than 4096 and a factor of 4096
 *        so the screen can be read in exactly.
 */
#define BLKSZ 512

/*
 * save_game:
 *	Implement the "save game" command
 */
void
save_game()
{
#ifndef DEMO
	register int retcode;
	char savename[20];

	msg("");
	mpos = 0;
	if (terse)
		addstr("Save file ? ");
	else
		printw("Save file (press enter (\x11\xd9) to default to \"%s\") ? ",
				s_save );
	retcode = getinfo(savename,19);
	if (*savename == 0)
		strcpy(savename,s_save);
	msg("");
	mpos = 0;
	if (retcode != ESCAPE)
	{
		if ((retcode = save_ds(savename)) == -1)
		{
			if (remove(savename) == 0)
				ifterse1("out of space?","out of space, can not write %s",savename);
			msg("Sorry, you can't save the game just now");
			is_saved = FALSE;
		}
		else if (retcode > 0)
			fatal("\nGame saved as %s.", savename);
	}
#endif
}
#ifndef DEMO
/*
 *  Save:
 * 		Determine the entire data area that needs to be saved,
 *		Open save file, first write in to save file a header
 *		that demensions the data area that will be saved,
 *		and then dump data area determined previous to opening
 *		file.
 */
int
save_ds(savename)
	char *savename;
{
	register FILE *file;
	register char answer;

	if ((file = fopen(savename, "r")) != NULL)
	{
		fclose(file);
		msg("%s %sexists, overwrite (y/n) ?",savename,noterse("already "));
		answer = readchar();
		msg("");
		if ((answer != 'y') && (answer != 'Y'))
			return(-2);
	}

	if ((file = fopen(savename, "w")) == NULL)
	{
		msg("Could not create %s",savename);
		return (-2);
	}
	is_saved = TRUE;
	mpos = 0;

	errno = 1;
	if ( ! fwrite(msaveid, MIDSIZE, 1, file)
		|| ! fwrite(&_lowmem , &_Uend - &_lowmem, 1, file)
			|| ! fwrite(end_sb, startmem - end_sb, 1, file))
			goto wr_err;
	/*
	 * save the screen (have to bring it into current data segment first)
	 */
	wdump();
	if (fwrite(savewin, 4000, 1, file))
		errno = 0;
	wrestor();

wr_err:
	fclose(file);
	switch (errno)
	{
		default:
			msg("Could not write savefile to disk!");
			return -1;
		case 0:
			move(24,0);
			clrtoeol();
			move(23,0);
			return 1;
	}
}
#endif //DEMO

/*
 *	Restore:
 *		Open saved data file, read in header, and determine how much
 *		data area is going to be restored,
 *		Close save data file,
 *		Allocate enough data space so that open data file information
 *		will be stored outside the data area that will be restored,
 *		Now reopen data save file,
 *		skip header,
 *		dump into memory all saved data.
 */
void
restore(savefile)
	char *savefile;
{
#ifndef DEMO
	int oldrev, oldver, old_check;
	register int oldcols;
	register FILE *file;
	char errbuf[11], save_name[MAXSTR];
	char *read_error = "Read Error";
	struct sw_regs *oregs;
	unsigned nbytes;
	char idbuf[MIDSIZE];

	oregs = regs;
	winit();
	if (bwflag)
		forcebw();
	if (no_check == 0)
		no_check = do_force;
	old_check = no_check;
	strcpy(errbuf,read_error);
	/*
	 * save things that will be bombed on when the
	 * restor takes place
	 */
	oldrev = revno;
	oldver = verno;

	if (!strcmp(s_drive,"?"))
	{
		int ot = scr_type;
		printw("Press space to restart game");
		scr_type = -1;
		wait_for(' ');
		scr_type = ot;
		addstr("\n");
	}
	if ((file = fopen(savefile, "r")) == NULL)
		fatal("%s not found\n",savefile);
	else
		printw("Restoring %s",savefile);
	strcpy(save_name, savefile);
	nbytes = &_Uend - &_lowmem;
	if (fread(idbuf, MIDSIZE, 1, file) || strcmp(idbuf,msaveid) )
		addstr("\nNot a savefile\n");
	else
	{
		if (fread(&_lowmem, nbytes, 1, file))
			if (fread(end_sb, startmem - end_sb, 1, file))
				goto rok;
		addstr(errbuf);
	}
	fclose(file);
	md_exit(EXIT_FAILURE);

rok:
	regs = oregs;
	if (revno != oldrev || verno != oldver)
	{
		fclose(file);
		md_exit(EXIT_FAILURE);
	}

	oldcols = COLS;
	//@ no longer needed, memory is now managed via malloc()
	//@ brk(end_sb);					/* Restore heap to empty state */
	init_ds();
	/*@
	 * There is a very clever trick going on here: by resetting the heap with
	 * brk(end_sb) and immediately calling init_ds() it guarantees that
	 * init_ds() will "re-allocate" the same memory area that was just written
	 * by the game restore, as long as end_sb was the first call to newmem().
	 *
	 * I just wonder how Rogue made sure it was safe to "blindly" write many
	 * KBs of data starting at end_sb address before that area was "properly"
	 * allocated via newmem()/sbrk(). Perhaps it didn't, but it just worked.
	 * Ah, the wonders of real mode :)
	 */
	endwin();
	winit();
	if (oldcols != COLS)
	{
		fclose(file);
		fatal("Restore Error: new screen size\n");
	}

	wdump();
	if (!fread(savewin, 4000, 1, file))
	{
		fclose(file);
		fatal("Serious restore error");
	}
	wrestor();

	fclose(file);
	no_check = old_check;
	mpos = 0;
	ifterse1("%s, Welcome back!","Hello %s, Welcome back to the Dungeons of Doom!",whoami);
	dnum = srand();     /* make it a little tougher on cheaters */
	remove(save_name);
#endif //DEMO
}
