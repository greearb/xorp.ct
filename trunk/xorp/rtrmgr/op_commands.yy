%{
#define YYSTYPE char*
#include <assert.h>
#include <stdio.h>
#include "rtrmgr_module.h"
#include "op_commands.hh"
/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize opcmdstacksize
#define yysslim opcmdsslim
%}

%token UPLEVEL
%token DOWNLEVEL
%token END
%left COLON
%token COMMAND
%token VARIABLE
%token LITERAL
%token SYNTAX_ERROR


%%

input:			/* empty */
			| definition input
			| syntax_error
			;

definition:		word word_or_variable_list specification { }
			;

word_or_variable_list:	/* empty */
			| word_or_variable word_or_variable_list { }
			;

word_or_variable:	word
			| variable
			;

word:			LITERAL { append_path($1); }
			;

variable:		VARIABLE { append_path($1); }
			;

specification:		startspec attributes endspec { }
			;

startspec:		UPLEVEL { push_path(); }
			;

endspec:		DOWNLEVEL { pop_path(); }
			;

attributes:		attribute
			| attribute attributes
			;

attribute:		COMMAND COLON LITERAL END {
				add_cmd($1);
				append_cmd($3);
				end_cmd();
			}
			;

syntax_error:		SYNTAX_ERROR { opcmderror("syntax error"); }
			;


%%

extern "C" int opcmdparse();
extern int opcmdlex();
extern FILE *opcmdin;
extern int opcmd_linenum;

static OpCommandList *ocl = NULL;
static string opcmd_filename;
static string lastsymbol;
static string current_cmd;
static list<string> cmd_list;


static void
append_path(char *segment)
{
    lastsymbol = segment;

    string segname;
    segname = segment;
    ocl->append_path(segname);
    free(segment);
}

static
void push_path()
{
    ocl->push_path();
}

static void
pop_path()
{
    ocl->pop_path();
}

static void
add_cmd(char *cmd)
{
    lastsymbol = cmd;

    ocl->add_cmd(cmd);
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
end_cmd()
{
    ocl->add_cmd_action(current_cmd, cmd_list);
    cmd_list.clear();
}

void
opcmderror(const char *s) throw (ParseError)
{
    string errmsg;

    errmsg = c_format("PARSE ERROR [Operational Command File: %s line %d]: %s",
		      opcmd_filename.c_str(), opcmd_linenum, s);
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());

    xorp_throw(ParseError, errmsg);
}

int
init_opcmd_parser(const char *filename, OpCommandList *o)
{
    ocl = o;
    opcmd_linenum = 1;

    opcmdin = fopen(filename, "r");
    if (opcmdin == NULL)
	return -1;

    opcmd_filename = filename;
    return 0;
}

void
parse_opcmd() throw (ParseError)
{
    if (opcmdparse() != 0)
	opcmderror("unknown error");
}
