%{
#define YYSTYPE char*
#include <assert.h>
#include <stdio.h>
#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_tree.hh"
/* XXX: sigh, the -p flag to yacc should do this for us */
#define yystacksize tpltstacksize
#define yysslim tpltsslim
%}

%token UPLEVEL
%token DOWNLEVEL
%token END
%left COLON
%left ASSIGN_DEFAULT
%token LISTNEXT
%token RETURN
%token TEXT_TYPE
%token INT_TYPE
%token UINT_TYPE
%token BOOL_TYPE
%token TOGGLE_TYPE
%token IPV4_TYPE
%token IPV4NET_TYPE
%token IPV6_TYPE
%token IPV6NET_TYPE
%token MACADDR_TYPE
%token BOOL_VALUE
%token INTEGER_VALUE
%token IPV4_VALUE
%token IPV4NET_VALUE
%token IPV6_VALUE
%token IPV6NET_VALUE
%token MACADDR_VALUE
%token VARDEF
%token COMMAND
%token VARIABLE
%token LITERAL
%token STRING
%token SYNTAX_ERROR


%%

input:		/* empty */
		| definition input
		| syntax_error
		;

definition:	nodename nodegroup
		;

nodename:	literals { push_path(); }
		| named_literal { push_path(); }
		;

named_literal:	LITERAL VARDEF {
			extend_path($1, true);
			extend_path($2, false);
		}
		| LITERAL VARDEF COLON type {
			extend_path($1, true);
			extend_path($2, false);
		}
		;

literals:	LITERAL { extend_path($1, false); }
		| literals LITERAL { extend_path($2, false); }
		| literals named_literal
		;

type:		TEXT_TYPE { tplt_type = NODE_TEXT; }
		| INT_TYPE { tplt_type = NODE_INT; }
		| UINT_TYPE { tplt_type = NODE_UINT; }
		| BOOL_TYPE { tplt_type = NODE_BOOL; }
		| TOGGLE_TYPE { tplt_type = NODE_TOGGLE; }
		| IPV4_TYPE { tplt_type = NODE_IPV4; }
		| IPV4NET_TYPE { tplt_type = NODE_IPV4NET; }
		| IPV6_TYPE { tplt_type = NODE_IPV6; }
		| IPV6NET_TYPE { tplt_type = NODE_IPV6NET; }
		| MACADDR_TYPE { tplt_type = NODE_MACADDR; }
		;

init_type:	TEXT_TYPE ASSIGN_DEFAULT STRING {
			tplt_type = NODE_TEXT;
			tplt_initializer = $3;
		}
		| INT_TYPE ASSIGN_DEFAULT INTEGER_VALUE {
			tplt_type = NODE_INT;
			tplt_initializer = $3;
		}
		| UINT_TYPE ASSIGN_DEFAULT INTEGER_VALUE {
			tplt_type = NODE_UINT;
			tplt_initializer = $3;
		}
		| BOOL_TYPE ASSIGN_DEFAULT BOOL_VALUE {
			tplt_type = NODE_BOOL;
			tplt_initializer = $3;
		}
		| TOGGLE_TYPE ASSIGN_DEFAULT BOOL_VALUE {
			tplt_type = NODE_TOGGLE;
			tplt_initializer = $3;
		}
		| IPV4_TYPE ASSIGN_DEFAULT IPV4_VALUE {
			tplt_type = NODE_IPV4;
			tplt_initializer = $3;
		}
		| IPV4NET_TYPE ASSIGN_DEFAULT IPV4NET_VALUE {
			tplt_type = NODE_IPV4NET;
			tplt_initializer = $3;
		}
		| IPV6_TYPE ASSIGN_DEFAULT IPV6_VALUE {
			tplt_type = NODE_IPV6;
			tplt_initializer = $3;
		}
		| IPV6NET_TYPE ASSIGN_DEFAULT IPV6NET_VALUE {
			tplt_type = NODE_IPV6NET;
			tplt_initializer = $3;
		}
		| MACADDR_TYPE ASSIGN_DEFAULT MACADDR_VALUE {
			tplt_type = NODE_MACADDR;
			tplt_initializer = $3;
		}
		;

nodegroup:	UPLEVEL statements DOWNLEVEL { pop_path(); }
		;

statements:	/* empty string */
		| statement statements
		;

statement:	terminal
		| command
		| definition
		;

terminal:	default_terminal
		| regular_terminal
		;

regular_terminal:	LITERAL COLON type END { terminal($1); }
		;

default_terminal:	LITERAL COLON init_type END { terminal($1); }
		;

command:	cmd_val
		| cmd_default
		;

cmd_val:	command_name COLON cmd_list END
		;

cmd_default:	command_name COLON END
		;

command_name:	COMMAND { add_cmd($1); }
		;

cmd_list:	cmd
		| cmd_list LISTNEXT cmd
		;

cmd:		LITERAL list_of_cmd_strings {
			prepend_cmd($1);
			end_cmd();
		}
		| LITERAL STRING RETURN STRING {
			append_cmd($1);
			append_cmd($2);
			append_cmd($3);
			append_cmd($4);
			end_cmd();
		}
		| LITERAL VARIABLE type { /* e.g.: set FOOBAR ipv4 */
			append_cmd($1);
			append_cmd($2);
			append_cmd($3);
			end_cmd();
		}
		| LITERAL LITERAL {
			append_cmd($1);
			append_cmd($2);
			end_cmd();
		}
                | LITERAL LITERAL STRING {
			append_cmd($1);
			append_cmd($2);
			append_cmd($3);
			end_cmd();
		}
		| VARIABLE {
			append_cmd($1);
			end_cmd();
		}
		| VARIABLE list_of_cmd_strings {
			prepend_cmd($1);
			end_cmd();
		}
		| LITERAL {
			append_cmd($1);
			end_cmd();
		}
		;

list_of_cmd_strings:
		STRING {
			append_cmd($1);
		}
		| list_of_cmd_strings STRING {
			append_cmd($2);
		}
		;

syntax_error:	SYNTAX_ERROR {
			tplterror("syntax error");
		}
		;


%%

extern char *lstr;
extern char *vstr;
extern char *cstr;
extern char *sstr;
extern char *istr;
extern FILE *tpltin;
extern int tplt_linenum;
extern "C" int tpltparse();
extern int tpltlex();

static TemplateTree* tt = NULL;
static string tplt_filename;
static string lastsymbol;
static int tplt_type;
static char *tplt_initializer = NULL;
static string current_cmd;
static list<string> cmd_list;


static void
extend_path(char *segment, bool is_tag)
{
    lastsymbol = segment;

    string segname;
    segname = segment;
    tt->extend_path(segname, is_tag);
    free(segment);
}

static void
push_path()
{
    tt->push_path(tplt_type, tplt_initializer);
    tplt_type = NODE_VOID;
    if (tplt_initializer != NULL) {
	free(tplt_initializer);
	tplt_initializer = NULL;
    }
}

static void
pop_path()
{
    tt->pop_path();
    tplt_type = NODE_VOID;
    if (tplt_initializer != NULL) {
	free(tplt_initializer);
	tplt_initializer = NULL;
    }
}

static void
terminal(char *segment)
{
    extend_path(segment, false);
    push_path();
    pop_path();
}

static void
add_cmd(char *cmd)
{
    lastsymbol = cmd;

    tt->add_cmd(cmd);
    current_cmd = cmd;
    free(cmd);
    cmd_list.clear();
}

static void
append_cmd(char *s)
{
    lastsymbol = s;

    cmd_list.push_back(string(s));
    free(s);
}

static void
prepend_cmd(char *s)
{
    lastsymbol = s;

    cmd_list.push_front(string(s));
    free(s);
}

static void
end_cmd()
{
    tt->add_cmd_action(current_cmd, cmd_list);
    cmd_list.clear();
}

void
tplterror(const char *s) throw (ParseError)
{
    string errmsg;

    errmsg = c_format("PARSE ERROR [Template File: %s line %d]: %s",
		      tplt_filename.c_str(), tplt_linenum, s);
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());

    xorp_throw(ParseError, errmsg);
}

int
init_template_parser(const char *filename, TemplateTree *c)
{
    tt = c;
    tplt_linenum = 1;

    tpltin = fopen(filename, "r");
    if (tpltin == NULL)
	return -1;

    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
    tplt_filename = filename;
    return 0;
}

void
parse_template() throw (ParseError)
{
    if (tpltparse() != 0)
	tplterror("unknown error");
}
