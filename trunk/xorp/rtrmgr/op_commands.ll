%{
#include <string.h>
#include "y.opcmd_tab.h"
#define YY_NO_UNPUT
%}
	int opcmd_linenum = 1;
	extern void* opcmdlval;
%option noyywrap
%x comment


%%

"{"	{
	return UPLEVEL;
	}

"}"	{
	return DOWNLEVEL;
	}

[ \t]+	/* whitespace */

"\n"	{
	/* newline is not significant in op_commands */
	opcmd_linenum++;
	}

";"	{
	return END;
	}

":"	{
	return COLON;
	}

\%[a-z][a-z0-9\-_]*	{
	opcmdlval = strdup(opcmdtext);
	return COMMAND;
	}

\$\([a-zA-Z@][a-zA-Z0-9\-_.@\*]*\)	{
	opcmdlval = strdup(opcmdtext);
	return VARIABLE;
	}

[a-zA-Z/][a-zA-Z0-9\-_/]*	{
	opcmdlval = strdup(opcmdtext);
	return LITERAL;
	}

"/*"			BEGIN(comment);

<comment>[^*\n]* 	/* eat up anything that's not a '*' */

<comment>"*"+[^*/\n]* 	/* eat up '*'s not followed by "/"s */

<comment>\n		opcmd_linenum++;

<comment>"*"+"/"	BEGIN(INITIAL);

.	{
	/* everything else is a syntax error */
	return SYNTAX_ERROR;
	}


%%
