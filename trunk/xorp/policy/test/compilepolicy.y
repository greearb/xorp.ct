%{
/*
 * yacc -d -p yy_compile_policy -o yacc.yy_compile_policy.cc compilepolicy.y
 */

#include "config.h"
#include <vector>
#include <string>
#include <list>
#include <vector>
#include "policy/test/compilepolicy.hh"
#include "policy/configuration.hh"
#include "policy/common/policy_utils.hh"

extern int yylex(void);
extern void yyerror(const char *);
extern Configuration _yy_configuration;

struct yy_tb {
	string name;
	yy_statements* block[3];
};

static vector<yy_tb*> _yy_terms;
static yy_statements* _yy_statements = NULL;


// add blocks to configuration, and delete stuff from memory
static void add_blocks (const string& pname, const string& tname, yy_tb& term) {
	if(pname == "ao" || tname == "ao"){}
	// source, action, dest
	for(int i = 0; i < 3; i++) {
		yy_statements* statements = term.block[i];

		// empty blocks!
		if(statements == 0)
			continue;

		int order = 0;
		for(yy_statements::iterator j = statements->begin();
		    j != statements->end(); ++j) {

		    yy_statement* statement = *j;

		    _yy_configuration.update_term_block(pname, tname, i, order,
		    					statement->var,
							statement->op,
							statement->arg);
		    delete statement;
		    order++;
		}
		delete statements;
	}
}

%}

%union {
	char *c_str;
	yy_statements* statements;
};

%token <c_str> YY_INT YY_STR YY_ID YY_PAR
%token <c_str> YY_SOURCEBLOCK YY_DESTBLOCK YY_ACTIONBLOCK
%token <c_str> YY_IPV4 YY_IPV4NET YY_IPV6 YY_IPV6NET

%token YY_SEMICOLON YY_LBRACE YY_RBRACE
%token YY_POLICY_STATEMENT YY_TERM
%token YY_SOURCE YY_DEST YY_ACTION
%token YY_SET

%token YY_EXPORT YY_IMPORT

%type <statements> source dest action
%%

configuration:
	  configuration policy_statement
	| configuration set
	| configuration YY_EXPORT YY_ID YY_STR YY_SEMICOLON {
				list<string> tmp;

				string proto = $3;
				string pols = $4;

				free($3);
				free($4);
			
				policy_utils::str_to_list(pols,tmp);

				_yy_configuration.update_exports(proto,tmp);
				}

	| configuration YY_IMPORT YY_ID YY_STR YY_SEMICOLON {
				list<string> tmp;

				string proto = $3;
				string pols = $4;

				free($3);
				free($4);
			
				policy_utils::str_to_list(pols,tmp);

				_yy_configuration.update_imports(proto,tmp);
				}
	| /* empty */
	;

set:
	  YY_SET YY_ID YY_STR YY_SEMICOLON {
	  	string id = $2;
		string sets = $3;

		free($2); free($3);
	  	
		_yy_configuration.create_set(id);
		_yy_configuration.update_set(id,sets);
	  }
	;  

policy_statement:
	  YY_POLICY_STATEMENT YY_ID YY_LBRACE terms YY_RBRACE
	  {
		string pname = $2;
		free($2);

		_yy_configuration.create_policy(pname);
		
		int order = 0;
		for(vector<yy_tb*>::iterator i = _yy_terms.begin();
		    i != _yy_terms.end(); ++i) {

			yy_tb* term = *i;

			string& tname = term->name;
			_yy_configuration.create_term(pname, order, tname);

			add_blocks(pname, tname, *term);

			delete term;
			order++;
		}

	  	_yy_terms.clear();
	  }
	  
	;

terms:
	  terms YY_TERM YY_ID YY_LBRACE source dest action YY_RBRACE 	
	  {
	  	yy_tb* tb = new yy_tb;

		tb->name = $3;
		tb->block[0] = $5;
		tb->block[1] = $6;
		tb->block[2] = $7;
		
		free($3); 
		_yy_terms.push_back(tb);
	  }
	| /* exmpty */
	;

source:
	YY_SOURCE YY_LBRACE statements YY_RBRACE
	{
		yy_statements* tmp = _yy_statements;
		_yy_statements = NULL;
		$$ = tmp;
	}
	;
	
dest:
	YY_DEST YY_LBRACE statements YY_RBRACE
	{
		yy_statements* tmp = _yy_statements;
		_yy_statements = NULL;
		$$ = tmp;
	}
	;

action:
	YY_ACTION YY_LBRACE statements YY_RBRACE
	{
		yy_statements* tmp = _yy_statements;
		_yy_statements = NULL;
		$$ = tmp;
	}
	;

statements:    statements YY_ID YY_STR YY_PAR YY_SEMICOLON
	       {
	       
	       	if (_yy_statements == NULL) {
			_yy_statements = new yy_statements;
		}
		
		yy_statement* statement = new yy_statement;
		statement->var = $2;
		statement->op = $3;
		statement->arg = $4;
		
		free($2);
		free($3);
		free($4);

		_yy_statements->push_back(statement);
	       }
	     | /* empty */
	     ;

%%
