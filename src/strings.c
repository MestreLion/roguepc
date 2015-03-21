#include "extern.h"

//@ extern char ctp_[]; //@ not needed anymore. could not find definition

/*@
 * Functions available in <ctype.h>
 *
isalpha(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x03); }
isupper(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x01); }
islower(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x02); }
isdigit(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x04); }
isspace(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0x10); }
isprint(x) {	return  x > 128 ? 0 : (ctp_[(x)+1]&0xc7); }

toascii(x)
{
	return (x&127);
}

toupper(chr)
	char chr;
{
	return(islower(chr)?((chr)-('a'-'A')):(chr));
}

tolower(chr)
	char chr;
{
	return(isupper(chr)?((chr)+('a'-'A')):(chr));
}
*/

//@ Locale-independent versions, as expected by Rogue
bool is_alpha(char ch) { return (isascii(ch) && isalpha(ch)); }
bool is_upper(char ch) { return (isascii(ch) && isupper(ch)); }
bool is_lower(char ch) { return (isascii(ch) && islower(ch)); }
bool is_digit(char ch) { return (isascii(ch) && isdigit(ch)); }
bool is_space(char ch) { return (isascii(ch) && isspace(ch)); }
bool is_print(char ch) { return (isascii(ch) && isprint(ch)); }

/*@
 * No exact match in signature and behavior from glibc or POSIX
 * Similar to <string.h> strncpy(), but not a drop-in equivalent.
 * snprintf() is perhaps a better replacement candidate.
 */
char *
stccpy(s1,s2,count)
	char *s1, *s2;
	int count;
{
	while (count-->0 && *s2)
		*s1++ = *s2++;
	*s1 = 0;
	/*
	 * lets return the address of the end of the string so
	 * we can use that info if we are going to cat on something else!!
	 */
	return (s1);
}


/*
 * redo Lattice token parsing routines
 */

//@ strip leading blanks
char *
stpblk(str)
	char *str;
{
	while (is_space(*str))
		str++;
	return(str);
}

/*
 * remove trailing whitespace from the end of a line
 */
char *
endblk(str)
	char *str;
{
	register char *backup;

	backup = str + strlen(str);
	while (backup != str && is_space(*(--backup)))
		*backup = 0;
	return(str);
}

/*
 * lcase: convert a string to lower case
 */
void
lcase(str)
	char *str;
{
	while ( (*str = tolower(*str)) )
		str++;
}
