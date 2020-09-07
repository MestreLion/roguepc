#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <SDL.h>

static const char * const PICFILE = "../rogue.pic";
static const char * PROGNAME = NULL;  // == argv[0], set by main()


int epyx_yeah(const char* path);

void usage(FILE* stream)
{
	fprintf(stream, "Usage: %s [-h|--help] [PICFILE]\n", PROGNAME);
}


// explain_output_error_and_die() using stdlib only
_Noreturn void fatal(const char *fmt, ...)
{
	char msg[1000];

	va_list argp;
	va_start(argp, fmt);
	vsnprintf(msg, sizeof(msg), fmt, argp);
	va_end(argp);

	if (errno)
		// just like perror()!
		fprintf(stderr, "%s: %s: %s\n", PROGNAME, strerror(errno), msg);
	else
		fprintf(stderr, "%s: %s\n", PROGNAME, msg);

	usage(stderr);
	exit(EXIT_FAILURE);
}


int main(int argc, char* argv[])
{
	char* arg;
	const char* path = NULL;

	PROGNAME = argv[0];

	while (--argc) {
		arg = *(++argv);
		if (!strcmp("-h", arg) || !strcmp("--help", arg)) {
			printf("Display a 320x200 PIC image in BSAVE format using SDL\n");
			printf("With CGA colors, palette 1i (Black / Cyan / Magenta / White)\n");
			printf("Default image path: %s\n", PICFILE);
			usage(stdout);
			return 0;
		}
		if (!strcmp("--", arg)) {
			argc--;
			break;
		}
		if (arg[0] == '-')
			fatal("invalid option: %s", arg);
		if (path)
			fatal("too many arguments: %s", arg);
		path = arg;
	}
	if (!path) {
		if (argc) {
			path = *(++argv);
			argc--;
		}
		else
			path = PICFILE;
	}
	if (argc)
		fatal("too many arguments: %s", *(++argv));

	assert(!argc && path);
	if (!epyx_yeah(path))
		exit(EXIT_FAILURE);
}


// A.K.A floor(log2(x)) or logb(x)
int log2i(int x)
{
	int r = 0;
	while (x >>= 1) r++;
	return r;
}


int epyx_yeah(const char* path)
{
	// Independent constants
	const int BSAVE_HEADER   =   7;  // BSAVE header is 7 bytes in rogue.pic
	const int CGA_WIDTH      = 320;  // Columns in graphics mode 4
	const int CGA_HEIGHT     = 200;  // Rows
	const int CGA_BG_COLOR   =   0;  // First color in palette is the background
	const int CGA_FIELDS     =   2;  // Interleaf: even and odd lines (rows)
	const int CGA_PADDING    = 192;  // 192 bytes of padding after each field
	const int CGA_COLORS[][3]= {     // Palette 1i (intensified), not all colors
		{  0,   0,   0},  // Black
		{ 85, 255, 255},  // Light Cyan
		{255,  85, 255},  // Light Magenta
		{255, 255, 255}   // White
	};

	// Derived constants
	const int CGA_NUM_COLORS = sizeof(CGA_COLORS) / sizeof(*CGA_COLORS);  // 4
	const int CGA_BIT_DEPTH  = log2i(CGA_NUM_COLORS);  // 2 bits per color
	const int CGA_PPB        = 8 / CGA_BIT_DEPTH;  // 4 pixels per Byte
	const int CGA_SIZE       = CGA_WIDTH * CGA_HEIGHT / CGA_PPB  // 16386
	                           + (CGA_FIELDS * CGA_PADDING);

	// Oh, the wonders of modern platforms and megabytes of stack space!
	// A modern GNU/Linux's 8 MB *stack* is almost as large as the *hard drive*
	// of an 1985 IBM XT, and more than 10 times its RAM. Food for thought...
	unsigned char data[CGA_SIZE];

	FILE*         file     = NULL;
	SDL_Window*   window   = NULL;
	SDL_Renderer* renderer = NULL;
	SDL_Event     event;

	if ((file = fopen(path, "rb")) == NULL)
		fatal("%s", path);  // rely on strerror(errno) built-in message

	// Read the file data, discarding the header
	if (   !fread(data, BSAVE_HEADER, 1, file)
	    || !fread(data, sizeof(data), 1, file)
	) {
		fclose(file);
		fatal("error reading file: %s", path);  // no errno set for EOF :(
	}
	fclose(file);

	if (   SDL_Init(SDL_INIT_VIDEO) != 0
	    || SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear") != SDL_TRUE
	    || SDL_CreateWindowAndRenderer(0, 0,
			SDL_WINDOW_FULLSCREEN_DESKTOP, &window, &renderer) != 0
	) {
		SDL_Quit();
		fatal("could not initialize SDL: %s", SDL_GetError());
	}
	SDL_RenderSetLogicalSize(renderer, CGA_WIDTH, CGA_HEIGHT);
	assert(CGA_BG_COLOR >= 0 && CGA_BG_COLOR < CGA_NUM_COLORS);
	SDL_SetRenderDrawColor(
		renderer,
		CGA_COLORS[CGA_BG_COLOR][0],
		CGA_COLORS[CGA_BG_COLOR][1],
		CGA_COLORS[CGA_BG_COLOR][2],
		SDL_ALPHA_OPAQUE
	);
	SDL_RenderClear(renderer);  // Clear the screen

	// Decode the image into the renderer
	// Could be done in fewer nested loops, but this is much easier to follow.
	int i = 0, field, y, x, p;
	unsigned char c, prevc = CGA_BG_COLOR;
	// Field loop: 2 blocks of even and odd lines
	for (field = 0; field < CGA_FIELDS; field++, i += CGA_PADDING) {
		// Line (row) loop. Screen Y is (y + field)
		for (y = 0; y < CGA_HEIGHT; y += CGA_FIELDS) {
			// Byte (column) loop. Screen X is (x + p)
			for (x = 0; x < CGA_WIDTH; x += CGA_PPB, i++) {
				assert(i < CGA_SIZE);
				// Pixel loop. Each byte contains 4 pixels
				for (p = 0; p < CGA_PPB; p++) {
					// "unpack" the pixels: (d >> 6, 4, 2, 0) & 0b00000011;
					c = (data[i] >> ((CGA_PPB - p - 1) * CGA_BIT_DEPTH))
							& (CGA_NUM_COLORS-1);
					assert(c < CGA_NUM_COLORS);
					// Set color, if needed
					if (c != prevc) {
						SDL_SetRenderDrawColor(
							renderer,
							CGA_COLORS[c][0],
							CGA_COLORS[c][1],
							CGA_COLORS[c][2],
							SDL_ALPHA_OPAQUE
						);
						prevc = c;
					}
					// Draw!
					SDL_RenderDrawPoint(renderer, x+p, y+field);
				}
			}
		}
	}
	assert(i == CGA_SIZE);
	SDL_RenderPresent(renderer);

	// Wait for 5 minutes or until keyboard/mouse button press
	while (SDL_GetTicks() < 1000 * 60 * 5) {
		if (   SDL_PollEvent(&event)
		    && (   event.type == SDL_QUIT
		        || event.type == SDL_KEYUP
		        || event.type == SDL_MOUSEBUTTONUP
		       )
		)
			break;
		SDL_Delay(16);  // ~60FPS
	}
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 1;
}
