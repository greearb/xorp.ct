%{
#include <string.h>
#include "y.boot_tab.h"
#define SBUFSIZE 1024
%}
	int boot_linenum = 1;
	extern void* bootlval;
	char stringbuf[SBUFSIZE + 1];
%option noyywrap
%option nounput
%x comment
%x string

RE_IPV4_BYTE 25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?
RE_IPV4 {RE_IPV4_BYTE}\.{RE_IPV4_BYTE}\.{RE_IPV4_BYTE}\.{RE_IPV4_BYTE}
RE_IPV4_PREFIXLEN 3[0-2]|[0-2]?[0-9]
RE_IPV4NET {RE_IPV4}\/{RE_IPV4_PREFIXLEN}

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
RE_IPV6NET	{RE_IPV6}\/{RE_IPV6_PREFIXLEN}

RE_MACADDR [a-fA-F0-9]{2}(:[a-fA-F0-9]{2}){5}


%%

"{"	{
	return UPLEVEL;
	}

"}"	{
	return DOWNLEVEL;
	}

[ \t]+	/* whitespace */

"\n"	{
	boot_linenum++;
	return END;
	}

";"	{
	return END;
	}

":"	{
	return ASSIGN_OPERATOR;
	}

"true"	{
	bootlval = strdup(boottext);
	return BOOL_VALUE;
	}

"false"	{
	bootlval = strdup(boottext);
	return BOOL_VALUE;
	}

[0-9]+	{
	bootlval = strdup(boottext);
	return UINT_VALUE;
	}

{RE_IPV4}	{
	bootlval = strdup(boottext);
	return IPV4_VALUE;
	}

{RE_IPV4NET} {
	bootlval = strdup(boottext);
	return IPV4NET_VALUE;
	}

{RE_IPV6}	{
	bootlval = strdup(boottext);
	return IPV6_VALUE;
	}

{RE_IPV6NET} {
	bootlval = strdup(boottext);
	return IPV6NET_VALUE;
	}

{RE_MACADDR}	{
	bootlval = strdup(boottext);
	return MACADDR_VALUE;
	}

[a-z][a-z0-9"\-""_"]*	{
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
			boot_linenum++;
			strncat(stringbuf, "\n", SBUFSIZE);
			}

<string>\\+\n		/* allow quoted newlines */ {
			boot_linenum++;
			strncat(stringbuf, "\n", SBUFSIZE);
			}

<string>\"		{
			BEGIN(INITIAL);
			bootlval = strdup(stringbuf);
			return STRING;
			}

"/*"			BEGIN(comment);

<comment>[^*\n]* 	/* eat up anything that's not a '*' */

<comment>"*"+[^*/\n]* 	/* eat up '*'s not followed by "/"s */

<comment>\n		boot_linenum++;

<comment>"*"+"/"	BEGIN(INITIAL);

.	{
	/* everything else is a syntax error */
	return SYNTAX_ERROR;
	}


%%
