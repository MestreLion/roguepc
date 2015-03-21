/*
 * Env.c:   routines to set up environment
 *          Jon Lane  -  10/31/83
 */

/*@
 * setenv() and putenv() are different from their counterparts at <stdlib.h>:
 * "Environment" is read from a text file and manipulated in a custom struct
 */

#include "rogue.h"  //@ could be "extern.h" if not for some strings.c functions

#define ERROR   -1
#define MATCH    0
#define MAXEP	 8
#define FOREVER	 1

//@ made static. could also be hardcoded in struct environment element array
static char l_name[] = "name";
static char l_save[] = "savefile";
static char l_score[] = "scorefile";
static char l_macro[] = "macro";
static char l_fruit[] = "fruit";
static char l_drive[] = "drive";
static char l_menu [] = "menu";
static char l_screen[]   = "screen";

//@ public extern'ed vars
char whoami[] = "Rodney\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char s_score[]  =  "rogue.scr\0\0\0\0\0";
char s_save[]   =   "rogue.sav\0\0\0\0\0";
char macro[]    =   "v\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
char fruit[]    =  "Slime Mold\0\0\0\0\0\0\0\0\0\0\0\0\0";
char s_drive[]  =  "?";
char s_menu[]   =  "on\0";
char s_screen[]    =  "\0w fast";

static
struct environment {
	char *e_label;
	char *e_string;
	int  strlen;
} element[MAXEP] = {
	{l_name,	whoami,		23},
	{l_score,	s_score,	14},
	{l_save,	s_save,		14},
	{l_macro,	macro,		40},
	{l_fruit,	fruit,		23},
	{l_drive,	s_drive,	 1},
	{l_menu,	s_menu,		 3},
	{l_screen,	s_screen,	 7},
};

static byte	peekc(void);
static void	putenv_struct(char *label, char *string);

//@ already static in original
static FILE *file;
static byte ch;
static int pstate;
static char blabel[11], bstring[25];
static char *plabel, *pstring;

//@ renamed from setenv() to avoid collision with <stdlib.h>
/*
 *  setenv_file: read in environment from a file
 *
 *        envfile - name of file that contains data to be
 *                  put in the environment
 *
 *        STATUS  - setenv return
 *                  @@ FALSE on failure to open envfile, TRUE otherwise
 */
bool
setenv_from_file(envfile)
	char *envfile;
{
	register char pc;

	one_tick();	/* if he tries to disable the clock */
	if ((file = fopen(envfile, "r")) == NULL)
	{
		return FALSE;
	}

	while ( FOREVER )
	{
		/*
		 * Look for another label
		 */
		pstate = 0;
		plabel = blabel;
		pstring = bstring;

		/*
		 * Skip white space, this is the only state (pstate == 0)
		 * where eof will not be aborted
		 */
		while (is_space(peekc()))
			;
		if (ch == 0) {
			fclose(file);
			return TRUE;
		}
		pstate = 3;
		/*
		 * Skip comments.
		 */
		if (ch == '#') {
			while (peekc() != '\n')
				;
			continue;
		}
		pstate = 1;
		/*
		 * start of label found
		 */
		*plabel = ch;
		while ((pc = peekc()) != '=' && pc != '-')
			if (!is_space(*plabel) || !is_space(ch))
				*(++plabel) = ch;
		if (!is_space(*plabel))
			plabel++;
		*plabel = 0;

		/*
		 * Looking for corresponding string
		 */
		while (is_space(peekc()))
			;

		/*
		 * Start of string found
		 */
		pstate = 2;
		*pstring = ch;
		while (peekc() != '\n')
			if (!is_space(*pstring) || !is_space(ch))
				*(++pstring) = ch;
		if (!is_space(*pstring))
			pstring++;
		*pstring = 0;
		lcase(blabel);
		putenv_struct(blabel,bstring);
		/* printf("env: found (%s) = (%s)\n",blabel,bstring); */
	}
	/*
	 * for all environment strings that have to be in lowercase ....
	 * @ this will never be reached, there is no `break` in previous `while`
	 */
	lcase(s_menu);
	lcase(s_screen);
	return TRUE;
}

/*
 *  Peekc -
 *  Return the next char associated with
 *  efd (environment file descripter
 *
 *  This routine has some knowledge of the
 *  file parsing state so that it knows
 *  if there has been a premature eof.  This
 *  way I can avoid checking for premature eof
 *  every time a character is read.
 */
static
byte
peekc(void)
{
	ch = 0;
	/*
	 * we make sure that the strings never get filled past
	 * the end, this way we only have to check for these
	 * things once
	 */
	if (plabel > &blabel[10])
		plabel = &blabel[10];
	if (pstring > &bstring[24])
		pstring = &bstring[24];
	if (!fread(&ch, 1, 1, file) && pstate != 0) {
		/*
		 * When looking for the end of the string,
		 * Let the eof look like newlines
		 */
		if (pstate >= 2)
			return('\n');
		fatal("rogue.opt: incorrect file format\n");
	}
	if (ch == 26)  //@ EOF char, common in text files back then.
		ch = '\n';
	return(ch);
}

#ifdef LUXURY
/*
 * Getenv: UNIX compatable call
 *
 *	  label - label of thing in environment
 *
 *	  STATUS - returns the string associated with the label
 *			   or NULL (0) if it is not present
 *
 * @ used only by the unused is_set(), so safe to remove. renamed from getenv()
 * @ to avoid conflict with <stdlib.h>, and also made static
 */
static
char *
getenv_struct(label)
	char *label;
{
	register int i;

	for (i=0 ; i<MAXEP ; i++ )
	{
		if ( strcmp(label,element[i].e_label) == MATCH )
			return(element[i].e_string);
	}
	return(NULL);
}
#endif

//@ renamed from putenv() to avoid collision with <stdlib.h>
/*
 * putenv_struct: Put something into the "fake" environment struct
 *
 *	  label  - label of thing in environment
 *	  string - string associated with the label
 *
 *	  No meaningful return codes to save data space
 *	  Just ingnores strange labels
 */
void
putenv_struct(label,string)
	char *label, *string;
{
	register int i;

	for (i=0 ; i<MAXEP ; i++)
	{
		if ( strcmp(label,element[i].e_label) == MATCH )
			stccpy(element[i].e_string, string, element[i].strlen);
	}
}

#ifdef LUXURY
//@ unused, safe to remove, made static
static
bool
is_set(label,string)
	char *label,*string;
{
	return(!strcmp(string,getenv(label)));
}
#endif
