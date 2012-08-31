%{
/*
 * yacc -d -p yy_policy_backend_parser -o yacc.yy_policy_backend_parser.cc backend.y
 */

#include <vector>

#include "libxorp/xorp.h"
#include "policy/common/varrw.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"
#include "policy/backend/policy_backend_parser.hh"
#include "policy/backend/instruction.hh"
#include "policy/backend/term_instr.hh"
#include "policy/backend/policy_instr.hh"

extern int  yylex(void);
extern void yyerror(const char*);

using namespace policy_backend_parser;

static ElementFactory	_ef;

%}

%union {
	char*		c_str;
	PolicyInstr*	c_pi;
};

%token <c_str> YY_ARG
%token YY_NEWLINE YY_BLANK
%token YY_POLICY_START YY_POLICY_END YY_TERM_START YY_TERM_END
%token YY_PUSH YY_PUSH_SET
%token YY_EQ YY_NE YY_LT YY_GT YY_LE YY_GE
%token YY_NOT YY_AND YY_OR YY_XOR YY_HEAD YY_CTR YY_NE_INT
%token YY_ADD YY_SUB YY_MUL
%token YY_ONFALSE_EXIT
%token YY_REGEX
%token YY_LOAD YY_STORE
%token YY_ACCEPT YY_REJECT
%token YY_SET YY_NEXT YY_POLICY YY_SUBR_START YY_SUBR_END YY_TERM

%type <c_pi> policy
%%

program:
	  program policy { _yy_policies->push_back($2); }
	| program subroutine
	| program set
	| /* empty */
	;

set:
	  YY_SET YY_ARG YY_ARG YY_ARG YY_NEWLINE
	  {
		// XXX: doesn't delete old
		(*_yy_sets)[$3] = _ef.create($2, $4);
		free($2); free($3); free($4);
	  }
	;

subroutine:
	  YY_SUBR_START YY_NEWLINE policies YY_SUBR_END YY_NEWLINE
	;

policies:
	  policies policy { (*_yy_subr)[$2->name()] = $2; }
	| /* empty */
	;

policy:	  YY_POLICY_START YY_ARG YY_NEWLINE terms YY_POLICY_END YY_NEWLINE
	{
		PolicyInstr* pi = new PolicyInstr($2,_yy_terms);

		pi->set_trace(_yy_trace);
		_yy_trace = false;

		_yy_terms = new vector<TermInstr*>();

		free($2);

		$$ = pi;
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

	| YY_LOAD YY_ARG	{
				char* err = NULL;
				bool is_error = false;
				VarRW::Id id = strtoul($2, &err, 10);
				if ((err != NULL) && (*err != '\0'))
				    is_error = true;
				free($2);
				if (is_error) {
					yyerror("Need numeric var ID");
					YYERROR;
				}
				_yy_instructions->push_back(new Load(id));
				}

	| YY_STORE YY_ARG	{
				char* err = NULL;
				bool is_error = false;
				VarRW::Id id = strtoul($2, &err, 10);

				if ((err != NULL) && (*err != '\0'))
				    is_error = true;

				free($2);

				if (is_error) {
					yyerror("Need numeric var ID");
					YYERROR;
				}

				if (id == VarRW::VAR_TRACE)
					_yy_trace = true;

				_yy_instructions->push_back(new Store(id));
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
	| YY_HEAD	{ _yy_instructions->push_back(new NaryInstr(new OpHead));}
	| YY_CTR	{ _yy_instructions->push_back(new NaryInstr(new OpCtr));}
	| YY_NE_INT	{ _yy_instructions->push_back(new NaryInstr(new OpNEInt));}
	| YY_REGEX	{ _yy_instructions->push_back(new NaryInstr(new OpRegex));}
	| YY_NEXT YY_POLICY
	{ _yy_instructions->push_back(new Next(Next::POLICY)); }
	| YY_NEXT YY_TERM
	{ _yy_instructions->push_back(new Next(Next::TERM)); }
	| YY_POLICY YY_ARG
	{ _yy_instructions->push_back(new Subr($2)); free($2); }
	;
%%
