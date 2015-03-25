/*
 * Defines for things used in mach_dep.c
 *
 * @(#)extern.h	5.1 (Berkeley) 5/11/82
 */

/*@
 * Also standard library includes, defines and "overrides",
 * assembly function declarations, and a bunch of global variables.
 *
 * The plan is to gradually move all platform-agnostic vars to rogue.h,
 * and keep here only the declarations for mach_dep.c (as originally intended)
 *
 * Standard Library includes will also remain here, as original code did not
 * (explicitly) use any. After all, Rogue is pre-ANSI C.
 *
 * Assembly functions are be replaced either by standard library equivalents
 * or functions in mach_dep.
 *
 * When port is completed, maybe this will be renamed mach_dep.h to avoid
 * confusion with extern.c, which is the definition of global game vars
 */

/*@
 * Functions from libc and their "overrides"
 */

//@ make setenv() and getenv() available, remove toascii() and isascii()
#define _POSIX_C_SOURCE 200809L

//@ uintptr_t, uint16_t
#include <stdint.h>

//@ is{alpha,digit,upper,...}() and to{upper,lower,...}() families
#include <ctype.h>
#ifndef isascii
//@ Marked obsolescent in POSIX-2008, so not in <ctype.h> if C99 is used
#define isascii(c)	(((c) & ~0x7f) == 0)
#endif

//@ str{len,cat,cpy,cmp,chr}() and possibly others
#include <string.h>
#define bcopy(dest,source)	memmove(&(dest),&(source),sizeof(dest))
#define stpchr	strchr
#define setmem(dest,length,ch)	memset(dest,ch,length)

//@ sprintf(), f{open,read,seek,write,close}(), remove(), access(), putchar()
//@ popen(), fgets(), pclose()
#include <stdio.h>

//@ exit(), atoi(), NULL, EXIT_*, malloc(), free(), abs(), setenv(), getenv()
#include <stdlib.h>
#define srand	md_srand	//@ use internal seed generator

//@ errno, originally in begin.asm
#include <errno.h>

//@ pause(), access(), sleep(), close()
#include <unistd.h>
#define access(f)	access(f, F_OK)

//@ time(), nanosleep()
#include <time.h>
#define clock	md_clock

//@ vsprintf()
#include <stdarg.h>

//@ bool type, originally typedef unsigned char
#include <stdbool.h>

#ifdef __linux__
//@ open()
#include <fcntl.h>
//@ ioctl()
#include <sys/ioctl.h>
//@ KDGKBLED
#include <linux/kd.h>
#endif

//@ setlocale()
#include <locale.h>


/*@
 * Project includes, defines and typedefs
 */
#include "swint.h"

//@ moved from curses.h so it's close to 'bool' definition
#ifndef TRUE
#define TRUE 	1
#define FALSE	0
#endif

#define msleep(ms)	md_nanosleep(1000000L * ms)

#ifdef __GNUC__
//@ macro for dummy arguments in stub functions
#define UNUSED(arg) __attribute__((unused))arg
#else
#define UNUSED(arg) arg
#endif


/*@
 * Simplified version of <time.h> struct tm, to wrap and abstract it,
 * so local time source is opaque and easily replaceable.
 */
struct md_tm {
	int second;		/* Seconds	[0-60] (1 leap second) */
	int minute;		/* Minutes	[0-59] */
	int hour;		/* Hours	[0-23] */
	int day;		/* Day		[1-31] */
	int month;		/* Month	[0-11] */
	int year;		/* Year */
};
typedef struct md_tm TM;

typedef uintptr_t	intptr;  //@ size of a real pointer
typedef uint16_t	dosptr;  //@ size of a pointer in DOS, as Rogue relies on
/*
 *  MANX C compiler funnies
 *  @ moved from rogue.h
 */
typedef unsigned char byte;


/*
 * Function types
 */
//@ mach_dep.c originals
int 	md_srand(), bdos(), swint(), sysint();
void	setup(), flush_type(), credits(), one_tick();
char	*newmem();
byte	readchar();
bool	isjr(), set_ctrlb();
#ifdef ROGUE_DOS_CLOCK
void	clock_on(void);
void	no_clock(void);
#endif

//@ new functions
byte	swap_bits(byte data, unsigned i, unsigned j, unsigned width);
int 	md_keyboard_leds(void);
long	md_time(void);
TM  	*md_localtime(void);
void	md_nanosleep(long nanoseconds);

//@ dos.asm
int 	csum();
byte 	peekb();
void	pokeb();
void	out();
byte	in();
void	dmaout();
void	dmain();
void	_halt();
void	md_clock();
void	COFF();

//@ moved from main.c
void	fatal(const char *msg, ...);

//@ moved from croot.c
void	md_exit(int status);


/*@
 * Global vars
 */
#ifdef ROGUE_DOS_CLOCK
extern unsigned int tick;  //@ from dos.asm
#endif
extern struct sw_regs *regs; //@ from main.c, originally declared in swint.h
#ifdef ROGUE_DEBUG
extern bool print_int_calls;
#endif
