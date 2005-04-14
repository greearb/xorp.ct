%{
/*
 * yacc -d -p yy_compile_policy -o yacc.yy_compile_policy.cc compilepolicy.y
 */

#include "config.h"

#include <vector>
#include <string>
#include <list>
#include <vector>

#include "policy/configuration.hh"
#include "policy/common/policy_utils.hh"

extern int yylex(void);
extern void yyerror(const char *);
extern Configuration _yy_configuration;

struct yy_tb {
	string name;
	string source;
	string dest;
	string action;

	yy_tb(const string& n, 
	      const string& s,
	      const string& d,
	      const string& a) : name(n),
				 source(s),
				 dest(d),
				 action(a) {}
};

static vector<yy_tb*> _yy_terms;

%}

%union {
	char *c_str;
};


%token <c_str> YY_INT YY_STR YY_ID 
%token <c_str> YY_SOURCEBLOCK YY_DESTBLOCK YY_ACTIONBLOCK
%token <c_str> YY_IPV4 YY_IPV4NET YY_IPV6 YY_IPV6NET

%token YY_SEMICOLON YY_LBRACE YY_RBRACE
%token YY_POLICY_STATEMENT YY_TERM

%token YY_SET

%token YY_EXPORT YY_IMPORT

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

		for(vector<yy_tb*>::iterator i = _yy_terms.begin();
		    i != _yy_terms.end(); ++i) {

			yy_tb* term = *i;

			string& tname = term->name;
			_yy_configuration.create_term(pname,tname);
			_yy_configuration.update_term_source(pname,tname,term->source);
			_yy_configuration.update_term_dest(pname,tname,term->dest);
			_yy_configuration.update_term_action(pname,tname,term->action);

			delete term;
		}

	  	_yy_terms.clear();
	  }
	  
	;

terms:
	  terms YY_TERM YY_ID YY_LBRACE YY_SOURCEBLOCK YY_DESTBLOCK YY_ACTIONBLOCK YY_RBRACE 	
	  {
	  	yy_tb* tb = new yy_tb($3,$5,$6,$7);
		
		free($3); free($5); free($6), free($7);

		_yy_terms.push_back(tb);
	  }
	| /* exmpty */
	;  

%%
