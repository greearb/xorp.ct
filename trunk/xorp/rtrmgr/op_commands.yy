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

%token STRING
%token COMMAND
%token UPLEVEL
%token DOWNLEVEL
%left COLON
%token END
%token LITERAL
%token VARIABLE

%%
input:		/* empty */
		| definition input

definition:	word specification {
		}
		| word word word_or_var_list specification {
		}

word:		LITERAL {
			append_path($1);
		}

word_or_var_list:
		/* empty */
		| word word_or_var_list
		| VARIABLE word_or_var_list {
			append_path($1);
		}

specification:	startspec attributes endspec {
		}

startspec:	UPLEVEL {
			push_path();
		}

endspec:	DOWNLEVEL {
			pop_path();
		}

attributes:	attribute
		|  attribute attributes

attribute:	COMMAND COLON LITERAL END {
			add_cmd($1);
			append_cmd($3);
			end_cmd();
		}

%%

extern FILE *opcmdin;
extern int op_linenum;
static const char *opcmd_filename;
static char lastsymbol[256];

static string current_cmd;
static list<string> cmd_list;
static OpCommandList *ocl;
extern "C" int opcmdparse();
extern int opcmdlex();

void
opcmderror (const char *s)
{
    printf("\n ERROR [Operational Command File: %s line %d]: %s\n",
	   opcmd_filename, op_linenum, s);
    printf("\n Last symbol parsed was %s\n", lastsymbol);
    exit(1);
}

static void
append_path(char *segment)
{
    strncpy(lastsymbol, segment, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

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
    strncpy(lastsymbol, cmd, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    ocl->add_cmd(cmd);
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

#ifdef DEBUG_OPCMD_PARSER
    printf("cmd_str now >");
    list<string>::const_iterator iter;
    for (iter = cmd_list.begin(); iter != cmd_list.end(); ++iter) {
	printf("%s ", iter->c_str());
    }
    printf("\n");
#endif

    free(s);
}

static void
end_cmd()
{
#ifdef DEBUG_OPCMD_PARSER
    printf("end_cmd\n");
#endif

    ocl->add_cmd_action(current_cmd, cmd_list);
    cmd_list.clear();
}

int
init_opcmd_parser(const char *filename, OpCommandList *o)
{
    ocl = o;
    op_linenum = 1;
    opcmdin = fopen(filename, "r");
    if (opcmdin == NULL)
	return -1;
    opcmd_filename = filename;
    return 0;
}

int
parse_opcmd()
{
    opcmdparse();
    return 0;
}
