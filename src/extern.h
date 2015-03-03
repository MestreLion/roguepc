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

//@ uintptr_t, uint16_t
#include <stdint.h>

//@ is{alpha,digit,upper,...}() and to{upper,lower,...}() families
#include <ctype.h>
#ifndef toascii
//@ Marked obsolescent in POSIX-2008, so not in <ctype.h> if C99 is used
#define toascii(c)	((c) & 0x7f)
#endif

//@ str{len,cat,cpy,cmp,chr}() and possibly others
#include <string.h>
#define bcopy(dest,source)	memmove(&(dest),&(source),sizeof(dest))
#define stpchr	strchr
#define setmem(dest,length,ch)	memset(dest,ch,length)

//@ sprintf(), f{open,read,seek,write,close}(), remove(), access(), putchar()
#include <stdio.h>

//@ atoi(), NULL, EXIT_*, malloc(), free(), abs()
#include <stdlib.h>
#define exit	croot_exit	//@ use croot's exit() as single point of exit
#define srand	md_srand	//@ use internal seed generator

//@ errno, originally in begin.asm
#include <errno.h>

//@ pause(), access()
#include <unistd.h>
#define daemon	start_daemon
#define access(f)	access(f, F_OK)

//@ time()
#include <time.h>
#define clock	md_clock

//@ vsprintf()
#include <stdarg.h>

//@ bool type, originally typedef unsigned char
#include <stdbool.h>


/*@
 * Project includes, defines and typedefs
 */
#include "swint.h"

//@ moved from curses.h so it's close to 'bool' definition
#define TRUE 	1
#define FALSE	0

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
void	setup(), clock_on(), no_clock(), flush_type(), credits(),
		unsetup(), one_tick();
char	*newmem();
byte	readchar();
bool	isjr(), set_ctrlb();
struct tm	*md_localtime();

//@ dos.asm
int 	csum();
int 	getds();
byte 	peekb();
void	pokeb();
void	out();
byte	in();
void	dmaout();
void	dmain();
void	_halt();
void	md_clock();
void	COFF();
bool	no_char();

//@ moved from main.c
void	fatal(const char *msg, ...);


/*@
 * Global vars
 */
extern unsigned int tick;  //@ from dos.asm
extern int  _dsval;  //@ from begin.asm
extern struct sw_regs *regs; //@ from main.c, originally declared in swint.h
#ifdef ROGUE_DEBUG
extern bool print_int_calls;
#endif
