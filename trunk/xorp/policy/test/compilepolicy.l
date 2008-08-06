%{
// XXX: this whole parsing is becoming a mess... Once we finalize how to test
// policy and what it should look like, it needs a re-write.

#include "libxorp/xorp.h"

#include "policy/test/compilepolicy.hh"
#include "yacc.yy_compile_policy.hh"
#include "policy/common/policy_utils.hh"
#include <sstream>

#define yylval yy_compile_policylval
#define yyparse yy_compile_policyparse
#define yyerror yy_compile_policyerror

void yyerror(const char *);
int yyparse(void);

namespace {
	unsigned _yy_lineno;
	string _yy_last_err;
}

%}

%option prefix="yy_compile_policy"
%option outfile="lex.yy_compile_policy.cc"
%option noyywrap
%option nounput
%option never-interactive
%x STR BLOCK

RE_IPV4_BYTE 25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?
RE_IPV4 {RE_IPV4_BYTE}\.{RE_IPV4_BYTE}\.{RE_IPV4_BYTE}\.{RE_IPV4_BYTE}
RE_IPV4_PREFIXLEN 3[0-2]|[0-2]?[0-9]
RE_IPV4NET {RE_IPV4}\/{RE_IPV4_PREFIXLEN}

RE_H4 [a-fA-F0-9]{1,4}
RE_H4_COLON {RE_H4}:
RE_LS32 (({RE_H4}:{RE_H4})|{RE_IPV4})
RE_IPV6_P1      {RE_H4_COLON}{6}{RE_LS32}
RE_IPV6_P2      ::{RE_H4_COLON}{5}{RE_LS32}
RE_IPV6_P3      ({RE_H4})?::{RE_H4_COLON}{4}{RE_LS32}
RE_IPV6_P4      ({RE_H4_COLON}{0,1}{RE_H4})?::{RE_H4_COLON}{3}{RE_LS32}
RE_IPV6_P5      ({RE_H4_COLON}{0,2}{RE_H4})?::{RE_H4_COLON}{2}{RE_LS32}
RE_IPV6_P6      ({RE_H4_COLON}{0,3}{RE_H4})?::{RE_H4_COLON}{1}{RE_LS32}
RE_IPV6_P7      ({RE_H4_COLON}{0,4}{RE_H4})?::{RE_LS32}
RE_IPV6_P8      ({RE_H4_COLON}{0,5}{RE_H4})?::{RE_H4}
RE_IPV6_P9      ({RE_H4_COLON}{0,6}{RE_H4})?::
RE_IPV6 	{RE_IPV6_P1}|{RE_IPV6_P2}|{RE_IPV6_P3}|{RE_IPV6_P4}|{RE_IPV6_P5}|{RE_IPV6_P6}|{RE_IPV6_P7}|{RE_IPV6_P8}|{RE_IPV6_P9}
RE_IPV6_PREFIXLEN 12[0-8]|1[01][0-9]|[0-9][0-9]?
RE_IPV6NET      {RE_IPV6}\/{RE_IPV6_PREFIXLEN}

%%

\"		BEGIN(STR);

"{"		return YY_LBRACE;

"}"		return YY_RBRACE;

"source"	BEGIN(BLOCK); return YY_SOURCE;
"dest"		BEGIN(BLOCK); return YY_DEST;
"action"	BEGIN(BLOCK); return YY_ACTION;

<BLOCK>";"	{ return YY_SEMICOLON; }
<BLOCK>[[:blank:]\n]*"{"[[:blank:]\n]*	{  
	 	  _yy_lineno += policy_utils::count_nl(yytext);
	   	  return YY_LBRACE; 
					}
<BLOCK>[[:blank:]\n]*"}"[[:blank:]\n]*	{
		  BEGIN(INITIAL);
		  _yy_lineno += policy_utils::count_nl(yytext);
		  return YY_RBRACE; 
					}

<BLOCK>[^;{}]*	{ yylval.c_str = strdup(yytext);
		  _yy_lineno += policy_utils::count_nl(yytext);
		  return YY_STATEMENT;
		}

<STR>\"		BEGIN(INITIAL);
<STR>[^\"]+	{ yylval.c_str = strdup(yytext); 
		  _yy_lineno += policy_utils::count_nl(yytext);
		  return YY_STR;
		}

{RE_IPV4}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV4;
		}

{RE_IPV4NET}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV4NET;
		}
		
{RE_IPV6}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV6;
		}

{RE_IPV6NET}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV6NET;
		}

"term"		return YY_TERM;

"policy\-statement"	return YY_POLICY_STATEMENT;

"SET"		return YY_SET;

"import"	return YY_IMPORT;
"export"	return YY_EXPORT;

[[:alpha:]][[:alnum:]_]*	{ yylval.c_str = strdup(yytext);
				  return YY_ID;
				}  

;		return YY_SEMICOLON;

"\n"		_yy_lineno++;

[[:blank:]]+	/* eat blanks */

.		{ yyerror("Unknown character"); }

%%

void yyerror(const char *m) {
        ostringstream oss;

        oss << "Error on line " <<  _yy_lineno << ": " << m;

        _yy_last_err = oss.str();
}


// XXX: no memory management [a lot of leaks... for simplicy]
int
do_parsing(const string& conf, string& outerr) {
        YY_BUFFER_STATE yybuffstate = yy_scan_string(conf.c_str());

        _yy_last_err = "No error";
        _yy_lineno =1;

        int res = yyparse();

	yy_delete_buffer(yybuffstate);
        outerr = _yy_last_err;

        return res;
}
