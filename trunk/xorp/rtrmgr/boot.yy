%{
#define YYSTYPE char*
#include <assert.h>
#include <stdio.h>
#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_tree.hh"
#include "conf_tree.hh"
/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize bootstacksize
#define yysslim bootsslim
%}

%token UPLEVEL
%token DOWNLEVEL
%token END
%left ASSIGN_OPERATOR
%token BOOL_VALUE
%token UINT_VALUE
%token IPV4_VALUE
%token IPV4NET_VALUE
%token IPV6_VALUE
%token IPV6NET_VALUE
%token MACADDR_VALUE
%token LITERAL
%token STRING
%token SYNTAX_ERROR


%%

input:		/* empty */
		| definition input
		| emptystatement input
		| syntax_error
		;

definition:	nodename nodegroup
		;

nodename:	literals { push_path(); }
		;

literals:	LITERAL { extend_path($1); }
		| literals LITERAL { extend_path($2); }
		| literals BOOL_VALUE { extend_path($2); }
		| literals UINT_VALUE { extend_path($2); }
		| literals IPV4_VALUE { extend_path($2); }
		| literals IPV4NET_VALUE { extend_path($2); }
		| literals IPV6_VALUE { extend_path($2); }
		| literals IPV6NET_VALUE { extend_path($2); }
		| literals MACADDR_VALUE { extend_path($2); }
		;

nodegroup:	UPLEVEL statements DOWNLEVEL { pop_path(); }
		| END { pop_path(); }
		;

statements:	/* empty string */
		| statement statements
		;

statement:	terminal
		| definition
		| emptystatement
		;

emptystatement:	END
		;

terminal:	LITERAL END {
			terminal($1, strdup(""), NODE_VOID);
		}
		| LITERAL ASSIGN_OPERATOR STRING END {
			terminal($1, $3, NODE_TEXT);
		}
		| LITERAL ASSIGN_OPERATOR BOOL_VALUE END {
			terminal($1, $3, NODE_BOOL);
		}
		| LITERAL ASSIGN_OPERATOR UINT_VALUE END {
			terminal($1, $3, NODE_UINT);
		}
		| LITERAL ASSIGN_OPERATOR IPV4_VALUE END {
			terminal($1, $3, NODE_IPV4);
		}
		| LITERAL ASSIGN_OPERATOR IPV4NET_VALUE END {
			terminal($1, $3, NODE_IPV4PREFIX);
		}
		| LITERAL ASSIGN_OPERATOR IPV6_VALUE END {
			terminal($1, $3, NODE_IPV6);
		}
		| LITERAL ASSIGN_OPERATOR IPV6NET_VALUE END {
			terminal($1, $3, NODE_IPV6PREFIX);
		}
		| LITERAL ASSIGN_OPERATOR MACADDR_VALUE END {
			terminal($1, $3, NODE_MACADDR);
		}
		;

syntax_error:	SYNTAX_ERROR {
			booterror("syntax error");
		}
		;


%%

extern void boot_scan_string(const char *configuration);
extern int boot_linenum;
extern "C" int bootparse();
extern int bootlex();

static ConfigTree *config_tree = NULL;
static string boot_filename;
static string lastsymbol;


static void
extend_path(char *segment)
{
    lastsymbol = segment;

    config_tree->extend_path(string(segment));
    free(segment);
}

static void
push_path()
{
    config_tree->push_path();
}

static void
pop_path()
{
    config_tree->pop_path();
}

static void
terminal(char *segment, char *value, int type)
{
    extend_path(segment);
    push_path();

    lastsymbol = value;

    config_tree->terminal_value(value, type);
    free(value);
    pop_path();
}

void
booterror(const char *s) throw (ParseError)
{
    string errmsg;

    if (! boot_filename.empty()) {
	errmsg = c_format("PARSE ERROR [Config File %s, line %d]: %s",
			  boot_filename.c_str(),
			  boot_linenum, s);
    } else {
	errmsg = c_format("PARSE ERROR [line %d]: %s", boot_linenum, s);
    }
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());

    xorp_throw(ParseError, errmsg);
}

int
init_bootfile_parser(const char *configuration,
		     const char *filename,
		     ConfigTree *ct)
{
    config_tree = ct;
    boot_filename = filename;
    boot_linenum = 1;
    boot_scan_string(configuration);
    return 0;
}

void
parse_bootfile() throw (ParseError)
{
    if (bootparse() != 0)
	booterror("unknown error");
}
