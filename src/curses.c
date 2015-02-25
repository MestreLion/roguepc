/*
 *  Cursor motion stuff to simulate a "no refresh" version of curses
 */
#include	"rogue.h"
#include	"curses.h"

/*
 *  Globals for curses
 */
int LINES=25, COLS=80;
int is_saved = FALSE;
bool iscuron = TRUE;
int ch_attr = 0x7;
int old_page_no;
int no_check = FALSE;
int scr_ds=0xB800;
int svwin_ds;
int scr_type = -1;
int page_no = 0;
char savewin[4096];

int tab_size = 8;  //@ unused

#define MAXATTR 17
byte color_attr[] = {
	7,  /*  0 normal         */
	2,  /*  1 green          */
	3,  /*  2 cyan           */
	4,  /*  3 red            */
	5,  /*  4 magenta        */
	6,  /*  5 brown          */
	8,  /*  6 dark grey      */
	9,  /*  7 light blue     */
	10, /*  8 light green    */
	12, /*  9 light red      */
	13, /* 10 light magenta  */
	14, /* 11 yellow         */
	15, /* 12 uline          */
	1,  /* 13 blue           */
	112,/* 14 reverse        */
	15, /* 15 high intensity */
   112, /* bold				 */
	0   /* no more           */
} ;

byte monoc_attr[] = {
	  7,  /*  0 normal         */
	 7,  /*  1 green          */
	 7,  /*  2 cyan           */
	 7,  /*  3 red            */
	 7,  /*  4 magenta        */
	 7,  /*  5 brown          */
	 7,  /*  6 dark grey      */
	 7,  /*  7 light blue     */
	 7,  /*  8 light green    */
	 7,  /*  9 light red      */
	 7,  /* 10 light magenta  */
	 7,  /* 11 yellow         */
	 17,  /* 12 uline          */
	 7,  /* 13 blue           */
	120,  /* 14 reverse        */
	 7,  /* 15 white/hight    */
	120,  /* 16 bold		   */
	0     /* no more           */
} ;

byte *at_table;

int c_row, c_col;   /*  Save cursor positions so we don't ask dos */
int scr_row[25];

byte dbl_box[BX_SIZE] = {
	0xc9, 0xbb, 0xc8, 0xbc, 0xba, 0xcd, 0xcd
};

byte sng_box[BX_SIZE] = {
	0xda, 0xbf, 0xc0, 0xd9, 0xb3, 0xc4, 0xc4
};

byte fat_box[BX_SIZE] = {
	0xdb, 0xdb, 0xdb, 0xdb, 0xdb, 0xdf, 0xdc
};

byte spc_box[BX_SIZE] = {
	0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20
};

/*@
 * Beep a an audible beep, if possible
 *
 * To be replaced by ncurses beep()
 *
 * Originally in dos.asm
 *
 * Used hardware port 0x61 (Keyboard Controller) for direct PC Speaker access.
 * Now prints a BEL (0x07) character in standard output, which is supposed to
 * make terminals play a beep.
 */
void
beep()
{
	printf("\a");  //@ lame, I know... but it works
}


/*@
 * Move the cursor to the given row and column
 *
 * To be replaced by ncurses move()
 *
 * Originally in zoom.asm
 *
 * As per original code, also updates C global variables c_col_ and c_row,
 * used as "cache" by curses. See getrc(). Actual cursor movement is only
 * performed if iscuron is set. See cursor().
 *
 * Used BIOS INT 10h/AH=02h to set the cursor on C variable page_no.
 * The BIOS call is now performed via swint()
 *
 * INT 10h/AH=02h - Set Cursor Position
 * BH = page number
 * DH = row
 * DL = col
 */
void
move(row, col)
	int row;
	int col;
{
	c_row = row;
	c_col = col;

	if (iscuron)
	{
		regs->ax = HIGH(2);
		regs->bx = HIGH(page_no);
		regs->dx = HILO(row, col);
		swint(SW_SCR, regs);
	}
}


/*@
 * Put the given character on the screen
 *
 * Character is put at current (c_row, c_col) cursor position, and set with
 * current ch_attr attributes.
 *
 * No direct ncurses replacement.
 *
 * <stdio.h> putchar() is a replacement candidate, but it lacks the character
 * attributes. Anyway, this will probably be deprecated after addch()
 * is replaced, as addch() is putchr() main client. Also see credits() and
 * backspace().
 *
 * Originally in zoom.asm
 *
 * By my understanding, asm function works as follows: if cursor is on (via
 * iscuron C var), it invokes BIOS INT 10h/AH=09h to put char with attributes.
 * If not, it waits for video retrace (unless no_check C var was TRUE) and
 * then write directly in Video Memory, using C vars scr_row and scr_ds to
 * calculate position address.
 *
 * This function replicates this behavior using dmaout() and swint().
 *
 * BIOS INT 10h/AH=09h - Write character with attribute at cursor position
 * AL = character
 * BH = page number
 * BL = character attribute
 * CX = number of times to write character
 *
 */
void
putchr(ch)
	byte ch;
{
	if (iscuron)
	{
		// Use BIOS call
		regs->ax = HILO(9, ch);
		regs->bx = HILO(page_no, ch_attr);
		regs->cx = 1;
		swint(SW_SCR, regs);
	}
	else
	{
		if (!no_check){;}  // "wait" for video retrace
		/*
		 * Write to video memory
		 * Each char uses 2 bytes in video memory, hence doubling c_col.
		 * scr_row[] array takes that into account, so we can use c_row
		 * directly. See winit().
		 * Also, I *think* dmaout() length assumes 16-bit data, the size of int
		 * in DOS, hence 1 for writing 2 bytes.
		 */
		dmaout(HILO(ch_attr, ch), 1,
			scr_ds, scr_row[c_row] + 2 * c_col);
	}
#ifdef ROGUE_DEBUG
	printf("%c", ch);
#endif
}


/*@
 * Return character and attribute at current cursor position
 *
 * To be replaced by ncurses inch()
 *
 * Originally in zoom.asm
 *
 * By my understanding, asm function works very similar to putchr():
 * If iscuron, invokes BIOS INT 10h/AH=02h to set cursor position from cache
 * and 10h/AH=08h to read character, else wait retrace (unless no_check) and
 * read directly from Video Memory. The set cursor BIOS call seems quite
 * redundant, as virtually all calls to curch(), from both the inch() macro and
 * the mvinch() wrapper, are preceded by a move().
 *
 * This function replicates this behavior, except the redundant move.
 *
 * BIOS INT 10h/AH=08h - Read character and attribute at cursor position
 * BH = page number
 * Return:
 * AH = attribute
 * AL = character
 *
 */
int
curch()
{
	byte chr;
	byte attr;
	int offset;

	if (iscuron)
	{
		regs->ax = HIGH(8);
		regs->bx = HIGH(page_no);
		return swint(SW_SCR, regs);
	}
	else
	{
		if (!no_check){;}
		/*
		 * I feel dumb for not knowing the 16-bit equivalent of peekb(), but
		 * at least it seems scr_load() doesn't either ;)
		 * Maybe dmain() would be better?
		 */
		offset = scr_row[c_row] + 2 * c_col;
		chr  = peekb(scr_ds, offset++);
		attr = peekb(scr_ds, offset);
		return HILO(attr, chr);
	}
}


/*@
 * Fills a buffer with "extended" chars (char + attribute)
 *
 * This is the n-bytes version of setmem(). There is no POSIX or ncurses
 * direct replacement other than a loop writing multiple bytes at a time.
 *
 * For DOS compatibility, chtype (and by proxy wsetmem()) is currently set to
 * operate on 16-bit words. But chtype in actual <ncurses.h> may be set to
 * a whooping 64-byte unsigned long, so make *really* sure buffer size and
 * count argument are consistent with chtype size!
 *
 * This function could be generic enough to be in mach_dep.c, but it was only
 * by curses.c to write a character + attribute to window buffer.
 *
 * Originally in dos.asm
 */
void
wsetmem(buffer, count, attrchar)
	void *buffer;
	int count;
	chtype attrchar;  // enforced to prevent misuse
{
	while (count--)
		((chtype *)buffer)[count] = (chtype)attrchar;
}

/*
 * clear screen
 */
void
clear()
{
	if (scr_ds == svwin_ds)
		wsetmem(savewin, LINES*COLS, 0x0720);
	else
		blot_out(0,0,LINES-1,COLS-1);
}


/*
 *  Turn cursor on and off
 */
bool
cursor(ison)
	bool ison;
{
	register bool oldstate;

	if (iscuron == ison)
		return ison;
	oldstate = iscuron;
	iscuron = ison;

	regs->ax = 0x100;
	if (ison)
	{
		regs->cx = (is_color ? 0x607 : 0xb0c);
		swint(SW_SCR, regs);
		move(c_row, c_col);
	}
	else
	{
		regs->cx = 0xf00;
		swint(SW_SCR, regs);
	}
	return(oldstate);
}


/*
 * get curent cursor position
 */
void
getrc(rp,cp)
	int *rp, *cp;
{
	*rp = c_row;
	*cp = c_col;
}

void
real_rc(pn, rp,cp)
	int pn, *rp, *cp;
{
	/*
	 * pc bios: read current cursor position
	 */
	regs->ax = 0x300;
	regs->bx = pn << 8;

	swint(SW_SCR, regs);

	*rp = regs->dx >> 8;
	*cp = regs->dx & 0xff;
}

/*
 *	clrtoeol
 */
void
clrtoeol()
{
	int r,c;

	if (scr_ds == svwin_ds)
		return;
	getrc(&r,&c);
	blot_out(r,c,r,COLS-1);
}

void
mvaddstr(r,c,s)
	int r,c;
	char *s;
{
	move(r, c);
	addstr(s);
}

void
mvaddch(r,c,chr)
	int r, c;
	char chr;
{
	move(r, c);
	addch(chr);
}

byte
mvinch(r, c)
	int r, c;
{
	move(r, c);
	return(curch()&0xff);
}

/*
 * put the character on the screen and update the
 * character position
 */
int
addch(chr)
	byte chr;
{
	int r, c;
	byte old_attr;

	old_attr = ch_attr;

	if (at_table == color_attr)
	{
		/* if it is inside a room */
		if (ch_attr == 7)
		switch(chr)
		{
			case DOOR:
			case VWALL:
			case HWALL:
			case ULWALL:
			case URWALL:
			case LLWALL:
			case LRWALL:
				ch_attr = 6;  /* brown */
				break;
			case FLOOR:
				ch_attr = 10;  /* light green */
				break;
			case STAIRS:
				ch_attr = 160; /* black on green*/
				break;
			case TRAP:
				ch_attr = 5;  /* magenta */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = 14;  /* yellow */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = 9;
				break;
			case FOOD:
				ch_attr = 4;
				break;
		}
		/* if inside a passage or a maze */
		else if (ch_attr == 112)
		switch(chr)
		{
			case FOOD:
				ch_attr = 116;   /* red */
				break;
			case GOLD:
			case PLAYER:
				ch_attr = 126;  /* yellow on white */
				break;
			case POTION:
			case SCROLL:
			case STICK:
			case ARMOR:
			case AMULET:
			case RING:
			case WEAPON:
				ch_attr = 113;    /* blue on white */
				break;
		}
		else if (ch_attr == 15 && chr == STAIRS)
			ch_attr = 160;
	}

	getrc(&r,&c);
	if (chr == '\n') {
		if (r == LINES-1) {
			scroll_up(0, LINES-1, 1);
			move(LINES-1, 0);
		} else
			move(r+1, 0);
			ch_attr = old_attr;
		return c_row;
	}
	putchr(chr);
	move(r,c+1);
	ch_attr = old_attr;
	/*
	 * if you have gone of the screen scroll the whole window
	 */
	return(c_row);
}

void
addstr(s)
	char *s;
{
	while(*s)
		addch(*s++);
#ifdef ROGUE_DEBUG
	printf("\n");
#endif
}

void
set_attr(bute)
	int bute;
{
	if (bute < MAXATTR)
		ch_attr = at_table[bute];
	else
		ch_attr = bute;
}

void
error(mline,msg,a1,a2,a3,a4,a5)
	int mline;
	char *msg;
	int a1,a2,a3,a4,a5;
{
	int row, col;

	getrc(&row,&col);
	move(mline,0);
	clrtoeol();
	printw(msg,a1,a2,a3,a4,a5);
	move(row,col);
}

//@ unused, and already stubbed in original
/*
 * Called when rogue runs to move our cursor to be where DOS thinks
 * the cursor is
 */
void
set_cursor()
{
/*
	regs->ax = 15 << 8;
	swint(SW_SCR, regs);
	real_rc(regs->bx >> 8, &c_row, &c_col);
*/
}

/*
 *  winit(win_name):
 *		initialize window -- open disk window
 *						  -- determine type of moniter
 *						  -- determine screen memory location for dma
 */
void
winit()
{
	register int i, cnt;

	/*
	 * Get monitor type
	 */
	regs->ax = 15 << 8;
	swint(SW_SCR, regs);
	old_page_no = regs->bx >> 8;
	scr_type = regs->ax = 0xff & regs->ax;
	/*
	 * initialization is any good because restarting game
	 * has old values!!!
	 * So reassign defaults
	 */
	LINES   =  25;
	COLS    =  80;
	scr_ds  =  0xB800;
	at_table = monoc_attr;

	switch (scr_type) {
		/*
		 *  It is a TV
		 */
		case 1:
			at_table = color_attr;
			/* no break */
		case 0:
			COLS = 40;
			break;

		/*
		 * Its a high resolution monitor
		 */
		case 3:
			at_table = color_attr;
			/* no break */
		case 2:
			break;
		case 7:
			scr_ds = 0xB000;
			no_check = TRUE;
			break;
		/*
		 * Just to save text space lets eliminate these
		 *
		case 4:
		case 5:
		case 6:
			move(24,0);
			fatal("Program can't be run in graphics mode");
		 */
		default:
			move(24,0);
			fatal("Unknown screen type (%d)",regs->ax);
			break;
	}
	/*
	 * Read current cursor position
	 */
	real_rc(old_page_no, &c_row, &c_col);
	/*@ savewin is now a fixed size array.
	if ((savewin = sbrk(4096)) == (void *)-1) {
		svwin_ds = -1;
		savewin = (char *) _flags;
		if (scr_type == 7)
			fatal(no_mem);
	} else {
		savewin = (char *) (((intptr) savewin + 0xf) & 0xfff0);
		svwin_ds = (((intptr) savewin >> 4) & 0xfff) + _dsval;
	}
	*/
	svwin_ds = (((intptr) savewin >> 4) & 0xfff) + _dsval;

	for (i = 0, cnt = 0; i < 25; cnt += 2*COLS, i++)
		scr_row[i] = cnt;
	//@ newmem(2);  // no longer need memory alignment
	switch_page(3);
	if (old_page_no != page_no)
		clear();
	move(c_row, c_col);
	if (isjr())
		no_check = TRUE;
}

void
forcebw()
{
	at_table = monoc_attr;
}

/*
 *  wdump(windex)
 *		dump the screen off to disk, the window is save so that
 *		it can be retieved using windex
 */
void
wdump()
{
	sav_win();
	dmain(savewin,LINES*COLS,scr_ds,0);
	is_saved = TRUE;
}

char *
sav_win()
{
	if (savewin == (char *)_flags)
		dmaout(savewin,LINES*COLS,0xb800,8192);
	return(savewin);
}

void
res_win()
{
	if (savewin == (char *)_flags)
		dmain(savewin,LINES*COLS,0xb800,8192);
}

/*
 *	wrestor(windex):
 *		restor the window saved on disk
 */
void
wrestor()
{
	dmaout(savewin,LINES*COLS,scr_ds,0);
	res_win();
	is_saved = FALSE;
}

/*
 * wclose()
 *   close the window file
 */
void
wclose()
{

	/*
	 * Restor cursor (really you want to restor video state, but be carefull)
	 */
	if (scr_type >= 0)
		cursor(TRUE);
	if (page_no != old_page_no)
		switch_page(old_page_no);
}

/*
 *  Some general drawing routines
 */

void
box(ul_r, ul_c, lr_r, lr_c)
{
	vbox(dbl_box, ul_r, ul_c, lr_r, lr_c);
}

/*
 *  box:  draw a box using given the
 *        upper left coordinate and the lower right
 */
void
vbox(box, ul_r,ul_c,lr_r,lr_c)
	byte box[BX_SIZE];
	int ul_r,ul_c,lr_r,lr_c;
{
	register int i, wason;
	int r,c;

	wason = cursor(FALSE);
	getrc(&r,&c);

	/*
	 * draw horizontal boundry
	 */
	move(ul_r, ul_c+1);
	repchr(box[BX_HT], i = (lr_c - ul_c - 1));
	move(lr_r, ul_c+1);
	repchr(box[BX_HB], i);
	/*
	 * draw vertical boundry
	 */
	for (i=ul_r+1;i<lr_r;i++) {
		mvaddch(i,ul_c,box[BX_VW]);
		mvaddch(i,lr_c,box[BX_VW]);
	}
	/*
	 * draw corners
	 */
	mvaddch(ul_r,ul_c,box[BX_UL]);
	mvaddch(ul_r,lr_c,box[BX_UR]);
	mvaddch(lr_r,ul_c,box[BX_LL]);
	mvaddch(lr_r,lr_c,box[BX_LR]);

	move(r,c);
	cursor(wason);
}

/*
 * center a string according to how many columns there really are
 */
void
center(row,string)
	int row;
	char *string;
{
	mvaddstr(row,(COLS-strlen(string))/2,string);
}


/*
 * printw(Ieeeee)
 */
void
printw(msg,a1,a2,a3,a4,a5,a6,a7,a8)
	char *msg;
	int a1, a2, a3, a4, a5, a6, a7, a8;
{
	char pwbuf[132];
	sprintf(pwbuf,msg,a1,a2,a3,a4,a5,a6,a7,a8);
	addstr(pwbuf);
}

void
scroll_up(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	regs->ax = 0x600 + nlines;
	regs->bx = 0x700;
	regs->cx = start_row << 8;
	regs->dx = (end_row << 8) + COLS - 1;
	swint(SW_SCR,regs);
	move(end_row,c_col);
}

void
scroll_dn(start_row,end_row,nlines)
	int start_row,end_row,nlines;
{
	regs->ax = 0x700 + nlines;
	regs->bx = 0x700;
	regs->cx = start_row << 8;
	regs->dx = (end_row << 8) + COLS - 1;
	swint(SW_SCR,regs);
	move(start_row,c_col);
}

void
scroll()
{
	scroll_up(0,24,1);
}



/*
 * blot_out region
 *    (upper left row, upper left column)
 *	  (lower right row, lower right column)
 */
void
blot_out(ul_row,ul_col,lr_row,lr_col)
{
	regs->ax = 0x600;
	regs->bx = 0x700;
	regs->cx = (ul_row<<8) + ul_col;
	regs->dx = (lr_row<<8) + lr_col;
	swint(SW_SCR,regs);
	move(ul_row,ul_col);
}

void
repchr(chr,cnt)
	int chr, cnt;
{
	while(cnt-- > 0) {
		putchr(chr);
		c_col++;
	}
}

/*
 * try to fixup screen after we get a control break
 */
void
fixup()
{
	blot_out(c_row,c_col,c_row,c_col+1);
}

/*
 * Clear the screen in an interesting fashion
 */

void
implode()
{
	int j, delay, r, c, cinc = COLS/10/2, er, ec;

	er = (COLS == 80 ? LINES-3 : LINES-4);
	/*
	 * If the curtain is down, just clear the memory
	 */
	if (scr_ds == svwin_ds) {
		wsetmem(savewin, (er + 1) * COLS, 0x0720);
		return;
	}
	delay = scr_type == 7 ? 500 : 10;
	for (r = 0,c = 0,ec = COLS-1; r < 10; r++,c += cinc,er--,ec -= cinc) {
		vbox(sng_box, r, c, er, ec);
		for (j = delay; j--; )
			;
		for (j = r+1; j <= er-1; j++) {
			move(j, c+1); repchr(' ', cinc-1);
			move(j, ec-cinc+1); repchr(' ', cinc-1);
		}
		vbox(spc_box, r, c, er, ec);
	}
}

/*
 * drop_curtain:
 *	Close a door on the screen and redirect output to the temporary buffer
 */
static int old_ds;
void
drop_curtain()
{
	register int r, j, delay;

	if (svwin_ds == -1)
		return;
	old_ds = scr_ds;
	dmain(savewin, LINES * COLS, scr_ds, 0);
	cursor(FALSE);
	delay = (scr_type == 7 ? 3000 : 2000);
	green();
	vbox(sng_box, 0, 0, LINES-1, COLS-1);
	yellow();
	for (r = 1; r < LINES-1; r++) {
		move(r, 1);
		repchr(0xb1, COLS-2);
		for (j = delay; j--; )
			;
	}
	scr_ds = svwin_ds;
	move(0,0);
	standend();
}

void
raise_curtain()
{
	register int i, j, o, delay;

	if (svwin_ds == -1)
		return;
	scr_ds = old_ds;
	delay = (scr_type == 7 ? 3000 : 2000);
	for (i = 0, o = (LINES-1)*COLS*2; i < LINES; i++, o -= COLS*2) {
		dmaout(savewin + o, COLS, scr_ds, o);
		for (j = delay; j--; )
			;
	}
}

void
switch_page(pn)
{
	register int pgsize;

	if (scr_type == 7) {
		page_no = 0;
		return;
	}
	if (COLS == 40)
		pgsize = 2048;
	else
		pgsize = 4096;
	regs->ax = 0x0500 | pn;
	swint(SW_SCR, regs);
	scr_ds = 0xb800 + ((pgsize * pn) >> 4);
	page_no = pn;
}

byte
get_mode()
/*@
 * Get current video mode using software interrupt 10h, AH=0Fh
 *
 * Response is:
 * AL = Video Mode, AH = number of character columns, BH = active page
 * https://en.wikipedia.org/wiki/INT_10H
 * http://www.ctyme.com/intr/rb-0108.htm
 *
 * Return only AL, the low A byte representing Video Mode, as a 16-bit int
 * For EGA text, AL is 03h for color or 07h for monochrome
 */
{
	struct sw_regs regs;

	regs.ax = 0xF00;  //@ AH = 0Fh
	swint(SW_SCR,&regs);
	return 0xff & regs.ax;
}

/*@
 * Set current video mode using software interrupt 10h, AH=00h
 *
 * Response is:
 * AL = Video Mode or CRT controller
 * http://www.ctyme.com/intr/rb-0069.htm
 *
 * Return AL
 */
byte
video_mode(type)
{
	struct sw_regs regs;

	regs.ax = type;
	swint(SW_SCR,&regs);
	return regs.ax;
}
