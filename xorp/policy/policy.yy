%{
/*
 * Grammar may be simplified, by allowing "any structure", semantic checking is
 * done at run time anyway...
 * By any structure i mean that you may add / multiple boolean expressions for
 * example. This will give more run time flexibility
 *
 * yacc -d -p yy_policy_parser -o yacc.yy_policy_parser.cc policy.y
 */

#include <vector>

#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "policy/common/element.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"
#include "policy/policy_parser.hh"

extern int  yylex(void);
extern void yyerror(const char *m);

using namespace policy_parser;

static ElementFactory _ef;

%}

%union {
	char*		c_str;
	Node*		node;
	BinOper*	op;
};

%token <c_str> YY_BOOL YY_INT YY_UINT YY_UINTRANGE YY_STR YY_ID
%token <c_str> YY_IPV4 YY_IPV4RANGE YY_IPV4NET YY_IPV6 YY_IPV6RANGE YY_IPV6NET
%token YY_SEMICOLON YY_LPAR YY_RPAR YY_ASSIGN YY_SET YY_REGEX
%token YY_ACCEPT YY_REJECT YY_PROTOCOL YY_NEXT YY_POLICY YY_PLUS_EQUALS
%token YY_MINUS_EQUALS YY_TERM

%left YY_NOT YY_AND YY_XOR YY_OR YY_HEAD YY_CTR YY_NE_INT
%left YY_EQ YY_NE YY_LE YY_GT YY_LT YY_GE
%left YY_IPNET_EQ YY_IPNET_LE YY_IPNET_GT YY_IPNET_LT YY_IPNET_GE
%left YY_ADD YY_SUB
%left YY_MUL

%type <node> actionstatement action boolstatement boolexpr expr assignexpr
%type <op>   assignop
%%

statement:
	  statement actionstatement { _parser_nodes->push_back($2); }
	| statement boolstatement { _parser_nodes->push_back($2); }
	| /* empty */
	;

actionstatement:
	  action YY_SEMICOLON { $$ = $1; }
	;

action:
	  assignexpr
	| YY_ACCEPT { $$ = new NodeAccept(_parser_lineno); }
	| YY_REJECT { $$ = new NodeReject(_parser_lineno); }
	| YY_NEXT YY_POLICY
	  { $$ = new NodeNext(_parser_lineno, NodeNext::POLICY); }
	| YY_NEXT YY_TERM
	  { $$ = new NodeNext(_parser_lineno, NodeNext::TERM); }
	;

assignexpr:
	  YY_ID assignop expr
	  { $$ = new NodeAssign($1, $2, $3, _parser_lineno); free($1); }
	;

assignop:
	  YY_ASSIGN		{ $$ = NULL; }
	| YY_PLUS_EQUALS	{ $$ = new OpAdd; }
	| YY_MINUS_EQUALS	{ $$ = new OpSub; }
	;

boolstatement:
	  boolexpr YY_SEMICOLON { $$ = $1; }
	;

boolexpr:
	  YY_PROTOCOL YY_EQ YY_ID { $$ = new NodeProto($3,_parser_lineno); free($3); }
	| YY_NOT boolexpr { $$ = new NodeUn(new OpNot,$2,_parser_lineno); }
	| YY_POLICY YY_ID { $$ = new NodeSubr(_parser_lineno, $2); free($2); }
	| boolexpr YY_AND boolexpr { $$ = new NodeBin(new OpAnd,$1,$3,_parser_lineno); }
	| boolexpr YY_XOR boolexpr { $$ = new NodeBin(new OpXor,$1,$3,_parser_lineno); }
	| boolexpr YY_OR boolexpr { $$ = new NodeBin(new OpOr,$1,$3,_parser_lineno); }

	| expr YY_EQ expr { $$ = new NodeBin(new OpEq,$1,$3,_parser_lineno); }
	| expr YY_NE expr { $$ = new NodeBin(new OpNe,$1,$3,_parser_lineno); }

	| expr YY_LT expr { $$ = new NodeBin(new OpLt,$1,$3,_parser_lineno); }
	| expr YY_GT expr { $$ = new NodeBin(new OpGt,$1,$3,_parser_lineno); }
	| expr YY_LE expr { $$ = new NodeBin(new OpLe,$1,$3,_parser_lineno); }
	| expr YY_GE expr { $$ = new NodeBin(new OpGe,$1,$3,_parser_lineno); }

	| expr YY_IPNET_EQ expr { $$ = new NodeBin(new OpEq,$1,$3,_parser_lineno); }
	| expr YY_NOT expr { $$ = new NodeBin(new OpNe,$1,$3,_parser_lineno); }
	| expr YY_IPNET_LT expr { $$ = new NodeBin(new OpLt,$1,$3,_parser_lineno); }
	| expr YY_IPNET_GT expr { $$ = new NodeBin(new OpGt,$1,$3,_parser_lineno); }
	| expr YY_IPNET_LE expr { $$ = new NodeBin(new OpLe,$1,$3,_parser_lineno); }
	| expr YY_IPNET_GE expr { $$ = new NodeBin(new OpGe,$1,$3,_parser_lineno); }

	| expr YY_NE_INT expr { $$ = new NodeBin(new OpNEInt, $1, $3, _parser_lineno); }

	| YY_LPAR boolexpr YY_RPAR { $$ = $2; }

	| expr YY_REGEX expr    { $$ = new NodeBin(new OpRegex, $1, $3, _parser_lineno); }
	;

expr:
	  expr YY_ADD expr { $$ = new NodeBin(new OpAdd,$1,$3,_parser_lineno); }
	| expr YY_SUB expr { $$ = new NodeBin(new OpSub,$1,$3,_parser_lineno); }
	| expr YY_MUL expr { $$ = new NodeBin(new OpMul,$1,$3,_parser_lineno); }

	| YY_HEAD expr { $$ = new NodeUn(new OpHead, $2, _parser_lineno); }
	| YY_CTR expr expr { $$ = new NodeBin(new OpCtr, $2, $3, _parser_lineno); }

	| YY_LPAR expr YY_RPAR { $$ = $2; }

	| YY_STR { $$ = new NodeElem(_ef.create(ElemStr::id,$1),_parser_lineno); free($1); }
	| YY_UINT { $$ = new NodeElem(_ef.create(ElemU32::id,$1),_parser_lineno); free($1);}
	| YY_UINTRANGE { $$ = new NodeElem(_ef.create(ElemU32Range::id,$1),_parser_lineno); free($1);}
	| YY_INT { $$ = new NodeElem(_ef.create(ElemInt32::id,$1),_parser_lineno); free($1);}
	| YY_BOOL { $$ = new NodeElem(_ef.create(ElemBool::id,$1),_parser_lineno); free($1);}
	| YY_ID	{ $$ = new NodeVar($1,_parser_lineno); free($1); }
	| YY_SET YY_ID { $$ = new NodeSet($2,_parser_lineno); free($2); }
	| YY_IPV4 { $$ = new NodeElem(_ef.create(ElemIPv4::id,$1),_parser_lineno); free($1); }
	| YY_IPV4RANGE { $$ = new NodeElem(_ef.create(ElemIPv4Range::id,$1),_parser_lineno); free($1); }
	| YY_IPV6 { $$ = new NodeElem(_ef.create(ElemIPv6::id,$1),_parser_lineno); free($1); }
	| YY_IPV6RANGE { $$ = new NodeElem(_ef.create(ElemIPv6Range::id,$1),_parser_lineno); free($1); }
	| YY_IPV4NET { $$ = new NodeElem(_ef.create(ElemIPv4Net::id,$1),_parser_lineno); free($1); }
	| YY_IPV6NET { $$ = new NodeElem(_ef.create(ElemIPv6Net::id,$1),_parser_lineno); free($1); }
        ;
%%
