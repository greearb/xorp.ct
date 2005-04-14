%{
/*
 * Grammar may be simplified, by allowing "any structure", semantic checking is
 * done at run time anyway...
 * By any structure i mean that you may add / multiple boolean expressions for
 * example. This will give more run time flexibility
 *
 * yacc -d -p yy_policy_parser -o yacc.yy_policy_parser.cc policy.y
 *
 * XXX: with my version of yacc i need to move the #include <stdlib.h> under the
 * yyrcsid
 */

#include "config.h"

#include "policy/common/element.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"
#include "policy_parser.hh"

#include <vector>
#include <string>


extern int yylex(void);

extern void yyerror(const char *m);

using namespace policy_parser;

static ElementFactory _ef;

%}

%union {
	char *c_str;
	Node *node;
};



%token <c_str> YY_INT YY_UINT YY_STR YY_ID 
%token <c_str> YY_IPV4 YY_IPV4NET YY_IPV6 YY_IPV6NET

%left YY_NOT YY_AND YY_XOR YY_OR
%left YY_EQ YY_NE YY_LE YY_GT YY_LT YY_LE YY_GE
%left YY_ADD YY_SUB
%left YY_MUL

%token YY_SEMICOLON YY_LPAR YY_RPAR

%token YY_MODIFY YY_ASSIGN
%token YY_SET YY_REGEX

%token YY_ACCEPT YY_REJECT

%token YY_PROTOCOL

%type <node> actionstatement boolstatement boolexpr expr
%%

statement:
	  statement actionstatement { _parser_nodes->push_back($2); }
	| statement boolstatement { _parser_nodes->push_back($2); }  
	| /* empty */
	;

actionstatement:
	  YY_MODIFY YY_ID YY_ASSIGN expr YY_SEMICOLON { 
	  			$$ = new NodeAssign($2,$4,_parser_lineno); free($2); }  
	| YY_ACCEPT YY_SEMICOLON { $$ = new NodeAccept(_parser_lineno); }
	| YY_REJECT YY_SEMICOLON { $$ = new NodeReject(_parser_lineno); }
	; 

boolstatement:
	  boolexpr YY_SEMICOLON { $$ = $1; }
	;


boolexpr:
	  YY_PROTOCOL YY_ID { $$ = new NodeProto($2,_parser_lineno); free($2); }
	| YY_NOT boolexpr { $$ = new NodeUn(new OpNot,$2,_parser_lineno); }
	| boolexpr YY_AND boolexpr { $$ = new NodeBin(new OpAnd,$1,$3,_parser_lineno); }
	| boolexpr YY_XOR boolexpr { $$ = new NodeBin(new OpXor,$1,$3,_parser_lineno); }
	| boolexpr YY_OR boolexpr { $$ = new NodeBin(new OpOr,$1,$3,_parser_lineno); }

	| expr YY_EQ expr { $$ = new NodeBin(new OpEq,$1,$3,_parser_lineno); }
	| expr YY_NE expr { $$ = new NodeBin(new OpNe,$1,$3,_parser_lineno); }
	
	| expr YY_LT expr { $$ = new NodeBin(new OpLt,$1,$3,_parser_lineno); }
	| expr YY_GT expr { $$ = new NodeBin(new OpGt,$1,$3,_parser_lineno); }
	| expr YY_LE expr { $$ = new NodeBin(new OpLe,$1,$3,_parser_lineno); }
	| expr YY_GE expr { $$ = new NodeBin(new OpGe,$1,$3,_parser_lineno); }


	| YY_LPAR boolexpr YY_RPAR { $$ = $2; }
	
	| expr YY_REGEX YY_STR { $$ = new NodeRegex($1,$3,_parser_lineno);
					free($3); 
				}
	;

expr:	
	  expr YY_ADD expr { $$ = new NodeBin(new OpAdd,$1,$3,_parser_lineno); }
	| expr YY_SUB expr { $$ = new NodeBin(new OpSub,$1,$3,_parser_lineno); }
	| expr YY_MUL expr { $$ = new NodeBin(new OpMul,$1,$3,_parser_lineno); }
	
	| YY_LPAR expr YY_RPAR { $$ = $2; }

	| YY_STR { $$ = new NodeElem(_ef.create(ElemStr::id,$1),_parser_lineno); free($1); }
	| YY_UINT { $$ = new NodeElem(_ef.create(ElemU32::id,$1),_parser_lineno); free($1);}
	| YY_INT { $$ = new NodeElem(_ef.create(ElemInt32::id,$1),_parser_lineno); free($1);}
	| YY_ID	{ $$ = new NodeVar($1,_parser_lineno); free($1); }
	| YY_SET YY_ID { $$ = new NodeSet($2,_parser_lineno); free($2); }
	| YY_IPV4 { $$ = new NodeElem(_ef.create(ElemIPv4::id,$1),_parser_lineno); free($1); }
	| YY_IPV6 { $$ = new NodeElem(_ef.create(ElemIPv6::id,$1),_parser_lineno); free($1); }
	| YY_IPV4NET { $$ = new NodeElem(_ef.create(ElemIPv4Net::id,$1),_parser_lineno); free($1); }
	| YY_IPV6NET { $$ = new NodeElem(_ef.create(ElemIPv6Net::id,$1),_parser_lineno); free($1); }
        ;



%%
