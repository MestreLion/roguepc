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
 * and keep here only the definitions for mach_dep.c (as originally intended)
 * and other platform-dependent modules such as curses, env, croot, and
 * perhaps also load, save, protect.
 *
 * Standard Library includes will also remain here, as original code did not
 * (explicitly) use any. After all, Rogue is pre-ANSI C.
 *
 * Assembly functions will be replaced either by standard library equivalents
 * or functions in mach_dep.
 *
 * When port is successful, maybe this will be renamed mach_dep.h to avoid
 * confusion with extern.c, which is the definition of global game vars
 */

/*
 * Don't change the constants, since they are used for sizes in many
 * places in the program.
 */

#define MAXSTR		80	/* maximum length of strings */
#define MAXLINES	25	/* maximum number of screen lines used */
#define MAXCOLS		80	/* maximum number of screen columns used */

//@ uintptr_t
#include <stdint.h>
typedef uintptr_t	intptr;  //@ size of a real pointer
typedef uint16_t	dosptr;  //@ size of a pointer in DOS, as Rogue relies on


//@ moved from rogue.h for assembly functions global variables declarations
/*
 *  MANX C compiler funnies
 */
typedef unsigned char byte;
typedef unsigned char bool;

//@ moved from curses.h so it's close to 'bool' definition
#define TRUE 	1
#define FALSE	0

/*
 * Function types
 */
/*@
 * For the assembly files only. C functions were moved to rogue.h
 * Function signature was inferred from their standard counterparts, if any,
 * and their usage in rogue. This is just a stub for now, not an accurate
 * documentation on signature and sizes.
 */

//@ begin.asm
//@ Moved to mach_dep.c:
extern int  _dsval;
//@ Moved to save.c (no longer extern): char _lowmem, _Uend
//@ also contains void _exit() only used by croot.c
//@ and other public symbols not used in C code

//@ csav.asm
//@ it seems no symbols are directly referenced by any C code.

//@ dos.asm
void	wsetmem();
bool	no_char();  //@ actually return only 0 or 1, so a "true" bool
//@ Moved to mach_dep.c:
extern unsigned int tick;
int 	csum();
int 	getds();
byte 	peekb();
void	pokeb();
void	out();
void	dmaout();
void	dmain();
void	_halt();
void	clock();
void	COFF();

//@ fio.asm - replaced by <stdio.h> equivalents except write() in croot.c
//@ int 	open(), read(), write(), creat();
//@ void	close(), unlink(), lseek();

//@ sbrk.asm - replaced by <unistd.h>
//@ char *brk(), *sbrk();

//@ zoom.asm - replaced by functions in curses.c

/*@
 * Functions and constants from libc
 */

//@ is{alpha,digit,upper,...}() and to{ascii,upper,lower}() families
#include <ctype.h>

//@ str{len,cat,cpy,cmp,chr}() and possibly others
#include <string.h>
#define bcopy(a,b)	memmove(&(b),&(a),sizeof(a))
#define stpchr	strchr
#define setmem	memset

//@ sprintf(), f{open,read,seek,write,close}(), remove()
#include <stdio.h>
#define getch() (byte)getchar()

//@ atoi(), NULL, EXIT_*
#include <stdlib.h>
#define exit	exit_croot	//@ (pretend to) use croot's exit() for now
#define setenv	setenv_file	//@ use env.c fake environment
#define srand	srand_time	//@ use its rogue's own RNG mechanics

//@ errno, originally in begin.asm
#include <errno.h>

//@ brk(), sbrk(), pause()
#include <unistd.h>
#define daemon	start_daemon
