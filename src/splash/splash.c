#include <assert.h>  // assert
#include <stdarg.h>  // variable arguments: va_*, ...
#include <stdio.h>   // *printf, stderr, stdout
#include <stdlib.h>  // EXIT_FAILURE
#include <string.h>  // strcmp

#include "load_sdl.h"


static const char * const PICFILE = "../rogue.pic";
static const char * PROGNAME = NULL;  // == argv[0], set by main()


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
