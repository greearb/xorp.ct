%{

#include <vector>
#include <string>
#include <sstream>

#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "policy/common/policy_utils.hh"
#include "policy/policy_parser.hh"

#if defined(NEED_LEX_H_HACK)
#include "y.policy_parser_tab.cc.h"
#else
#include "y.policy_parser_tab.hh"
#endif

#define yylval policy_parserlval
#define yyerror policy_parsererror
#define yyparse policy_parserparse

void yyerror(const char *m);
extern int yyparse(void);

using namespace policy_parser;

// instantiate the globals here.
vector<Node*>* policy_parser::_parser_nodes;
unsigned policy_parser::_parser_lineno;

// try not to pollute
namespace {
	string _last_error;
	Term::BLOCKS _block;
}

%}

%option prefix="policy_parser"
%option outfile="lex.policy_parser.cc"
%option noyywrap
%option nounput
%option never-interactive
%x STR

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

[[:digit:]]+".."[[:digit:]]+	{ yylval.c_str = strdup(yytext);
		  return YY_UINTRANGE;
		}

[[:digit:]]+	{ yylval.c_str = strdup(yytext);
		  return YY_UINT;
		}

-[[:digit:]]+	{ yylval.c_str = strdup(yytext);
		  return YY_INT;
		}

"true"		{ yylval.c_str = strdup(yytext);
		  return YY_BOOL;
		}

"false"		{ yylval.c_str = strdup(yytext);
		  return YY_BOOL;
		}

\"|\'		BEGIN(STR);

<STR>\"|\'	BEGIN(INITIAL);

<STR>[^\"\']+	{ yylval.c_str = strdup(yytext);
		  _parser_lineno += policy_utils::count_nl(yytext);
		  /* XXX: a string can be started with " but terminated with '
		   * and vice versa...
		   */
		  return YY_STR;
		}

{RE_IPV4}".."{RE_IPV4}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV4RANGE;
		}

{RE_IPV4}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV4;
		}

{RE_IPV4NET}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV4NET;
		}


{RE_IPV6}".."{RE_IPV6}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV6RANGE;
		}

{RE_IPV6}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV6;
		}

{RE_IPV6NET}	{
		  yylval.c_str = strdup(yytext);
		  return YY_IPV6NET;
		}

":"		{
		  // the colon is an alias for asignment in action and equality
		  // in the source / dest blocks.
		  if (_block == Term::ACTION)
			return YY_ASSIGN;
		  else
			return YY_EQ;
		}

"<<"	return YY_LSHIFT;
">>"	return YY_RSHIFT;
"("		return YY_LPAR;
")"		return YY_RPAR;
"=="		return YY_EQ;
"!="		return YY_NE;
"<="		return YY_LE;
">="		return YY_GE;
"<"		return YY_LT;
">"		return YY_GT;
"+"		return YY_ADD;
"*"		return YY_MUL;
"\-"		return YY_SUB;
"="		return YY_ASSIGN;
"+="		return YY_PLUS_EQUALS;
"-="		return YY_MINUS_EQUALS;
"*="		return YY_MUL_EQUALS;
"/="		return YY_DIV_EQUALS;
"<<="		return YY_LSHIFT_EQUALS;
">>="		return YY_RSHIFT_EQUALS;
"&="		return YY_BITAND_EQUALS;
"|="		return YY_BITOR_EQUALS;
"^="		return YY_BITXOR_EQUALS;
"||"		return YY_OR;
"&&"		return YY_AND;
"!"		return YY_NOT;
"/"		return YY_DIV;
"&"		return YY_BITAND;
"|"		return YY_BITOR;
"^"		return YY_BITXOR;

"exact"		return YY_IPNET_EQ;
"longer"	return YY_IPNET_LT;
"shorter"	return YY_IPNET_GT;
"orlonger"	return YY_IPNET_LE;
"orshorter"	return YY_IPNET_GE;
"and"		return YY_AND;
"or"		return YY_OR;
"xor"		return YY_XOR;
"not"		return YY_NOT;
"add"		return YY_PLUS_EQUALS;
"sub"		return YY_MINUS_EQUALS;
"mul"		return YY_MUL_EQUALS;
"div"		return YY_DIV_EQUALS;
"lshift"	return YY_LSHIFT_EQUALS;
"rshift"	return YY_RSHIFT_EQUALS;
"bit_and"	return YY_BITAND_EQUALS;
"bit_or"	return YY_BITOR_EQUALS;
"bit_xor"	return YY_BITXOR_EQUALS;
"head"		return YY_HEAD;
"ctr"		return YY_CTR;
"ne_int"	return YY_NE_INT;
"accept"	return YY_ACCEPT;
"reject"	return YY_REJECT;
"SET"		return YY_SET;
"REGEX"		return YY_REGEX;
"protocol"	return YY_PROTOCOL;
"next"		return YY_NEXT;
"policy"	return YY_POLICY;
"term"		return YY_TERM;

[[:alpha:]][[:alnum:]_-]*		{ yylval.c_str = strdup(yytext);
					  return YY_ID;
					}

;		return YY_SEMICOLON;

[[:blank:]]+	/* eat blanks */

"\n"		_parser_lineno++;

.		{ yyerror("Unknown character"); }

%%

void yyerror(const char *m)
{
        ostringstream oss;
        oss << "Error on line " <<  _parser_lineno << " near (";

	for(int i = 0; i < yyleng; i++)
		oss << yytext[i];
	oss << "): " << m;

        _last_error = oss.str();
}

// Everything is put in the lexer because of YY_BUFFER_STATE...
int
policy_parser::policy_parse(vector<Node*>& outnodes, const Term::BLOCKS& block,
			    const string& conf, string& outerr)
{

        YY_BUFFER_STATE yybuffstate = yy_scan_string(conf.c_str());

        _last_error = "No error";
        _parser_nodes = &outnodes;
        _parser_lineno = 1;
	_block = block;

        int res = yyparse();

        yy_delete_buffer(yybuffstate);
        outerr = _last_error;

        return res;
}
