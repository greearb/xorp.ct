%{
#define YYSTYPE char*
#include <assert.h>
#include <stdio.h>
#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_tree.hh"
/* #define DEBUG_TEMPLATE_PARSER */
/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize tpltstacksize
#define yysslim tpltsslim
%}

%token STRING
%token COMMAND
%token UPLEVEL
%token DOWNLEVEL
%left COLON
%left ASSIGN_DEFAULT
%token END
%token LITERAL
%token SLASH
%token VARIABLE
%token VARDEF
%token LISTNEXT
%token INITIALIZER
%token RETURN
%token TEXT
%token UINT
%token INT
%token BOOL
%token TOGGLE
%token MACADDR
%token IPV4
%token IPV4PREFIX
%token IPV6
%token IPV6PREFIX
%token INTEGER

%%
input:		/* empty */
		| definition input

definition:	nodename nodegroup

nodename:	literals {push_path();}
		| named_literal { push_path(); }

named_literal:	LITERAL VARDEF {
			extend_path($1, true);
			extend_path($2, false);
		}
		| LITERAL VARDEF COLON type {
			extend_path($1, true);
			extend_path($2, false);
		}

literals:	LITERAL { extend_path($1, false); }
		| literals LITERAL { extend_path($2, false); }
		| literals named_literal

type:		TEXT { tplt_type = NODE_TEXT; }
		| UINT { tplt_type = NODE_UINT; }
		| INT { tplt_type = NODE_INT; }
		| BOOL { tplt_type = NODE_BOOL; }
		| TOGGLE { tplt_type = NODE_TOGGLE; }
		| IPV4 { tplt_type = NODE_IPV4; }
		| IPV4PREFIX { tplt_type = NODE_IPV4PREFIX; }
		| IPV6 { tplt_type = NODE_IPV6; }
		| IPV6PREFIX { tplt_type = NODE_IPV6PREFIX; }
		| MACADDR { tplt_type = NODE_MACADDR; }

init_type:	TEXT ASSIGN_DEFAULT STRING {
			tplt_type = NODE_TEXT;
			tplt_initializer = $3;
		}
		| UINT ASSIGN_DEFAULT INTEGER {
			tplt_type = NODE_UINT;
			tplt_initializer = $3;
		}
		| BOOL ASSIGN_DEFAULT INITIALIZER {
			tplt_type = NODE_BOOL;
			tplt_initializer = $3;
		}
		| TOGGLE ASSIGN_DEFAULT INITIALIZER {
			tplt_type = NODE_TOGGLE;
			tplt_initializer = $3;
		}

nodegroup:	UPLEVEL statements DOWNLEVEL { pop_path(); }

statements:	/* empty string */
		| statement statements

statement:	terminal | command | definition

terminal:	default_terminal | regular_terminal

regular_terminal:	LITERAL COLON type END { terminal($1); }

default_terminal:	LITERAL COLON init_type END { terminal($1); }

command:	cmd_val
		| cmd_default

cmd_val:	command_name COLON cmd_list END

cmd_default:	command_name COLON END

command_name:	COMMAND { add_cmd($1); }

cmd_list:	cmd
		| cmd_list LISTNEXT cmd

cmd:		LITERAL list_of_cmd_strings {
			prepend_cmd($1);
			end_cmd();
		}
		| LITERAL STRING RETURN STRING {
			append_cmd($1);
			append_cmd($2);
			append_cmd(strdup("return"));
			append_cmd($4);
			end_cmd();
		}
		| LITERAL VARIABLE type { /* eg: set FOOBAR ipv4 */
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
		}

list_of_cmd_strings:
		STRING {
			prepend_cmd($1);
		}
		| STRING list_of_cmd_strings {
			prepend_cmd($1);
		}

%%

extern char *lstr;
extern char *vstr;
extern char *cstr;
extern char *sstr;
extern char *istr;
extern FILE *tpltin;
extern int linenum;
static int tplt_type;
static char *tplt_initializer;
static const char *tplt_filename;
static char lastsymbol[256];

#define MAXSTACK 20
#define MAXPATH 256
/* static string cmd_str; */
static string current_cmd;
/* static string path_hold; */
static list<string> cmd_list;
static TemplateTree* tt;
extern "C" int tpltparse();
extern int tpltlex();

void
tplterror(const char *s)
{
    printf("\n ERROR [Template File: %s line %d]: %s\n",
	tplt_filename, linenum, s);
    printf("\n Last symbol parsed was %s\n", lastsymbol);
    exit(1);
}

static void
extend_path(char *segment, bool is_tag)
{
    strncpy(lastsymbol, segment, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    string segname;
    segname = segment;
    tt->extend_path(segname, is_tag);
    /* free(segment); */
    /* printf("\n>>> extend path: %s\n", path); */
}

static void
push_path()
{
    /* printf("\n>>>PUSH: %s\n", path); */
    tt->push_path(tplt_type, tplt_initializer);
    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
}

static void
pop_path()
{
    tt->pop_path();
    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
    /* printf("\n>>>POP: %s\n", path); */
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
    strncpy(lastsymbol, cmd, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    tt->add_cmd(cmd);
    current_cmd = cmd;
    free(cmd);
    cmd_list.clear();
}

static void
append_cmd(char *s)
{
    strncpy(lastsymbol, s, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    cmd_list.push_back(string(s));

#ifdef DEBUG_TEMPLATE_PARSER
    printf("cmd_str now >");
    list<string>::const_iterator iter;
    for (iter = cmd_list.begin(); iter != cmd_list.end(); ++iter) {
	printf("%s ", iter->c_str());
    }
    printf("\n");
#endif /* DEBUG_TEMPLATE_PARSER */

    free(s);
}

static void
prepend_cmd(char *s)
{
    strncpy(lastsymbol, s, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    cmd_list.push_front(string(s));

#ifdef DEBUG_TEMPLATE_PARSER
    printf("cmd_str now >");
    list<string>::const_iterator iter;
    for (iter = cmd_list.begin(); iter != cmd_list.end(); ++iter) {
	printf("%s ", iter->c_str());
    }
    printf("\n");
#endif /* DEBUG_TEMPLATE_PARSER */

    free(s);
}

static void
end_cmd()
{
#ifdef DEBUG_TEMPLATE_PARSER
    printf("end_cmd\n");
#endif

    tt->add_cmd_action(current_cmd, cmd_list);
    cmd_list.clear();
}

int
init_template_parser (const char *filename, TemplateTree *c)
{
    tt = c;
    linenum = 1;

    tpltin = fopen(filename, "r");
    if (tpltin == NULL)
	return -1;

    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
    tplt_filename = filename;
    return 0;
}

int
parse_template()
{
    tpltparse();
    return 0;
}
