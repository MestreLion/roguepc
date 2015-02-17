/*
 * This file contains the code to display the rogue picture files
 *
 * load.c	1.42	(A.I. Design)	2/12/84
 */

#include	"rogue.h"
#include	"curses.h"

/*@
 * References for 3D8h and 3D9h:
 * http://www.scribd.com/doc/251557562/CGA-IBM-Color-Graphics-Monitor-Adapter
 * http://www.seasip.info/VintagePC/cga.html
 */

/*@
 * 3D8h = CGA Select Mode port, 6-bits, write only:
 * Bit 0 - 0=40x25, 1=80x25 (for text mode, ignored if bit 1=0)
 * Bit 1 - 0=Text,  1=320x200 Graphics
 * Bit 2 - 0=Color, 1=Mono, or "burst color" for 3rd palette when in 320x200 **
 * Bit 3 - 1=(re-)enables video signal, which is disabled when mode changes
 * Bit 4 - 1=640x200 mono graphics mode
 * Bit 5 - 1=Enable blink attribute for text modes
 *
 * * When in 320x200 mode, Bit 2 acts as "burst color" bit that activates
 *   an undocumented 3d palette (Default/Cyan/Red/Magenta)
 */
#define MODEREG    0x3d8  //@ CGA Select Mode port address
#define BWENABLE   0x004  //@ Bit 2 mask, for BW mode / 3rd palette burst bit
#define MODESAVE   0x065  //@ Offset for current mode value in BIOS Data 40h

/*@
 * 3D9h = CGA Select Color port, 6-bits, write only. For mode 4 (320x200x4):
 * Bit 0 - Blue background component
 * Bit 1 - Green background component
 * Bit 2 - Red background component
 * Bit 3 - Intensified background color
 * Bit 4 - Alternate, intensified set of colors (the "i" palette variation)
 * Bit 5 - Active color set (Palette 0 or 1)
 *         Palette 0: Defaut, Red, Green, Yellow
 *         Palette 1: Default, Cyan, Magenta, White
 */
#define COLREG     0x3d9  //@ CGA Select Color port address
#define HIGHENABLE 0x010  //@ Bit 4 mask, for intensified palette
#define PALETTEBIT 0x020  //@ Bit 5 mask, unused

//@ temp buffer to hold image file bytes
static char *store;

/*@
 * block size used when reading image file
 * A packed CGA 320x200x2bit image (4 colors) requires 16000 bytes + headers
 * initial block size, 0x4000 = 16384, is enough to read file in 1 pass
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
epyx_yuck()
{
	//@ clock rate is 65536/3600 per hour, about 18.2 ticks per second
	extern unsigned int tick;
	register int type = get_mode();

	//@ 07h = Monochromatic (80x25 text) in MDA, Hercules, EGA, VGA
	if (type == 7 || (file = fopen("rogue.pic", "r")) == NULL)
		return;
	//@ Allocate the largest possible block size for the store buffer,
	//@ halving the requested amount in each attempt
	while ((int) (store = sbrk(blksize)) == -1)
		blksize /= 2;
	//@ 04h = Graphics mode 320x200 4 colors in CGA,PCjr,EGA,MCGA,VGA
	video_mode(4);

	scr_load();
	fclose(file);
	tick = 0;
#ifdef LOGFILE
	while (tick < 18 * 10)
		;
#else
	while(no_char() && tick < 18 * 60 * 5)  //@ ~5 minutes
		;
	if (!no_char())
		readchar();
#endif
	video_mode(type);
	brk(store);
	tick = 0;
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
 * 'ROGUE.PIC' contains:
 * - BSAVE header
 * - CGA Interlaced data, 2 blocks, even lines and odd lines. Each block is:
 *   - 8000 bytes of image
 *   -  192 bytes of padding
 *
 * 7-byte header, ignored by bload()
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
 * 192 bytes of padding
 *   ?                  Signature                 'PCPaint V1.0'
 *   BYTE PaletteID     Current Palette number    05h = Palette 1i ?
 *   BYTE BackColor     Color for index 00        00h = Black
 *   *    Padding       Padding                   55h
 *
 * 8000 bytes of image data for odd rows
 *   Same format as even rows
 *
 * 192 bytes of padding
 *   *    Padding       Padding                   55h
*/
void
scr_load()
{
	int palette, background;
	int mode, burst;

	//@ 0xb800 = Video memory address for CGA mode 04h
	bload(0xb800);

	//@ read image palette and bgcolor from CGA memory, written inside padding
	palette = peekb(8012,0xB800);     //@ 5 = CGA palette 1i
	background = peekb(8013,0xB800);  //@ 0

	//@ Intensified palette, enable bit 4 for the COLREG write
	if (palette >= 3)
		background |= HIGHENABLE;

	/*@
	 * Not sure why all this switch cases and palette remapping
	 * if in the end palette is not used any more, and mode is read
	 * from BIOS. In any case, modifications to the corresponding
	 * bytes in ROGUE.PIC does not seem to have any effect on display
	 * in Epyx v1.49, so this code might have changed there.
	 */
	burst   = 0;
	switch(palette)
	{
	case 2:
	case 5:
		burst = 1;
		/* no break */
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
		mode = mode | BWENABLE;
	pokeb(MODESAVE,0x40,mode); //@ write mode to BIOS
	out(MODEREG,mode);         //@ write mode to CGA 6845 controller
}

/*@
 * Load the file bytes into a memory segment using DMA
 */
void
bload(segment)
	unsigned segment;
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
 * Find the drive where the game runs from
 *
 * Return an int corresponding to the current drive or the "drive" value
 * from the fake environment (rogue.opt), where 0=A, 1=B, etc
 *
 * Also contains a no-op code which checked the existence of a file named
 * "jatgnas.8ys" in the root of such drive, but ignored the check result.
 *
 */
int
find_drive()
{
	int drive = bdos(0x19);  //@ Get Current Default Drive (0=A, 1=B, etc)
	char spec = s_drive[0];
	char filename[30];

	if (isalpha(spec))
	{
		/*@
		 * It looks like this block could be replaced with:
		 * drive = tolower(spec) - 'a';
		 */
		if (isupper(spec))
			drive = spec - 'A';
		else
			drive = spec - 'a';
	}
	/*@
	 * The following nonsense strongly indicates this is either a partial,
	 * work in progress function, or a leftover, or an already cracked version.
	 *
	 * access() can be found in <unistd.h>, which can not be included yet
	 * as it conflicts with asm's brk() and sbrk(). So for now the call is
	 * commented out. As return code was not checked it was a useless call
	 * anyway, and the whole block, along with the then-unused filename var,
	 * can be removed.
	 */
	strcpy(filename,"a:jatgnas.8ys");
	filename[0] += (char)drive;
	//@ access(filename);

	return drive;
}
