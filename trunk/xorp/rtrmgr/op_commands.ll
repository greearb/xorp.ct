%{
#include <string.h>
#include "y.opcmd_tab.h"
#define YY_NO_UNPUT
//#define DEBUG_OPCMD_PARSER
%}
	int op_level=0;
	int op_linenum=1;
	extern void* opcmdlval;
%option noyywrap
%x comment
%%

"{"	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex({) ");
#endif
	op_level++;
	return UPLEVEL;
	}

"}"	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(})\n");
#endif
	op_level--;
	return DOWNLEVEL;
	}

";"	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(;)\n");
#endif
	return END;
	}

":"	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(:) ");
#endif
	return COLON;
	}

"\n"	{ 
	/*newline is not significant in op_commands*/
	op_linenum++;
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(newline)\n");
#endif
	}

[ \t]+	/*whitespace*/

\%[a-z][a-z0-9\-_]*	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(command:%s) ", opcmdtext);
#endif
	opcmdlval = strdup(opcmdtext);
	return COMMAND;
	}

\$\([a-zA-Z@][a-zA-Z0-9\-_.@\*]*\)	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(variable:%s) ", opcmdtext);
#endif
	opcmdlval = strdup(opcmdtext);
	return VARIABLE;
	}

[a-zA-Z/][a-zA-Z0-9\-_/]*	{
#ifdef DEBUG_OPCMD_PARSER
	printf(" lex(literal:%s) ", opcmdtext);
#endif
	opcmdlval = strdup(opcmdtext);
	return LITERAL;
	}

"/*"			BEGIN(comment);
<comment>[^*\n]* 	/*eat up anything that's not a '*' */
<comment>"*"+[^*/\n]* 	/*eat up '*'s not followed by "/"s */
<comment>\n		op_linenum++;
<comment>"*"+"/"	BEGIN(INITIAL);


%%


