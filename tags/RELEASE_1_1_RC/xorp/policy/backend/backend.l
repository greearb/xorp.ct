%{

#include "config.h"
#include "policy/common/policy_utils.hh"
#include "policy_backend_parser.hh"

#include "yacc.yy_policy_backend_parser.cc.h"
#include <assert.h>
#include <sstream>

#define yyparse yy_policy_backend_parserparse
#define yyerror yy_policy_backend_parsererror
#define yylval yy_policy_backend_parserlval

using namespace policy_utils;
using namespace policy_backend_parser;

void yyerror(const char*);
int yyparse(void);



vector<PolicyInstr*>* policy_backend_parser::_yy_policies;
map<string,Element*>* policy_backend_parser::_yy_sets;

vector<TermInstr*>* policy_backend_parser::_yy_terms;
vector<Instruction*>* policy_backend_parser::_yy_instructions;

namespace {
	string _last_error;
	unsigned _parser_lineno;
}	

%}
%option noyywrap
%option nounput
%option prefix="yy_policy_backend_parser"
%option outfile="lex.yy_policy_backend_parser.cc"
%x STR
%%






"POLICY_START"	{ return YY_POLICY_START; }
"POLICY_END"	{ return YY_POLICY_END; }

"TERM_START"	{ return YY_TERM_START; }
"TERM_END"	{ return YY_TERM_END; }

"SET"		{ return YY_SET; }

"PUSH"		{ return YY_PUSH; }
"PUSH_SET"	{ return YY_PUSH_SET; }

"REGEX"		{ return YY_REGEX; }

"LOAD"		{ return YY_LOAD; }
"STORE"		{ return YY_STORE; }

"ACCEPT"	{ return YY_ACCEPT; }
"REJECT"	{ return YY_REJECT; }

"=="		{ return YY_EQ; }
"!="		{ return YY_NE; }
"<"		{ return YY_LT; }
">"		{ return YY_GT; }
"<="		{ return YY_LE; }
">="		{ return YY_GE; }


"NOT"		{ return YY_NOT; }
"AND"		{ return YY_AND; }
"XOR"		{ return YY_XOR; }
"OR"		{ return YY_OR; }


"+"		{ return YY_ADD; }
"\-"		{ return YY_SUB; }
"*"		{ return YY_MUL; }


"ONFALSE_EXIT"	{ return YY_ONFALSE_EXIT; }

"\n"		{ _parser_lineno++; return YY_NEWLINE; }




[[:blank:]]+	/* eat blanks */



[^\"[:blank:]\n]+	{	
			yylval.c_str = strdup(yytext);
		  	return YY_ARG;
			}  


\"		BEGIN(STR);

<STR>\"		BEGIN(INITIAL);

<STR>[^\"]+	{ yylval.c_str = strdup(yytext);
	 	  _parser_lineno += count_nl(yytext);	
		  return YY_ARG;
		}  



%%


void yyerror(const char *m) {
        ostringstream oss;

        oss << "Error on line " <<  _parser_lineno << ": " << m;

        _last_error = oss.str();
}


int
policy_backend_parser::policy_backend_parse(vector<PolicyInstr*>& outpolicies,
					    map<string,Element*>& outsets,
					    const string& conf,
					    string& outerr) {

	YY_BUFFER_STATE yybuffstate = yy_scan_string(conf.c_str());
	
	_last_error = "No error";
	_parser_lineno = 1;

	_yy_policies = &outpolicies;
	_yy_sets = &outsets;

	_yy_terms = new vector<TermInstr*>();
	_yy_instructions = new vector<Instruction*>();
	
	int res = yyparse();

        yy_delete_buffer(yybuffstate);
        outerr = _last_error;

	// parse error
	if(res) {
		// get rid of temporary parse object not yet bound to policies
		delete_vector(_yy_terms);
	        delete_vector(_yy_instructions);
	} 
	// good parse
	else {
		// all terms should be bound to policies
		assert(_yy_terms->empty());
		delete _yy_terms;

	        // all instructions should be bound to terms
		assert(_yy_instructions->empty());
	        delete _yy_instructions;
	}

	return res;
} 
