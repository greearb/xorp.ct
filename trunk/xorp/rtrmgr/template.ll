%{
#include <string.h>
#include "y.tplt_tab.h"
#define YY_NO_UNPUT
%}
	int level=0;
	int linenum=1;
	extern void* tpltlval;
%option noyywrap
%x comment
%%
"{"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex({) ");
#endif
	level++;
	return UPLEVEL;
	}
"}"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(})\n");
#endif
	level--;
	return DOWNLEVEL;
	}
"text"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(text) ");
#endif
	tpltlval = strdup(tplttext);
	return TEXT;
	}
"uint"	{
	tpltlval = strdup(tplttext);
	return UINT;
	}
"int"	{
	tpltlval = strdup(tplttext);
	return INT;
	}
"bool"	{
	tpltlval = strdup(tplttext);
	return BOOL;
	}
"toggle"	{
	tpltlval = strdup(tplttext);
	return TOGGLE;
	}
"ipv4"	{
	tpltlval = strdup(tplttext);
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(ipv4:%s ", tplttext);
#endif
	return IPV4;
	}
"ipv4_prefix"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(ipv4_prefix:%s ", tplttext);
#endif
	tpltlval = strdup(tplttext);
	return IPV4PREFIX;
	}
"ipv6"	{
	tpltlval = strdup(tplttext);
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(ipv6:%s ", tplttext);
#endif
	return IPV6;
	}
"ipv6_prefix"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(ipv6_prefix:%s ", tplttext);
#endif
	tpltlval = strdup(tplttext);
	return IPV6PREFIX;
	}
"macaddr"	{
	tpltlval = strdup(tplttext);
	return MACADDR;
	}
"="	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(=) ");
#endif
	return ASSIGN_DEFAULT;
	}
";"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(;)\n");
#endif
	return END;
	}
":"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(:) ");
#endif
	return COLON;
	}
","	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(,) ");
#endif
	return LISTNEXT;
	}
"->"	{
	return RETURN;
	}
"true"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(init:true) ");
#endif
	tpltlval = strdup(tplttext);
	return INITIALIZER;
	}
"false"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(init:false) ");
#endif
	tpltlval = strdup(tplttext);
	return INITIALIZER;
	}
"/"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(/) ");
#endif
	return SLASH;
        }
"\\"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(\\ (SLASH)) ");
#endif
	return SLASH;
        }
"\n"	{ 
	linenum++;
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(newline)\n");
#endif
	}

[ \t]+	/*whitespace*/

\%[a-z][a-z0-9\-_]*	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(command:%s) ", tplttext);
#endif
	tpltlval = strdup(tplttext);
	return COMMAND;
	}
\$\([a-zA-Z@][a-zA-Z0-9\-_.@]*\)	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(variable:%s) ", tplttext);
#endif
	tpltlval = strdup(tplttext);
	return VARIABLE;
	}
[ \t]"@"	{
	return VARDEF;
	}

[a-zA-Z][a-zA-Z0-9\-_]*	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(literal:%s) ", tplttext);
#endif
	tpltlval = strdup(tplttext);
	return LITERAL;
	}
\"[a-zA-Z0-9\-_\[\]:/&.,<>!@#$%^*()+=|\\~`{}<>? \t]*\"	{
	tpltlval = strdup(tplttext);
#ifdef DEBUG_TEMPLATE_PARSER
	printf("STRING >%s<", (char*)tpltlval);
#endif
	return STRING;
	}

"/*"			BEGIN(comment);
<comment>[^*\n]* 	/*eat up anything that's not a '*' */
<comment>"*"+[^*/\n]* 	/*eat up '*'s not followed by "/"s */
<comment>\n		linenum++;
<comment>"*"+"/"	BEGIN(INITIAL);

[\-]*[0-9]+	{
	tpltlval = strdup(tplttext);
	return INTEGER;
	}


%%


