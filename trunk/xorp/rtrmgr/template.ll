%{
#include <string.h>
#include "y.tplt_tab.h"
#define YY_NO_UNPUT
%}
	int level = 0;
	int linenum = 1;
	extern void* tpltlval;
%option noyywrap
%x comment

RE_IPV4_BYTE 25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?
RE_IPV4 {RE_IPV4_BYTE}\.{RE_IPV4_BYTE}\.{RE_IPV4_BYTE}\.{RE_IPV4_BYTE}
RE_IPV4_PREFIXLEN 3[0-2]|[0-2]?[0-9]
RE_IPV4_PREFIX {RE_IPV4}\/{RE_IPV4_PREFIXLEN}

/*
 * IPv6 address representation in Augmented Backus-Naur Form (ABNF).
 * Representation taken from email by Roy T. Fielding <roy.fielding@day.com>
 * to uri@w3.org mailing list on 05 Dec 2002:
 *   http://lists.w3.org/Archives/Public/uri/2002Dec/0000.html
 *
 *    IPv6address   = (                          6( h4 ":" ) ls32 )
 *                  / (                     "::" 5( h4 ":" ) ls32 )
 *                  / ( [              h4 ] "::" 4( h4 ":" ) ls32 )
 *                  / ( [ *1( h4 ":" ) h4 ] "::" 3( h4 ":" ) ls32 )
 *                  / ( [ *2( h4 ":" ) h4 ] "::" 2( h4 ":" ) ls32 )
 *                  / ( [ *3( h4 ":" ) h4 ] "::"    h4 ":"   ls32 )
 *                  / ( [ *4( h4 ":" ) h4 ] "::"             ls32 )
 *                  / ( [ *5( h4 ":" ) h4 ] "::"             h4   )
 *                  / ( [ *6( h4 ":" ) h4 ] "::"                  )
 *
 *    ls32          = ( h4 ":" h4 ) / IPv4address
 *                  ; least-significant 32 bits of address
 *    h4            = 1*4HEXDIG
 *    IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
 *    dec-octet     = 1*2DIGIT                      ; 0-9, 00-99
 *                  / ( "0" / "1" ) 2DIGIT          ; 000-199
 *                  / "2" %x30-34 DIGIT             ; 200-249
 *                  / "25" %x30-35                  ; 250-255
 *
 */
RE_H4 [a-fA-F0-9]{1,4}
RE_H4_COLON {RE_H4}:
RE_LS32 (({RE_H4}:{RE_H4})|{RE_IPV4})
RE_IPV6_P1	{RE_H4_COLON}{6}{RE_LS32}
RE_IPV6_P2	::{RE_H4_COLON}{5}{RE_LS32}
RE_IPV6_P3	({RE_H4})?::{RE_H4_COLON}{4}{RE_LS32}
RE_IPV6_P4	({RE_H4_COLON}{0,1}{RE_H4})?::{RE_H4_COLON}{3}{RE_LS32}
RE_IPV6_P5	({RE_H4_COLON}{0,2}{RE_H4})?::{RE_H4_COLON}{2}{RE_LS32}
RE_IPV6_P6	({RE_H4_COLON}{0,3}{RE_H4})?::{RE_H4_COLON}{1}{RE_LS32}
RE_IPV6_P7	({RE_H4_COLON}{0,4}{RE_H4})?::{RE_LS32}
RE_IPV6_P8	({RE_H4_COLON}{0,5}{RE_H4})?::{RE_H4}
RE_IPV6_P9	({RE_H4_COLON}{0,6}{RE_H4})?::
RE_IPV6		{RE_IPV6_P1}|{RE_IPV6_P2}|{RE_IPV6_P3}|{RE_IPV6_P4}|{RE_IPV6_P5}|{RE_IPV6_P6}|{RE_IPV6_P7}|{RE_IPV6_P8}|{RE_IPV6_P9}
RE_IPV6_PREFIXLEN 12[0-8]|1[01][0-9]|[0-9][0-9]?
RE_IPV6_PREFIX	{RE_IPV6}\/{RE_IPV6_PREFIXLEN}

RE_MACADDR [a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}

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
	return BOOL_INITIALIZER;
	}
"false"	{
#ifdef DEBUG_TEMPLATE_PARSER
	printf(" lex(init:false) ");
#endif
	tpltlval = strdup(tplttext);
	return BOOL_INITIALIZER;
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

[ \t]+	/* whitespace */

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
	printf("STRING >%s<", (char *)tpltlval);
#endif
	return STRING;
	}

"/*"			BEGIN(comment);
<comment>[^*\n]* 	/* eat up anything that's not a '*' */
<comment>"*"+[^*/\n]* 	/* eat up '*'s not followed by "/"s */
<comment>\n		linenum++;
<comment>"*"+"/"	BEGIN(INITIAL);

[\-]*[0-9]+	{
	tpltlval = strdup(tplttext);
	return INTEGER;
	}

{RE_IPV4}	{
	tpltlval = strdup(tplttext);
	return IPV4_INITIALIZER;
	}

{RE_IPV4_PREFIX}	{
	tpltlval = strdup(tplttext);
	return IPV4PREFIX_INITIALIZER;
	}

{RE_IPV6}	{
	tpltlval = strdup(tplttext);
	return IPV6_INITIALIZER;
	}

{RE_IPV6_PREFIX}	{
	tpltlval = strdup(tplttext);
	return IPV6PREFIX_INITIALIZER;
	}

{RE_MACADDR}	{
	tpltlval = strdup(tplttext);
	return MACADDR_INITIALIZER;
	}

%%
