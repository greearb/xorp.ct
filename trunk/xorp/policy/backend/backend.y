%{
/*
 * yacc -d -p yy_policy_backend_parser -o yacc.yy_policy_backend_parser.cc backend.y
 */

#include "config.h"
#include "policy_backend_parser.hh"

#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"

#include "instruction.hh"
#include "term_instr.hh"
#include "policy_instr.hh"

#include <vector>

extern int yylex(void);
extern void yyerror(const char*);

using namespace policy_backend_parser;

static ElementFactory _ef;

%}

%union {
	char* c_str;
};

%token <c_str> YY_ARG

%token YY_NEWLINE YY_BLANK

%token YY_POLICY_START YY_POLICY_END YY_TERM_START YY_TERM_END


%token YY_PUSH YY_PUSH_SET

%token YY_EQ YY_NE YY_LT YY_GT YY_LE YY_GE
%token YY_NOT YY_AND YY_OR YY_XOR

%token YY_ADD YY_SUB YY_MUL

%token YY_ONFALSE_EXIT

%token YY_REGEX

%token YY_LOAD YY_STORE

%token YY_ACCEPT YY_REJECT

%token YY_SET
%%

program:
	  program policy
	| program set  
	| /* empty */
	;

set:
	  YY_SET YY_ARG YY_ARG YY_NEWLINE 
	  {
	  	// XXX: doesn't delete old
		(*_yy_sets)[$2] = _ef.create("set",$3);
		free($2); free($3);
	  }
	;  

policy:	  YY_POLICY_START YY_ARG YY_NEWLINE terms YY_POLICY_END YY_NEWLINE {
			PolicyInstr* pi = new PolicyInstr($2,_yy_terms);
			_yy_terms = new vector<TermInstr*>();
			_yy_policies->push_back(pi);
			free($2);
			}
	;

terms:
	  terms YY_TERM_START YY_ARG YY_NEWLINE statements YY_TERM_END YY_NEWLINE {
	  
			TermInstr* ti = new TermInstr($3,_yy_instructions);
			_yy_instructions = new vector<Instruction*>();
			_yy_terms->push_back(ti);
			free($3);
			}
	| /* empty */		
	;

statements: 
	  statements statement YY_NEWLINE
	| /* empty */
	;


statement:
	  YY_PUSH YY_ARG YY_ARG {
	  			Instruction* i = new Push(_ef.create($2,$3));
				_yy_instructions->push_back(i);
	  			free($2); free($3);
	  			}
	| YY_PUSH_SET YY_ARG	{
				_yy_instructions->push_back(new PushSet($2));
				free($2);
				}
	
	| YY_ONFALSE_EXIT	{
				_yy_instructions->push_back(new OnFalseExit());
				}

	| YY_REGEX YY_ARG	{
				_yy_instructions->push_back(new Regex($2));
				free($2);
				}

	| YY_LOAD YY_ARG	{
				_yy_instructions->push_back(new Load($2));
				free($2);
				}
	| YY_STORE YY_ARG	{
				_yy_instructions->push_back(new Store($2));
				free($2);
				}

	| YY_ACCEPT		{ _yy_instructions->push_back(new Accept()); }
	| YY_REJECT		{ _yy_instructions->push_back(new Reject()); }

	| YY_EQ		{ _yy_instructions->push_back(new NaryInstr(new OpEq)); }
	| YY_NE		{ _yy_instructions->push_back(new NaryInstr(new OpNe)); }
	| YY_LT		{ _yy_instructions->push_back(new NaryInstr(new OpLt)); }
	| YY_GT		{ _yy_instructions->push_back(new NaryInstr(new OpGt)); }
	| YY_LE		{ _yy_instructions->push_back(new NaryInstr(new OpLe)); }
	| YY_GE		{ _yy_instructions->push_back(new NaryInstr(new OpGe)); }

	| YY_NOT	{ _yy_instructions->push_back(new NaryInstr(new OpNot)); }
	| YY_AND	{ _yy_instructions->push_back(new NaryInstr(new OpAnd)); }
	| YY_XOR	{ _yy_instructions->push_back(new NaryInstr(new OpXor)); }
	| YY_OR		{ _yy_instructions->push_back(new NaryInstr(new OpOr)); }

	| YY_ADD	{ _yy_instructions->push_back(new NaryInstr(new OpAdd)); }
	| YY_SUB	{ _yy_instructions->push_back(new NaryInstr(new OpSub)); }
	| YY_MUL	{ _yy_instructions->push_back(new NaryInstr(new OpMul)); }

	;  

%%
