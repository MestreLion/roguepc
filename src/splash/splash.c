#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define BSAVE_HEADER    7  // BSAVE header is 7 bytes in rogue.pic


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
	FILE* file = NULL;
	/*
	 * 0x4000 = 16384 bytes, the file size minus 7-byte header
	 * Oh, the wonders of modern platforms and megabytes of stack space!
	 */
	unsigned char data[0x4000];

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

	if ((file = fopen(path, "rb")) == NULL)
		fatal("%s", path);  // rely on strerror(errno) built-in message

	// Read the file data, discarding the header
	if (!fread(data, BSAVE_HEADER, 1, file) ||
	    !fread(data, sizeof(data), 1, file))
	{
		fclose(file);
		fatal("error reading file: %s", path);  // no errno set for EOF :(
	}
	fclose(file);
}
