%{
#define YYSTYPE char*
#include <assert.h>
#include <stdio.h>
#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_tree.hh"
#include "conf_tree.hh"
  /*sigh - -p flag to yacc should do this for us*/
#define yystacksize bootstacksize
#define yysslim bootsslim
%}

%token STRING
%token UPLEVEL
%token DOWNLEVEL
%left ASSIGN_VALUE
%left ASSIGN_DEFAULT
%token END
%token LITERAL
%token SLASH
%token LISTNEXT
%token TEXT
%token UINT
%token INTEGER
%token BOOL
%token IPV4
%token IPV4NET
%token IPV6
%token IPV6NET

%%
input:	/*empty*/
	| definition input
	| emptystatement input

definition:	nodename nodegroup {
                   /*printf("DEFINITION\n");*/
                }

nodename:	literals {push_path(); }

literals:	LITERAL { extend_path($1); }		
                | literals LITERAL { extend_path($2); }
		| literals IPV4 { extend_path($2); }
		| literals IPV4NET { extend_path($2); }
		| literals IPV6 { extend_path($2); }
		| literals IPV6NET { extend_path($2); }
		| literals INTEGER { extend_path($2); }

nodegroup:	UPLEVEL statements DOWNLEVEL { pop_path(); }
		| END {pop_path();}

statements:	/*empty string*/
		| statement statements

statement:	terminal | definition | emptystatement

emptystatement:	END {/*printf("EMPTY STATEMENT\n");*/}

terminal:	LITERAL END { 
		    terminal($1, strdup(""), NODE_VOID);
		}
		| LITERAL ASSIGN_VALUE STRING END { 
		    terminal($1, $3, NODE_TEXT);
                }
		| LITERAL ASSIGN_VALUE INTEGER END { 
		    terminal($1, $3, NODE_UINT);
                }
		| LITERAL ASSIGN_VALUE BOOL END { 
		    terminal($1, $3, NODE_BOOL);
                }
		| LITERAL ASSIGN_VALUE IPV4 END { 
		    terminal($1, $3, NODE_IPV4);
                }
		| LITERAL ASSIGN_VALUE IPV4NET END { 
		    terminal($1, $3, NODE_IPV4PREFIX);
                }
		| LITERAL ASSIGN_VALUE IPV6 END { 
		    terminal($1, $3, NODE_IPV6);
                }
		| LITERAL ASSIGN_VALUE IPV6NET END { 
		    terminal($1, $3, NODE_IPV6PREFIX);
                }



%%

/*extern FILE *bootin;*/
extern void boot_scan_string(const char *);
extern int bootlinenum;

#define MAXSTACK 20
#define MAXPATH 256
static ConfigTree *cf;
static string boot_filename;
static char lastsymbol[256];
extern "C" int bootparse();
extern int bootlex();


string booterrormsg(const char *s) {
  string errmsg;
  if (boot_filename.empty()) {
    errmsg = c_format("PARSE ERROR [Config File %s, line %d]: %s\n", 
		      boot_filename.c_str(),
		      bootlinenum, s);
  } else {
    errmsg = c_format("PARSE ERROR [line %d]: %s\n", bootlinenum, s);
  }
  return errmsg;
}

void booterror(const char *s) {
  xorp_throw(ParseError, booterrormsg(s));
}


static void extend_path(char *segment) {
    strncpy(lastsymbol, segment, 255);
    cf->extend_path(string(segment));
    free(segment);
}


static void push_path() {
    cf->push_path();
}

static void pop_path() {
    cf->pop_path();
}

static void terminal(char *segment, char *value, int type) {
    extend_path(segment);
    push_path();
    strncpy(lastsymbol, value, 255);
    cf->terminal_value(value, type);
    free(value);
    pop_path();
}

int init_bootfile_parser (const char *configuration, 
			  const char *filename, 
			  ConfigTree *c) {
    cf = c;
    boot_filename = filename;
    bootlinenum = 1;
    boot_scan_string(configuration);
    return 0;
}

int parse_bootfile() {
    bootparse();
    return 0;
}
