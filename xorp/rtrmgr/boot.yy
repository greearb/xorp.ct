%{
#define YYSTYPE char*

#include <assert.h>
#include <stdio.h>
#include <string>

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "conf_tree.hh"
#include "conf_tree_node.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "config_operators.hh"

/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize bootstacksize
#define yysslim bootsslim

/**
 * Forward declarations
 */
extern void boot_scan_string(const char *configuration);
extern int boot_linenum;
extern "C" int bootparse();
extern int bootlex();

void booterror(const char *s) throw (ParseError);

static ConfigTree *config_tree = NULL;
static string boot_filename;
static string lastsymbol;
static string node_id;

/**
 * Function declarations
 */
static void
extend_path(char* segment, int type, const string& node_id_str);

static void
push_path();

static void
pop_path();

static void
terminal(char* value, int type, ConfigOperator op);

void
booterror(const char *s) throw (ParseError);

int
init_bootfile_parser(const char *configuration,
		     const char *filename,
		     ConfigTree *ct);

void
parse_bootfile() throw (ParseError);

ConfigOperator boot_lookup_operator(const char* s);
%}

%token UPLEVEL
%token DOWNLEVEL
%token END
%left ASSIGN_OPERATOR
%token BOOL_VALUE
%token INT_VALUE
%token UINT_VALUE
%token UINTRANGE_VALUE
%token IPV4_VALUE
%token IPV4RANGE_VALUE
%token IPV4NET_VALUE
%token IPV6_VALUE
%token IPV6RANGE_VALUE
%token IPV6NET_VALUE
%token MACADDR_VALUE
%token URL_FILE_VALUE
%token URL_FTP_VALUE
%token URL_HTTP_VALUE
%token URL_TFTP_VALUE
%token LITERAL
%token STRING
%token ARITH
%token INFIX_OPERATOR
%token SYNTAX_ERROR
%token CONFIG_NODE_ID

%%

input:		/* empty */
		| definition input
		| emptystatement input
		| syntax_error
		;

definition:	long_nodename nodegroup
		| short_nodename long_nodegroup
		;

short_nodename:	literal { push_path(); }
		;

long_nodename:	literals { push_path(); }
		;

literal:	LITERAL { node_id = ""; extend_path($1, NODE_VOID, node_id); }
		| CONFIG_NODE_ID LITERAL { node_id = $1;
					   free($1);
					   extend_path($2, NODE_VOID, node_id); }
		;

literals:	literals literal
		| literal STRING { extend_path($2, NODE_TEXT, node_id); }
		| literal LITERAL { extend_path($2, NODE_TEXT, node_id); }
		| literal BOOL_VALUE { extend_path($2, NODE_BOOL, node_id); }
		| literal UINTRANGE_VALUE { extend_path($2, NODE_UINTRANGE, node_id); }
		| literal INT_VALUE { extend_path($2, NODE_INT, node_id); }
		| literal UINT_VALUE { extend_path($2, NODE_UINT, node_id); }
		| literal IPV4RANGE_VALUE { extend_path($2, NODE_IPV4RANGE, node_id); }
		| literal IPV4_VALUE { extend_path($2, NODE_IPV4, node_id); }
		| literal IPV4NET_VALUE { extend_path($2, NODE_IPV4NET, node_id); }
		| literal IPV6RANGE_VALUE { extend_path($2, NODE_IPV6RANGE, node_id); }
		| literal IPV6_VALUE { extend_path($2, NODE_IPV6, node_id); }
		| literal IPV6NET_VALUE { extend_path($2, NODE_IPV6NET, node_id); }
		| literal MACADDR_VALUE { extend_path($2, NODE_MACADDR, node_id); }
		| literal URL_FILE_VALUE { extend_path($2, NODE_URL_FILE, node_id); }
		| literal URL_FTP_VALUE { extend_path($2, NODE_URL_FTP, node_id); }
		| literal URL_HTTP_VALUE { extend_path($2, NODE_URL_HTTP, node_id); }
		| literal URL_TFTP_VALUE { extend_path($2, NODE_URL_TFTP, node_id); }
		;

nodegroup:	long_nodegroup
		| END { pop_path(); }
		;

long_nodegroup:	UPLEVEL statements DOWNLEVEL { pop_path(); }
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

term_literal:	LITERAL { node_id = ""; extend_path($1, NODE_VOID, node_id); }
		| CONFIG_NODE_ID LITERAL { node_id = $1;
					   free($1);
					   extend_path($2, NODE_VOID, node_id);}
		;

terminal:	term_literal END {
			terminal(strdup(""), NODE_VOID, OP_NONE);
		}
		| term_literal INFIX_OPERATOR STRING END {
			terminal($3, NODE_TEXT, boot_lookup_operator($2)); 
			free($2);
		}
		| term_literal INFIX_OPERATOR BOOL_VALUE END {
			terminal($3, NODE_BOOL, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR UINTRANGE_VALUE END {
			terminal($3, NODE_UINTRANGE, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR UINT_VALUE END {
			terminal($3, NODE_UINT, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR INT_VALUE END {
			terminal($3, NODE_INT, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR IPV4RANGE_VALUE END {
			terminal($3, NODE_IPV4RANGE, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR IPV4_VALUE END {
			terminal($3, NODE_IPV4, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR IPV4NET_VALUE END {
			terminal($3, NODE_IPV4NET, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR IPV6RANGE_VALUE END {
			terminal($3, NODE_IPV6RANGE, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR IPV6_VALUE END {
			terminal($3, NODE_IPV6, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR IPV6NET_VALUE END {
			terminal($3, NODE_IPV6NET, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR MACADDR_VALUE END {
			terminal($3, NODE_MACADDR, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR URL_FILE_VALUE END {
			terminal($3, NODE_URL_FILE, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR URL_FTP_VALUE END {
			terminal($3, NODE_URL_FTP, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR URL_HTTP_VALUE END {
			terminal($3, NODE_URL_HTTP, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR URL_TFTP_VALUE END {
			terminal($3, NODE_URL_TFTP, boot_lookup_operator($2));
			free($2);
		}
		| term_literal INFIX_OPERATOR ARITH END {
			terminal($3, NODE_ARITH, boot_lookup_operator($2));
			free($2);
		}
		;

syntax_error:	SYNTAX_ERROR {
			booterror("syntax error");
		}
		;


%%

void
extend_path(char* segment, int type, const string& node_id_str)
{
    lastsymbol = segment;

    string segment_copy = segment;
    free(segment);

    try {
	ConfigNodeId config_node_id(node_id_str);
	config_tree->extend_path(segment_copy, type, config_node_id);
    } catch (const InvalidString& e) {
	string s = c_format("Invalid config tree node ID: %s",
	    e.str().c_str());
	booterror(s.c_str());
    }
}

void
push_path()
{
    config_tree->push_path();
}

void
pop_path()
{
    config_tree->pop_path();
}

void
terminal(char* value, int type, ConfigOperator op)
{
    push_path();

    lastsymbol = value;

    config_tree->terminal_value(string(value), type, op);
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

ConfigOperator boot_lookup_operator(const char* s)
{
    char *s0, *s1, *s2;

    /* skip leading spaces */
    s0 = strdup(s);
    s1 = s0;
    while (*s1 != '\0' && *s1 == ' ') {
        s1++;
    }

    /* trim trailing spaces */
    s2 = s1;
    while (*s2 != '\0') {
        if (*s2 == ' ') {
            *s2 = 0;
            break;
        }
        s2++;
    }

    ConfigOperator op;
    string str = s1;
    free(s1);
    try {
        op = lookup_operator(str);
	return op;
    } catch (const ParseError& pe) {
        string errmsg = pe.why();
	errmsg += c_format("\n[Line %d]\n", boot_linenum);
	errmsg += c_format("Last symbol parsed was \"%s\"", lastsymbol.c_str());
	xorp_throw(ParseError, errmsg);
    }
    XLOG_UNREACHABLE();
}
