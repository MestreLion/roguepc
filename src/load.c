/*
 * This file contains the code to display the rogue picture files
 *
 * load.c	1.42	(A.I. Design)	2/12/84
 */

#include	"rogue.h"
#include	"curses.h"

/*@
 * References for 3D8h and 3D9h:
 * http://minuszerodegrees.net/5150_5160/cards/5150_5160_cards.htm#cga
 * http://www.scribd.com/doc/251557562/CGA-IBM-Color-Graphics-Monitor-Adapter
 * http://www.seasip.info/VintagePC/cga.html
 */

/*@
 * 3D8h = CGA Select Mode port, 6-bits, write only:
 * Bit 0 - 0=40x25, 1=80x25 (for text mode, ignored if bit 1 = 0)
 * Bit 1 - 0=Text,  1=320x200 Graphics
 * Bit 2 - 0=Color, 1=Mono, or "burst color" for 3rd palette when in 320x200 **
 * Bit 3 - 1=(re-)enables video signal, which is disabled when mode changes
 * Bit 4 - 1=640x200 mono graphics mode
 * Bit 5 - 1=Enable blink attribute for text modes
 *
 * ** When in 320x200 mode, Bit 2 acts as "burst color" bit that activates
 *    an undocumented 3d palette (Default/Cyan/Red/Magenta)
 */
#define MODEREG    0x3d8  //@ CGA Select Mode port address
#define BWENABLE   0x004  //@ Bit 2 mask, for BW mode / 3rd palette burst bit
#define MODESAVE   0x065  //@ BIOS Data Area 40:65: current value of 3D8h port

/*@
 * 3D9h = CGA Select Color port, 6-bits, write only. For mode 4 (320x200x4c):
 * Bit 0 - Background/Default color, Blue  component
 * Bit 1 - Background/Default color, Green component
 * Bit 2 - Background/Default color, Red   component
 * Bit 3 - Intensified background color
 * Bit 4 - Alternate, intensified set of colors (the "i" palette variation)
 * Bit 5 - Active color set (Palette 0 or 1)
 *         Palette 0: 0-Background/Default, 1-Red,  2-Green,   3-Yellow
 *         Palette 1: 0-Background/Default, 1-Cyan, 2-Magenta, 3-White
 */
#define COLREG     0x3d9  //@ CGA Select Color port address
#define HIGHENABLE 0x010  //@ Bit 4 mask, for intensified palette
#define PALETTEBIT 0x020  //@ Bit 5 mask, unused

static void	scr_load(void);
static void	bload(unsigned int segment);

//@ temp buffer to hold image file bytes
static char *store;

/*@
 * block size used when reading image file
 * A packed CGA 320x200x2bit image (4 colors) requires 16384 bytes:
 *   16000 total of pixel data + 2 x 192-byte padding
 * Initial requested block size, 0x4000=16384, is enough to read file in 1 pass
 */
static int blksize = 0x4000;
static FILE *file;

/*@
 * Display the Rogue Enyx title image
 * - Set video mode to 320x200, 4 colors (CGA),
 * - Load 'rogue.pic' and display it for about 5 minutes
 *   or until a key is pressed,
 * - Return to previous video mode
 */
void
epyx_yuck(void)
{
	register int type = get_mode();

	//@ 07h = Monochromatic (80x25 text) in MDA, Hercules, EGA, VGA
	if (type == 7 || (file = fopen("rogue.pic", "r")) == NULL)
		return;
	//@ Allocate the largest possible block size for the store buffer,
	//@ halving the requested amount in each attempt
	while ((store = malloc(blksize)) == NULL)
		blksize /= 2;
	//@ 04h = Graphics mode 320x200 4 colors in CGA,PCjr,EGA,MCGA,VGA
	video_mode(4);

	scr_load();
	fclose(file);
#ifdef LOGFILE
	//@ originally a busy loop of 18 * 10 ticks
	sleep(10);
#else
	/*@
	 *  Blocking timeout mode does not work with standard ncurses, as the
	 *  underlying functions wtimeout() / wget_wch() only work properly after
	 *  curses initialization with initscr(), done later in main() by calling
	 *  winit(). As it is, it's non-blocking and returns immediately.
	 *  Not an issue considering the whole image display is dummy as there is no
	 *  (portable) way to switch to CGA graphics mode in standard C in 2020.
	 *
	 *  Originally a busy loop of 18 * 60 * 5 ticks with no_char() shortcut.
	 */
	getch_timeout(1000 * 60 * 5);
#endif  // LOGFILE
	video_mode(type); //@ restore previous mode
	free(store);
}

/*@
 * Load the file bytes to video memory
 * and adjust the video flags
 *
 * PIC/BSAVE format documentation
 * http://www.fileformat.info/format/pictor/egff.htm#PICTOR-DMYID.3.1.2
 * http://www.shikadi.net/moddingwiki/PIC_Format
 * http://www.shikadi.net/moddingwiki/Raw_CGA_Data#Interlaced_CGA_Data
 * http://en.wikipedia.org/wiki/BSAVE_%28bitmap_format%29#Graphics
 *
 * 'ROGUE.PIC', 16391 bytes, contains:
 * - BSAVE header - 7 bytes
 * - CGA Interlaced data, 2 blocks, even lines and odd lines. Each block is:
 *   - 8000 bytes of image
 *   -  192 bytes of padding
 * - (it does *not* contain the trailer byte 1Ah (CPM EOF)
 *
 *    7 bytes BSAVE header, ignored by bload():
 *  BYTE Marker         Data type                  FDh = unpacked data
 *  WORD ScreenSegment  PC screen memory segment B800h = CGA video address
 *  WORD ScreenOffset   PC screen memory offset  0000h = no offset
 *  WORD DataSize       Size of screen data      4000h = 16384, 320x200x2bit
 *
 * 8000 bytes of image data for even rows
 *   BYTE ColorsX4      4 pixels per byte, each a 2-bit color. In palette 1i:
 *     00 Default (may select one out of 16 CGA colors, Black by default)
 *     01 Cyan
 *     02 Magenta
 *     11 White
 *
 * 192 bytes of padding, but PC Paint also stores metadata in this first block
 *    12B Signature     Editor used to create    'PCPaint V1.0'
 *   BYTE PaletteID     Current Palette number    05h = Palette 1i (Why?)
 *   BYTE BackColor     Color of Default(index 0) 00h = Black
 *   178B Padding       Padding                   55h x 178
 *
 * 8000 bytes of image data for odd rows
 *   Same format as even rows
 *
 * 192 bytes of padding
 *   192B Padding       Padding                   55h x 192
*/
static
void
scr_load(void)
{
	int palette, background;
	int mode, burst;

	//@ Write the file. 0xb800 = Video memory address for CGA mode 04h
	bload(0xb800);

	/*@
	 * read image palette and bgcolor from CGA memory that was just written
	 * with the file data. Offsets 8012 and 8013 are in the first 192-byte
	 * padding block, right after the 'PCPaint V1.0' signature
	 */
	palette = peekb(8012,0xB800);     //@ 5 = CGA palette 1i, see below
	background = peekb(8013,0xB800);  //@ 0 = Color index 0 (BG) is Black

	//@ Intensified palette, enable bit 4 for the COLREG write
	if (palette >= 3)
		background |= HIGHENABLE;

	/*@
	 * Not sure why all this switch cases and palette remapping
	 * if in the end the palette just sets the burst bit and is
	 * not used anymore. mode is read from BIOS Data Area and re-applied
	 * with bust enabled. In any case, modifications to the corresponding
	 * bytes in ROGUE.PIC does not seem to have any effect on display
	 * in Epyx v1.49 (using DOSBox), so this code might have changed there.
	 */
	burst   = 0;
	switch(palette)
	{
	case 2:
	case 5:
		burst = 1;
		/* fallthrough */
	case 0:
	case 3:
		palette = 1;  //@ ignored
		break;
	case 1:
	case 4:
		palette = 0;  //@ ignored
		break;
	}

	//@ Set background color and palette intensity
	out (COLREG,background);

	//@ Read current video mode from BIOS Data Area, sans the burst bit
	mode = peekb(MODESAVE,0x40) & (~BWENABLE);
	if (burst == 1)
		mode = mode | BWENABLE;  //@ enable burst bit
	pokeb(MODESAVE,0x40,mode);   //@ write new mode to BIOS Data Area
	out(MODEREG,mode);           //@ write mode to CGA 6845 controller
}

/*@
 * Load the file bytes into a memory segment
 */
static
void
bload(unsigned int segment)
{
	register unsigned offset = 0, rdcnt;

	if (!fread(store, 7, 1, file))	/* Ignore first seven bytes */
		fseek(file, 7L, SEEK_SET);
	while ((rdcnt = fread(store, blksize, 1, file))) {
		dmaout(store,rdcnt/2,segment,offset);
		if ((offset += rdcnt) >= 16384)
			break;
	}
}

/*@
 * Find the drive where game files are located, only used for copy protection.
 *
 * Return an int corresponding to the current drive or the "drive" value
 * from the fake environment (rogue.opt), where 0=A, 1=B, etc
 *
 * Also contained a no-op code which checked the existence of a file named
 * "jatgnas.8ys" in the root of such drive, but ignored the check result.
 *
 * Btw... what is this function doing here?
 */
int
find_drive(void)
{
#ifdef ROGUE_DOS_DRIVE
	int drive = bdos(0x19);  //@ Get Current Default Drive (0=A, 1=B, etc)
#else
	int drive = current_drive;
#endif
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
