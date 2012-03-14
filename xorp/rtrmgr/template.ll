%{
/*
 * XXX: A hack to get rid of problematic __unused definition that might
 * be inserted by lex itself and that might be conflicting when including
 * some of the system header files.
 */
#ifdef __unused
#define __xorp_unused __unused
#undef __unused
#endif

#define YYSTYPE char*
#include "libxorp/xorp.h"

#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_NETBSD)
#include "y.tplt_tab.h"
#else
#include "y.tplt_tab.hh"
#endif

#ifdef __xorp_unused
#define __unused __xorp_unused
#undef __xorp_unused
#endif
%}
	int tplt_linenum = 1;
	string tplt_parsebuf;
%option noyywrap
%option nounput
%option never-interactive
%x comment
%x string


/*
 * Regular expressions of IP and MAC addresses, URLs, etc.
 */

/*
 * IPv4 address representation.
 */
RE_IPV4_BYTE	25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?
RE_IPV4		{RE_IPV4_BYTE}"."{RE_IPV4_BYTE}"."{RE_IPV4_BYTE}"."{RE_IPV4_BYTE}
RE_IPV4_PREFIXLEN 3[0-2]|[0-2]?[0-9]
RE_IPV4NET	{RE_IPV4}"/"{RE_IPV4_PREFIXLEN}

/*
 * IPv6 address representation in Augmented Backus-Naur Form (ABNF)
 * as defined in RFC-2234.
 * IPv6 address representation taken from RFC-3986:
 *
 *  IPv6address   =                            6( h16 ":" ) ls32
 *                /                       "::" 5( h16 ":" ) ls32
 *                / [               h16 ] "::" 4( h16 ":" ) ls32
 *                / [ *1( h16 ":" ) h16 ] "::" 3( h16 ":" ) ls32
 *                / [ *2( h16 ":" ) h16 ] "::" 2( h16 ":" ) ls32
 *                / [ *3( h16 ":" ) h16 ] "::"    h16 ":"   ls32
 *                / [ *4( h16 ":" ) h16 ] "::"              ls32
 *                / [ *5( h16 ":" ) h16 ] "::"              h16
 *                / [ *6( h16 ":" ) h16 ] "::"
 *
 *  h16           = 1*4HEXDIG
 *  ls32          = ( h16 ":" h16 ) / IPv4address
 *  IPv4address   = dec-octet "." dec-octet "." dec-octet "." dec-octet
 *  dec-octet     = DIGIT                  ; 0-9
 *                 / %x31-39 DIGIT         ; 10-99
 *                 / "1" 2DIGIT            ; 100-199
 *                 / "2" %x30-34 DIGIT     ; 200-249
 *                 / "25" %x30-35          ; 250-255
 */

RE_H16		[a-fA-F0-9]{1,4}
RE_H16_COLON	{RE_H16}":"
RE_LS32		(({RE_H16}":"{RE_H16})|{RE_IPV4})
RE_IPV6_P1	{RE_H16_COLON}{6}{RE_LS32}
RE_IPV6_P2	"::"{RE_H16_COLON}{5}{RE_LS32}
RE_IPV6_P3	({RE_H16})?"::"{RE_H16_COLON}{4}{RE_LS32}
RE_IPV6_P4	({RE_H16_COLON}{0,1}{RE_H16})?"::"{RE_H16_COLON}{3}{RE_LS32}
RE_IPV6_P5	({RE_H16_COLON}{0,2}{RE_H16})?"::"{RE_H16_COLON}{2}{RE_LS32}
RE_IPV6_P6	({RE_H16_COLON}{0,3}{RE_H16})?"::"{RE_H16_COLON}{1}{RE_LS32}
RE_IPV6_P7	({RE_H16_COLON}{0,4}{RE_H16})?"::"{RE_LS32}
RE_IPV6_P8	({RE_H16_COLON}{0,5}{RE_H16})?"::"{RE_H16}
RE_IPV6_P9	({RE_H16_COLON}{0,6}{RE_H16})?"::"
RE_IPV6		{RE_IPV6_P1}|{RE_IPV6_P2}|{RE_IPV6_P3}|{RE_IPV6_P4}|{RE_IPV6_P5}|{RE_IPV6_P6}|{RE_IPV6_P7}|{RE_IPV6_P8}|{RE_IPV6_P9}
RE_IPV6_PREFIXLEN 12[0-8]|1[01][0-9]|[0-9][0-9]?
RE_IPV6NET	{RE_IPV6}"/"{RE_IPV6_PREFIXLEN}

/*
 * Ethernet MAC address representation.
 */
RE_MACADDR	[a-fA-F0-9]{1,2}(:[a-fA-F0-9]{1,2}){5}

/*
 * URL-related regular expressions.
 *
 * Implementation is based on the BNF-like specification from:
 * - RFC-1738: HTTP, FTP, FILE
 * - RFC-3617: TFTP
 * - RFC-3986: update of RFC-1738
 */
RE_URL		{RE_URL_FILE}|{RE_URL_FTP}|{RE_URL_HTTP}|{RE_URL_TFTP}

/*
 * URL schemeparts for IP based protocols.
 * Representation taken from RFC-1738, and some of it is updated by RFC-3986.
 *
 * login          = [ user [ ":" password ] "@" ] hostport
 * hostport       = host [ ":" port ]
 * host           = hostname | hostnumber
 * hostname       = *[ domainlabel "." ] toplabel
 * domainlabel    = alphadigit | alphadigit *[ alphadigit | "-" ] alphadigit
 * toplabel       = alpha | alpha *[ alphadigit | "-" ] alphadigit
 * alphadigit     = alpha | digit
 * hostnumber     = digits "." digits "." digits "." digits
 * port           = digits
 * user           = *[ uchar | ";" | "?" | "&" | "=" ]
 * password       = *[ uchar | ";" | "?" | "&" | "=" ]
 */
RE_URL_LOGIN	({RE_URL_USER}(":"{RE_URL_PASSWORD})?"@")?{RE_URL_HOSTPORT}
RE_URL_HOSTPORT	{RE_URL_HOST}(":"{RE_URL_PORT})?
RE_URL_HOST	{RE_URL_HOSTNAME}|{RE_IPV4}|{RE_URL_IP_LITERAL}
RE_URL_IP_LITERAL "["({RE_IPV6}|{RE_URL_IPV_FUTURE})"]"
RE_URL_IPV_FUTURE "v"({RE_URL_HEXDIG})+"."({RE_URL_UNRESERVED}|{RE_URL_SUBDELIMS}|":")+
RE_URL_HOSTNAME	({RE_URL_DOMAINLABEL}".")*{RE_URL_TOPLABEL}
RE_URL_DOMAINLABEL {RE_URL_ALPHADIGIT}|{RE_URL_ALPHADIGIT}({RE_URL_ALPHADIGIT}|"-")*{RE_URL_ALPHADIGIT}
RE_URL_TOPLABEL	{RE_URL_ALPHA}|{RE_URL_ALPHA}({RE_URL_ALPHADIGIT}|"-")*{RE_URL_ALPHADIGIT}
RE_URL_ALPHADIGIT {RE_URL_ALPHA}|{RE_URL_DIGIT}
RE_URL_HOSTNUMBER {RE_URL_DIGITS}"."{RE_URL_DIGITS}"."{RE_URL_DIGITS}"."{RE_URL_DIGITS}
RE_URL_PORT	{RE_URL_DIGITS}
RE_URL_USER	({RE_URL_UCHAR}|";"|"?"|"&"|"=")*
RE_URL_PASSWORD	({RE_URL_UCHAR}|";"|"?"|"&"|"=")*

/*
 * FILE URL regular expression.
 * Representation taken from RFC-1738.
 *
 * fileurl        = "file://" [ host | "localhost" ] "/" fpath
 */
RE_URL_FILE	"file://"({RE_URL_HOST}|"localhost")?"/"{RE_URL_FPATH}

/*
 * FTP URL regular expression.
 * Representation taken from RFC-1738.
 *
 * ftpurl         = "ftp://" login [ "/" fpath [ ";type=" ftptype ]]
 * fpath          = fsegment *[ "/" fsegment ]
 * fsegment       = *[ uchar | "?" | ":" | "@" | "&" | "=" ]
 * ftptype        = "A" | "I" | "D" | "a" | "i" | "d"
 */
RE_URL_FTP	"ftp://"{RE_URL_LOGIN}("/"{RE_URL_FPATH}(";type="{RE_URL_FTPTYPE})?)?
RE_URL_FPATH	{RE_URL_FSEGMENT}("/"{RE_URL_FSEGMENT})*
RE_URL_FSEGMENT	({RE_URL_UCHAR}|"?"|":"|"@"|"&"|"=")*
RE_URL_FTPTYPE	"A"|"I"|"D"|"a"|"i"|"d"

/*
 * HTTP URL regular expression.
 * Representation taken from RFC-1738.
 *
 * httpurl        = "http://" hostport [ "/" hpath [ "?" search ]]
 * hpath          = hsegment *[ "/" hsegment ]
 * hsegment       = *[ uchar | ";" | ":" | "@" | "&" | "=" ]
 * search         = *[ uchar | ";" | ":" | "@" | "&" | "=" ]
 */
RE_URL_HTTP	"http://"{RE_URL_HOSTPORT}("/"{RE_URL_HPATH}("?"{RE_URL_SEARCH})?)?
RE_URL_HPATH	{RE_URL_HSEGMENT}("/"{RE_URL_HSEGMENT})*
RE_URL_HSEGMENT	({RE_URL_UCHAR}|";"|":"|"@"|"&"|"=")*
RE_URL_SEARCH	({RE_URL_UCHAR}|";"|":"|"@"|"&"|"=")*

/*
 * TFTP URL regular expression.
 * Representation taken from RFC-3617.
 *
 * tftpURI         = "tftp://" host "/" file [ mode ]
 * mode            = ";"  "mode=" ( "netascii" / "octet" )
 * file            = *( unreserved / escaped )
 * host            = <as specified by RFC 2732 [3]>
 * unreserved      = <as specified in RFC 2396 [4]>
 * escaped         = <as specified in RFC 2396>
 */
RE_URL_TFTP	"tftp://"{RE_URL_HOST}"/"{RE_URL_TFTP_FILE}({RE_URL_TFTP_MODE})?
RE_URL_TFTP_MODE ";""mode="("netascii"|"octet")
RE_URL_TFTP_FILE ({RE_URL_UNRESERVED}|{RE_URL_ESCAPE})*

/*
 * URL-related miscellaneous definitions.
 * Representation taken from RFC-1738 and from RFC-3986.
 *
 * lowalpha       = "a" | "b" | "c" | "d" | "e" | "f" | "g" | "h" |
 *                  "i" | "j" | "k" | "l" | "m" | "n" | "o" | "p" |
 *                  "q" | "r" | "s" | "t" | "u" | "v" | "w" | "x" |
 *                  "y" | "z"
 * hialpha        = "A" | "B" | "C" | "D" | "E" | "F" | "G" | "H" | "I" |
 *                  "J" | "K" | "L" | "M" | "N" | "O" | "P" | "Q" | "R" |
 *                  "S" | "T" | "U" | "V" | "W" | "X" | "Y" | "Z"
 * alpha          = lowalpha | hialpha
 * digit          = "0" | "1" | "2" | "3" | "4" | "5" | "6" | "7" |
 *                  "8" | "9"
 * safe           = "$" | "-" | "_" | "." | "+"
 * extra          = "!" | "*" | "'" | "(" | ")" | ","
 * national       = "{" | "}" | "|" | "\" | "^" | "~" | "[" | "]" | "`"
 * punctuation    = "<" | ">" | "#" | "%" | <">
 *
 *
 * reserved       = ";" | "/" | "?" | ":" | "@" | "&" | "="
 * hex            = digit | "A" | "B" | "C" | "D" | "E" | "F" |
 *                  "a" | "b" | "c" | "d" | "e" | "f"
 * escape         = "%" hex hex
 *
 * unreserved     = alpha | digit | safe | extra
 * uchar          = unreserved | escape
 * xchar          = unreserved | reserved | escape
 * digits         = 1*digit
 *
 * sub-delims     = "!" / "$" / "&" / "'" / "(" / ")"
 *                / "*" / "+" / "," / ";" / "="
 */
RE_URL_LOWALPHA	[a-z]
RE_URL_HIALPHA	[A-Z]
RE_URL_ALPHA	{RE_URL_LOWALPHA}|{RE_URL_HIALPHA}
RE_URL_DIGIT	[0-9]
RE_URL_SAFE	"$"|"-"|"_"|"."|"+"
RE_URL_EXTRA	"!"|"*"|"'"|"("|")"|","
RE_URL_NATIONAL	"{"|"}"|"|"|"\"|"^"|"~"|"["|"]"|"`"
RE_URL_PUNCTUATION "<"|">"|"#"|"%"|<">
RE_URL_RESERVED	";"|"/"|"?"|":"|"@"|"&"|"="
RE_URL_HEXDIG	{RE_URL_DIGIT}|[A-F]|[a-f]
RE_URL_ESCAPE	"%"{RE_URL_HEXDIG}{RE_URL_HEXDIG}
RE_URL_UNRESERVED {RE_URL_ALPHA}|{RE_URL_DIGIT}|{RE_URL_SAFE}|{RE_URL_EXTRA}
RE_URL_UCHAR	{RE_URL_UNRESERVED}|{RE_URL_ESCAPE}
RE_URL_XCHAR	{RE_URL_UNRESERVED}|{RE_URL_RESERVED}|{RE_URL_ESCAPE}
RE_URL_DIGITS	{RE_URL_DIGIT}{1,}
RE_URL_SUBDELIMS "!"|"$"|"&"|"'"|"("|")"|"*"|"+"|","|";"|"="


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
	tplt_linenum++;
	}

";"	{
	return END;
	}

":"	{
	return COLON;
	}

"="	{
	return ASSIGN_DEFAULT;
	}

","	{
	return LISTNEXT;
	}

"->"	{
	tpltlval = strdup(tplttext);
	return RETURN;
	}

"txt"	{
	tpltlval = strdup(tplttext);
	return TEXT_TYPE;
	}

"i32"	{
	tpltlval = strdup(tplttext);
	return INT_TYPE;
	}

"u32range"	{
	tpltlval = strdup(tplttext);
	return UINTRANGE_TYPE;
	}

"u32"	{
	tpltlval = strdup(tplttext);
	return UINT_TYPE;
	}

"bool"	{
	tpltlval = strdup(tplttext);
	return BOOL_TYPE;
	}

"toggle"	{
	tpltlval = strdup(tplttext);
	return TOGGLE_TYPE;
	}

"ipv4range"	{
	tpltlval = strdup(tplttext);
	return IPV4RANGE_TYPE;
	}

"ipv4"	{
	tpltlval = strdup(tplttext);
	return IPV4_TYPE;
	}

"ipv4net"	{
	tpltlval = strdup(tplttext);
	return IPV4NET_TYPE;
	}

"ipv6range"	{
	tpltlval = strdup(tplttext);
	return IPV6RANGE_TYPE;
	}

"ipv6"	{
	tpltlval = strdup(tplttext);
	return IPV6_TYPE;
	}

"ipv6net"	{
	tpltlval = strdup(tplttext);
	return IPV6NET_TYPE;
	}

"macaddr"	{
	tpltlval = strdup(tplttext);
	return MACADDR_TYPE;
	}

"url_file"	{
	tpltlval = strdup(tplttext);
	return URL_FILE_TYPE;
	}

"url_ftp"	{
	tpltlval = strdup(tplttext);
	return URL_FTP_TYPE;
	}

"url_http"	{
	tpltlval = strdup(tplttext);
	return URL_HTTP_TYPE;
	}

"url_tftp"	{
	tpltlval = strdup(tplttext);
	return URL_TFTP_TYPE;
	}

"true"	{
	tpltlval = strdup(tplttext);
	return BOOL_VALUE;
	}

"false"	{
	tpltlval = strdup(tplttext);
	return BOOL_VALUE;
	}

[0-9]+".."[0-9]+	{
	tpltlval = strdup(tplttext);
	return UINTRANGE_VALUE;
	}

[\-]*[0-9]+	{
	tpltlval = strdup(tplttext);
	return INTEGER_VALUE;
	}

{RE_IPV4}".."{RE_IPV4}	{
	tpltlval = strdup(tplttext);
	return IPV4RANGE_VALUE;
	}

{RE_IPV4}	{
	tpltlval = strdup(tplttext);
	return IPV4_VALUE;
	}

{RE_IPV4NET} {
	tpltlval = strdup(tplttext);
	return IPV4NET_VALUE;
	}

{RE_IPV6}".."{RE_IPV6}	{
	tpltlval = strdup(tplttext);
	return IPV6RANGE_VALUE;
	}

{RE_IPV6}	{
	tpltlval = strdup(tplttext);
	return IPV6_VALUE;
	}

{RE_IPV6NET} {
	tpltlval = strdup(tplttext);
	return IPV6NET_VALUE;
	}

{RE_MACADDR}	{
	tpltlval = strdup(tplttext);
	return MACADDR_VALUE;
	}

{RE_URL_FILE}	{
	tpltlval = strdup(tplttext);
	return URL_FILE_VALUE;
	}

{RE_URL_FTP}	{
	tpltlval = strdup(tplttext);
	return URL_FTP_VALUE;
	}

{RE_URL_HTTP}	{
	tpltlval = strdup(tplttext);
	return URL_HTTP_VALUE;
	}

{RE_URL_TFTP}	{
	tpltlval = strdup(tplttext);
	return URL_TFTP_VALUE;
	}

[ \t]"@"	{
	tpltlval = strdup("@");
	return VARDEF;
	}

\%[a-zA-Z][a-zA-Z0-9\-_]*	{
	tpltlval = strdup(tplttext);
	return COMMAND;
	}

\$\([a-zA-Z@][a-zA-Z0-9\-_\.@]*\)	{
	tpltlval = strdup(tplttext);
	return VARIABLE;
	}

[a-zA-Z][a-zA-Z0-9\-_]*	{
	tpltlval = strdup(tplttext);
	return LITERAL;
	}

\"			{
			BEGIN(string);
			/* XXX: include the original quote */
			tplt_parsebuf="\"";
			}

<string>[^\\\n\"]*	/* normal text */ {
			tplt_parsebuf += tplttext;
			}

<string>\\+\"		/* allow quoted quotes */ {
			tplt_parsebuf += "\"";
			}

<string>\\+\\		/* allow quoted backslash */ {
			tplt_parsebuf += "\\";
			}

<string>\n		/* allow unquoted newlines */ {
			tplt_linenum++;
			tplt_parsebuf += "\n";
			}

<string>\\+n		/* allow C-style quoted newlines */ {
			/* XXX: don't increment the line number */
			tplt_parsebuf += "\n";
			}

<string>\"		{
			BEGIN(INITIAL);
			/* XXX: include the original quote */
			tplt_parsebuf += "\"";
			tpltlval = strdup(tplt_parsebuf.c_str());
			return STRING;
			}

"/*"			BEGIN(comment);

<comment>[^*\n]* 	/* eat up anything that's not a '*' */

<comment>"*"+[^*/\n]* 	/* eat up '*'s not followed by "/"s */

<comment>\n		tplt_linenum++;

<comment>"*"+"/"	BEGIN(INITIAL);

.	{
	/* everything else is a syntax error */
	return SYNTAX_ERROR;
	}


%%
