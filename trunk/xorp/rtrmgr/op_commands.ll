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
	/* newline is not significant */
	opcmd_linenum++;
	}

";"	{
	return END;
	}

":"	{
	return COLON;
	}

"%module"	{
	return CMD_MODULE;
	}

"%command"	{
	return CMD_COMMAND;
	}

"%help"	{
	return CMD_HELP;
	}

"%opt_parameter"	{
	return CMD_OPT_PARAMETER;
	}

"%tag"	{
	return CMD_TAG;
	}

\$\([a-zA-Z@][a-zA-Z0-9\-_\.@\*]*\)	{
	opcmdlval = strdup(opcmdtext);
	return VARIABLE;
	}

[a-zA-Z0-9_/\.][a-zA-Z0-9\-_/\.]*	{
	opcmdlval = strdup(opcmdtext);
	return LITERAL;
	}

\"[a-zA-Z0-9\-_\[\]:/&\.,<>!@#$%^*()+=|\\~`{}<>? \t]*\"	{
	opcmdlval = strdup(opcmdtext);
	return STRING;
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
