%{
#include <string.h>
#include "y.boot_tab.h"
#define YY_NO_UNPUT
#define SBUFSIZE 1024
%}
	int bootlinenum = 1;
	extern void* bootlval;
	char stringbuf[SBUFSIZE+1];
%option noyywrap
%x comment
%x string
DBYTE [12]?[0-9]{1,2}
IPV4 {DBYTE}.{DBYTE}.{DBYTE}.{DBYTE}
%%
"{"	{
	/* printf(" lex({) "); */
	return UPLEVEL;
	}
"}"	{
	/* printf(" lex(}) "); */
	return DOWNLEVEL;
	}
";"	{
	/* printf(" lex(;) "); */
	return END;
	}
"/"	{
	/* printf(" lex(/) "); */
	return SLASH;
	}
"\\"	{
	/* printf(" lex(\\ (SLASH)) "); */
	return SLASH;
	}
":"	{
	/* printf(" lex(:) "); */
	return ASSIGN_VALUE;
	}
","	{
	/* printf(" lex(,) "); */
	return LISTNEXT;
	}
"true"	{
	/* printf(" lex(init) "); */
	bootlval = strdup(boottext);
	return BOOL;
	}
"false"	{
	/* printf(" lex(init) "); */
	bootlval = strdup(boottext);
	return BOOL;
	}
"\n"	{
	bootlinenum++;
	return END;
	}
\"{IPV4}\" {
	bootlval = strdup(boottext);
	return IPV4;
	}
{IPV4}  {
	bootlval = strdup(boottext);
	return IPV4;
	}
\"{IPV4}"/"{DBYTE}\"  {
	bootlval = strdup(boottext);
	return IPV4NET;
	}
{IPV4}"/"{DBYTE}  {
	bootlval = strdup(boottext);
	return IPV4NET;
	}

[ \t]+	/*whitespace*/

[a-z][a-z0-9"\-""_"]*	{
	/* printf(" lex(literal) at %x\n ", lstr); */
	bootlval = strdup(boottext);
	return LITERAL;
	}
\"			{
			BEGIN(string);
			memset(stringbuf, 0, SBUFSIZE);
			}
<string>[^\\\n\"]*	/* normal text */ {
			strncat(stringbuf, boottext, SBUFSIZE);
			}
<string>\\+\"		/* allow quoted quotes */ {
			strncat(stringbuf, "\"", SBUFSIZE);
			}
<string>\\+\\		/* allow quoted backslash */ {
			strncat(stringbuf, "\\", SBUFSIZE);
			}
<string>\n		/* allow unquoted newlines */ {
			bootlinenum++;
			strncat(stringbuf, "\n", SBUFSIZE);
			}
<string>\\+\n		/* allow quoted newlines */ {
			bootlinenum++;
			strncat(stringbuf, "\n", SBUFSIZE);
			}
<string>\"		{
			BEGIN(INITIAL);
			/* printf("STRING:>%s<\n", stringbuf); */
			bootlval = strdup(stringbuf);
			return STRING;
			}

"/*"			BEGIN(comment);
<comment>[^*\n]* 	/* eat up anything that's not a '*' */
<comment>"*"+[^*/\n]* 	/* eat up '*'s not followed by "/"s */
<comment>\n		bootlinenum++;
<comment>"*"+"/"	BEGIN(INITIAL);

[0-9]+	{
	bootlval = strdup(boottext);
	return INTEGER;
	}

%%
