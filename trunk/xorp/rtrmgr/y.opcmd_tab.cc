#ifndef lint
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28.2.1 2001/07/19 05:46:39 peter Exp $";
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING() (yyerrflag!=0)
#if defined(__cplusplus) || __STDC__
static int yygrowstack(void);
#else
static int yygrowstack();
#endif
#define yyparse opcmdparse
#define yylex opcmdlex
#define yyerror opcmderror
#define yychar opcmdchar
#define yyval opcmdval
#define yylval opcmdlval
#define yydebug opcmddebug
#define yynerrs opcmdnerrs
#define yyerrflag opcmderrflag
#define yyss opcmdss
#define yyssp opcmdssp
#define yyvs opcmdvs
#define yyvsp opcmdvsp
#define yylhs opcmdlhs
#define yylen opcmdlen
#define yydefred opcmddefred
#define yydgoto opcmddgoto
#define yysindex opcmdsindex
#define yyrindex opcmdrindex
#define yygindex opcmdgindex
#define yytable opcmdtable
#define yycheck opcmdcheck
#define yyname opcmdname
#define yyrule opcmdrule
#define yysslim opcmdsslim
#define yystacksize opcmdstacksize
#define YYPREFIX "opcmd"
#line 2 "op_commands.yy"
#define YYSTYPE char*

#include <assert.h>
#include <stdio.h>

#include <map>

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"

#include "op_commands.hh"

/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize opcmdstacksize
#define yysslim opcmdsslim
#line 63 "y.opcmd_tab.c"
#define YYERRCODE 256
#define UPLEVEL 257
#define DOWNLEVEL 258
#define END 259
#define COLON 260
#define CMD_MODULE 261
#define CMD_COMMAND 262
#define CMD_HELP 263
#define CMD_OPT_PARAMETER 264
#define CMD_TAG 265
#define VARIABLE 266
#define WILDCARD 267
#define LITERAL 268
#define STRING 269
#define SYNTAX_ERROR 270
const short opcmdlhs[] = {                                        -1,
    0,    0,    0,    1,    4,    4,    6,    6,    6,    3,
    7,    8,    5,    9,   11,   10,   10,   12,   12,   12,
   12,   13,   14,   15,   16,   17,   17,    2,
};
const short opcmdlen[] = {                                         2,
    0,    2,    1,    3,    0,    2,    1,    1,    1,    1,
    1,    1,    3,    1,    1,    1,    2,    1,    1,    1,
    1,    4,    5,    5,    5,    3,    3,    1,
};
const short opcmddefred[] = {                                      0,
   10,   28,    0,    0,    3,    0,    2,   11,   12,    7,
    0,    0,    8,    9,   14,    4,    0,    6,    0,    0,
    0,    0,    0,    0,   18,   19,   20,   21,    0,    0,
    0,    0,   15,   13,   17,    0,    0,    0,    0,   22,
    0,    0,    0,    0,    0,   23,   24,   25,   26,   27,
};
const short opcmddgoto[] = {                                       3,
    4,    5,    6,   11,   16,   12,   13,   14,   17,   23,
   34,   24,   25,   26,   27,   28,   42,
};
const short opcmdsindex[] = {                                   -258,
    0,    0,    0, -258,    0, -259,    0,    0,    0,    0,
 -254, -259,    0,    0,    0,    0, -260,    0, -249, -245,
 -244, -243, -240, -260,    0,    0,    0,    0, -248, -250,
 -247, -242,    0,    0,    0, -238, -239, -239, -246,    0,
 -235, -232, -231, -230, -255,    0,    0,    0,    0,    0,
};
const short opcmdrindex[] = {                                     30,
    0,    0,    0,   30,    0, -226,    0,    0,    0,    0,
    0, -226,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -225,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
const short opcmdgindex[] = {                                     28,
    0,    0,   -6,   22,    0,    0,    0,    0,    0,   11,
    0,    0,    0,    0,    0,    0,   -2,
};
#define YYTABLESIZE 36
const short opcmdtable[] = {                                      10,
   19,   20,   15,   21,   22,   10,    8,    9,    1,    1,
   29,    2,   49,   50,   30,   31,   32,   33,   37,   36,
   40,   38,   44,   41,   45,   39,   46,   47,   48,    1,
    5,    7,   16,   18,   35,   43,
};
const short opcmdcheck[] = {                                       6,
  261,  262,  257,  264,  265,   12,  266,  267,  268,  268,
  260,  270,  268,  269,  260,  260,  260,  258,  269,  268,
  259,  269,  269,  263,  260,  268,  259,  259,  259,    0,
  257,    4,  258,   12,   24,   38,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 270
#if YYDEBUG
const char * const opcmdname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"UPLEVEL","DOWNLEVEL","END",
"COLON","CMD_MODULE","CMD_COMMAND","CMD_HELP","CMD_OPT_PARAMETER","CMD_TAG",
"VARIABLE","WILDCARD","LITERAL","STRING","SYNTAX_ERROR",
};
const char * const opcmdrule[] = {
"$accept : input",
"input :",
"input : definition input",
"input : syntax_error",
"definition : word word_or_variable_list specification",
"word_or_variable_list :",
"word_or_variable_list : word_or_variable word_or_variable_list",
"word_or_variable : word",
"word_or_variable : variable",
"word_or_variable : wildcard",
"word : LITERAL",
"variable : VARIABLE",
"wildcard : WILDCARD",
"specification : startspec attributes endspec",
"startspec : UPLEVEL",
"endspec : DOWNLEVEL",
"attributes : attribute",
"attributes : attribute attributes",
"attribute : cmd_module",
"attribute : cmd_command",
"attribute : cmd_opt_parameter",
"attribute : cmd_tag",
"cmd_module : CMD_MODULE COLON LITERAL END",
"cmd_command : CMD_COMMAND COLON STRING cmd_help END",
"cmd_opt_parameter : CMD_OPT_PARAMETER COLON STRING cmd_help END",
"cmd_tag : CMD_TAG COLON LITERAL STRING END",
"cmd_help : CMD_HELP COLON LITERAL",
"cmd_help : CMD_HELP COLON STRING",
"syntax_error : SYNTAX_ERROR",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
#endif
#if YYDEBUG
#include <stdio.h>
#endif
#ifdef YYSTACKSIZE
#undef YYMAXDEPTH
#define YYMAXDEPTH YYSTACKSIZE
#else
#ifdef YYMAXDEPTH
#define YYSTACKSIZE YYMAXDEPTH
#else
#define YYSTACKSIZE 10000
#define YYMAXDEPTH 10000
#endif
#endif
#define YYINITSTACKSIZE 200
int yydebug;
int yynerrs;
int yyerrflag;
int yychar;
short *yyssp;
YYSTYPE *yyvsp;
YYSTYPE yyval;
YYSTYPE yylval;
short *yyss;
short *yysslim;
YYSTYPE *yyvs;
int yystacksize;
#line 115 "op_commands.yy"

extern "C" int opcmdparse();
extern int opcmdlex();
extern FILE *opcmdin;
extern int opcmd_linenum;

void opcmderror(const char *s) throw (ParseError);

static OpCommandList *ocl = NULL;
static list<string> path_segments;
static list<list<string> >path_segments_stack;
static list<OpCommand> op_command_stack;
static list<string> op_command_tag_help_stack;
static list<map<string, string> > opt_params_tag_help_stack;
static list<map<string, string> > tags_stack;
static bool is_help_tag = false;
static string help_tag;
static string help_string;

static string opcmd_filename;
static string lastsymbol;


// Strip the quotes, the heading and trailing empty space
static string
strip_quotes_and_empty_space(const string& s)
{
    string res;

    // Strip the quotes
    if ((s.length() < 2) || (s[0] != '"') || (s[s.length() - 1] != '"')) {
	string errmsg = c_format("Internal parser error: %s "
				 "is not a quoted string",
				 s.c_str());
	opcmderror(errmsg.c_str());
    }
    res = s.substr(1, s.length() - 2);

    // Strip the heading and trailing empty space
    while (!res.empty()) {
	size_t len = res.length();
	if ((res[0] == ' ') || (res[0] == '\t')) {
	    res = res.substr(1, len - 1);
	    continue;
	}
	if ((res[len - 1] == ' ') || (res[len - 1] == '\t')) {
	    res = res.substr(0, res.length() - 1);
	    continue;
	}
	break;
    }

    return res;
}

static void
resolve_tag(const string& tag, string& value)
{
    map<string, string>& tags = tags_stack.back();
    map<string, string>::iterator iter;

    iter = tags.find(tag);
    if (iter == tags.end()) {
	string errmsg = c_format("Cannot resolve tag %s", tag.c_str());
	opcmderror(errmsg.c_str());
    }

    value = iter->second;
}

static void
append_path_word(char *s)
{
    string word = s;
    lastsymbol = s;
    free(s);

    path_segments.push_back(word);
}

static void
append_path_variable(char *s)
{
    string variable = s;
    lastsymbol = s;
    free(s);

    // Check if a valid variable name
    if (ocl->check_variable_name(variable) == false) {
	string errmsg = c_format("Bad variable name: %s", variable.c_str());
	opcmderror(errmsg.c_str());
    }
    path_segments.push_back(variable);
}

static void
push_path()
{
    if (! path_segments_stack.empty()) {
	// Extend the nested path
	list<string>& tmp_prefix = path_segments_stack.back();
	path_segments.insert(path_segments.begin(),
			     tmp_prefix.begin(), tmp_prefix.end());
    }
    path_segments_stack.push_back(path_segments);

    OpCommand *existing_command = ocl->find_op_command(path_segments);
    if (existing_command != NULL) {
	string errmsg = c_format("Duplicate command: %s",
				 existing_command->command_name().c_str());
	opcmderror(errmsg.c_str());
    }

    // Push all temporary state storage into the stack
    map<string, string> dummy_map;
    OpCommand op_command(path_segments);
    op_command_stack.push_back(op_command);
    op_command_tag_help_stack.push_back("");
    opt_params_tag_help_stack.push_back(dummy_map);
    tags_stack.push_back(dummy_map);
}

static void
pop_path()
{
    string help;

    if (op_command_stack.empty())
	opcmderror("Invalid end of block");

    OpCommand& op_command = op_command_stack.back();

    // Resolve the command help (if tagged)
    string& op_command_tag_help = op_command_tag_help_stack.back();
    if (! op_command_tag_help.empty()) {
	resolve_tag(op_command_tag_help, help);
	op_command.set_help_string(help);
    }

    // Resolve the optional parameters help (if tagged)
    map<string, string>& opt_params_tag_help = opt_params_tag_help_stack.back();
    map<string, string>::iterator iter;
    for (iter = opt_params_tag_help.begin();
	 iter != opt_params_tag_help.end();
	 ++iter) {
	 string opt_param = iter->first;
	 string tag = iter->second;
	 resolve_tag(tag, help);
	 op_command.add_opt_param(opt_param, help);
    }

    // Add the command
    if (ocl->add_op_command(op_command) == NULL) {
	string errmsg = c_format("Cannot add command %s: internal error",
				 op_command.command_name().c_str());
	opcmderror(errmsg.c_str());
    }

    // Clear the path segments
    path_segments.clear();

    // Pop from the stack all temporary state
    path_segments_stack.pop_back();
    op_command_stack.pop_back();
    op_command_tag_help_stack.pop_back();
    opt_params_tag_help_stack.pop_back();
    tags_stack.pop_back();
}

static void
add_cmd_module(char *s)
{
    string module = s;
    lastsymbol = s;
    free(s);

    OpCommand& op_command = op_command_stack.back();
    if (! op_command.module().empty()) {
	string errmsg = c_format("Module name already set to %s "
				 "(only one module allowed per command)",
				 op_command.module().c_str());
	opcmderror(errmsg.c_str());
    }
    op_command.set_module(module);
}

static void
add_cmd_command(char *s)
{
    string command = s;
    lastsymbol = s;
    free(s);

    //
    // XXX: note that we allow the command itself to be empty
    // (e.g., if the entry is for an intermediate node in the hierarchy).
    //
    command = strip_quotes_and_empty_space(command);

    // Split the command filename from the arguments
    string filename, arguments;
    for (size_t i = 0; i < command.length(); i++) {
	if ((command[i] == ' ') || (command[i] == '\t')) {
	    filename = command.substr(0, i);
	    arguments = command.substr(i);
	    break;
	}
    }
    if (filename.empty())
	filename = command;

    // Find the executable filename
    string executable_filename;
    if ( (! command.empty())
	&& (! ocl->find_executable_filename(filename, executable_filename))) {
	string errmsg = c_format("Executable file not found: %s",
				 filename.c_str());
	opcmderror(errmsg.c_str());
    }

    // Get a reference to the OpCommand instance
    OpCommand& op_command = op_command_stack.back();
    if (! op_command.command_action().empty()) {
	string errmsg = c_format("Command action already set to %s "
				 "(only one action allowed per command)",
				 op_command.command_action().c_str());
	opcmderror(errmsg.c_str());
    }

    //
    // Split the arguments and verify that the positional arguments
    // (e.g., $0, $1, etc) are resolvable.
    //
    list<string> command_action_arguments;
    string current_argument;
    for (size_t i = 0; i < arguments.length(); i++) {
	if ((arguments[i] == ' ') || (arguments[i] == '\t')) {
	    if (! current_argument.empty()) {
		command_action_arguments.push_back(current_argument);
		current_argument.erase();
	    }
	    continue;
	}
	current_argument += arguments[i];
    }
    // Add the last argument
    if (! current_argument.empty()) {
	command_action_arguments.push_back(current_argument);
	current_argument.erase();
    }
    // Verify the positional arguments
    list<string>::const_iterator iter;
    for (iter = command_action_arguments.begin();
	 iter != command_action_arguments.end();
	 ++iter) {
	const string& arg = *iter;
	if (arg.empty()) {
	    string errmsg = c_format("Internal error spliting the positional "
				     "arguments");
	    opcmderror(errmsg.c_str());
	}
	if (arg[0] == '$') {
	    string errmsg;
	    string resolved_str = op_command.select_positional_argument(
		op_command.command_parts(), arg, errmsg);
	    if (resolved_str.empty()) {
		opcmderror(errmsg.c_str());
	    }
	}
    }

    op_command.set_command_action(command);
    op_command.set_command_action_filename(filename);
    op_command.set_command_action_arguments(command_action_arguments);
    op_command.set_command_executable_filename(executable_filename);

    if (is_help_tag) {
	string& op_command_tag_help = op_command_tag_help_stack.back();
	op_command_tag_help = help_tag;
	return;
    }

    op_command.set_help_string(help_string);
}

static void
add_cmd_opt_parameter(char *s)
{
    string opt_parameter = s;
    lastsymbol = s;
    free(s);

    opt_parameter = strip_quotes_and_empty_space(opt_parameter);
    if (opt_parameter.empty()) {
	opcmderror("Invalid emtpy optional parameter");
    }

    OpCommand& op_command = op_command_stack.back();
    map<string, string>& opt_params_tag_help = opt_params_tag_help_stack.back();
    if (op_command.has_opt_param(opt_parameter)
	|| (opt_params_tag_help.find(opt_parameter)
	    != opt_params_tag_help.end())) {
	string errmsg = c_format("Duplicated optional parameter %s",
				  opt_parameter.c_str());
	opcmderror(errmsg.c_str());
    }

    if (is_help_tag) {
	opt_params_tag_help.insert(make_pair(opt_parameter, help_tag));
	return;
    }

    op_command.add_opt_param(opt_parameter, help_string);
}

static void
add_cmd_tag(char *t, char *v)
{
    string tag = t;
    string value = v;
    lastsymbol = v;
    free(t);
    free(v);

    map<string, string>& tags = tags_stack.back();
    if (tags.find(tag) != tags.end()) {
	string errmsg = c_format("Duplicated tag %s", tag.c_str());
	opcmderror(errmsg.c_str());
    }

    // Strip the quotes
    value = strip_quotes_and_empty_space(value);

    tags.insert(make_pair(tag, value));
}

static void
add_cmd_help_tag(char *s)
{
    string tag = s;
    lastsymbol = s;
    free(s);

    help_tag = tag;
    is_help_tag = true;
}

static void
add_cmd_help_string(char *s)
{
    string help = s;
    lastsymbol = s;
    free(s);

    // Strip the quotes
    help = strip_quotes_and_empty_space(help);

    help_string = help;
    is_help_tag = false;
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
#line 604 "y.opcmd_tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
static int yygrowstack()
{
    int newsize, i;
    short *newss;
    YYSTYPE *newvs;

    if ((newsize = yystacksize) == 0)
        newsize = YYINITSTACKSIZE;
    else if (newsize >= YYMAXDEPTH)
        return -1;
    else if ((newsize *= 2) > YYMAXDEPTH)
        newsize = YYMAXDEPTH;
    i = yyssp - yyss;
    newss = yyss ? (short *)realloc(yyss, newsize * sizeof *newss) :
      (short *)malloc(newsize * sizeof *newss);
    if (newss == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    newvs = yyvs ? (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs) :
      (YYSTYPE *)malloc(newsize * sizeof *newvs);
    if (newvs == NULL)
        return -1;
    yyvs = newvs;
    yyvsp = newvs + i;
    yystacksize = newsize;
    yysslim = yyss + newsize - 1;
    return 0;
}

#define YYABORT goto yyabort
#define YYREJECT goto yyabort
#define YYACCEPT goto yyaccept
#define YYERROR goto yyerrlab

#ifndef YYPARSE_PARAM
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG void
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG
#define YYPARSE_PARAM_DECL
#endif	/* ANSI-C/C++ */
#else	/* YYPARSE_PARAM */
#ifndef YYPARSE_PARAM_TYPE
#define YYPARSE_PARAM_TYPE void *
#endif
#if defined(__cplusplus) || __STDC__
#define YYPARSE_PARAM_ARG YYPARSE_PARAM_TYPE YYPARSE_PARAM
#define YYPARSE_PARAM_DECL
#else	/* ! ANSI-C/C++ */
#define YYPARSE_PARAM_ARG YYPARSE_PARAM
#define YYPARSE_PARAM_DECL YYPARSE_PARAM_TYPE YYPARSE_PARAM;
#endif	/* ANSI-C/C++ */
#endif	/* ! YYPARSE_PARAM */

int
yyparse (YYPARSE_PARAM_ARG)
    YYPARSE_PARAM_DECL
{
    register int yym, yyn, yystate;
#if YYDEBUG
    register const char *yys;

    if ((yys = getenv("YYDEBUG")))
    {
        yyn = *yys;
        if (yyn >= '0' && yyn <= '9')
            yydebug = yyn - '0';
    }
#endif

    yynerrs = 0;
    yyerrflag = 0;
    yychar = (-1);

    if (yyss == NULL && yygrowstack()) goto yyoverflow;
    yyssp = yyss;
    yyvsp = yyvs;
    *yyssp = yystate = 0;

yyloop:
    if ((yyn = yydefred[yystate])) goto yyreduce;
    if (yychar < 0)
    {
        if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, reading %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
    }
    if ((yyn = yysindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: state %d, shifting to state %d\n",
                    YYPREFIX, yystate, yytable[yyn]);
#endif
        if (yyssp >= yysslim && yygrowstack())
        {
            goto yyoverflow;
        }
        *++yyssp = yystate = yytable[yyn];
        *++yyvsp = yylval;
        yychar = (-1);
        if (yyerrflag > 0)  --yyerrflag;
        goto yyloop;
    }
    if ((yyn = yyrindex[yystate]) && (yyn += yychar) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yychar)
    {
        yyn = yytable[yyn];
        goto yyreduce;
    }
    if (yyerrflag) goto yyinrecovery;
#if defined(lint) || defined(__GNUC__)
    goto yynewerror;
#endif
yynewerror:
    yyerror("syntax error");
#if defined(lint) || defined(__GNUC__)
    goto yyerrlab;
#endif
yyerrlab:
    ++yynerrs;
yyinrecovery:
    if (yyerrflag < 3)
    {
        yyerrflag = 3;
        for (;;)
        {
            if ((yyn = yysindex[*yyssp]) && (yyn += YYERRCODE) >= 0 &&
                    yyn <= YYTABLESIZE && yycheck[yyn] == YYERRCODE)
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: state %d, error recovery shifting\
 to state %d\n", YYPREFIX, *yyssp, yytable[yyn]);
#endif
                if (yyssp >= yysslim && yygrowstack())
                {
                    goto yyoverflow;
                }
                *++yyssp = yystate = yytable[yyn];
                *++yyvsp = yylval;
                goto yyloop;
            }
            else
            {
#if YYDEBUG
                if (yydebug)
                    printf("%sdebug: error recovery discarding state %d\n",
                            YYPREFIX, *yyssp);
#endif
                if (yyssp <= yyss) goto yyabort;
                --yyssp;
                --yyvsp;
            }
        }
    }
    else
    {
        if (yychar == 0) goto yyabort;
#if YYDEBUG
        if (yydebug)
        {
            yys = 0;
            if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
            if (!yys) yys = "illegal-symbol";
            printf("%sdebug: state %d, error recovery discards token %d (%s)\n",
                    YYPREFIX, yystate, yychar, yys);
        }
#endif
        yychar = (-1);
        goto yyloop;
    }
yyreduce:
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: state %d, reducing by rule %d (%s)\n",
                YYPREFIX, yystate, yyn, yyrule[yyn]);
#endif
    yym = yylen[yyn];
    yyval = yyvsp[1-yym];
    switch (yyn)
    {
case 4:
#line 42 "op_commands.yy"
{ }
break;
case 6:
#line 46 "op_commands.yy"
{ }
break;
case 10:
#line 54 "op_commands.yy"
{ append_path_word(yyvsp[0]); }
break;
case 11:
#line 57 "op_commands.yy"
{ append_path_variable(yyvsp[0]); }
break;
case 12:
#line 60 "op_commands.yy"
{ append_path_word(yyvsp[0]); }
break;
case 13:
#line 63 "op_commands.yy"
{ }
break;
case 14:
#line 66 "op_commands.yy"
{ push_path(); }
break;
case 15:
#line 69 "op_commands.yy"
{ pop_path(); }
break;
case 22:
#line 82 "op_commands.yy"
{
				add_cmd_module(yyvsp[-1]);
			}
break;
case 23:
#line 87 "op_commands.yy"
{
				add_cmd_command(yyvsp[-2]);
			}
break;
case 24:
#line 92 "op_commands.yy"
{
				add_cmd_opt_parameter(yyvsp[-2]);
			}
break;
case 25:
#line 97 "op_commands.yy"
{
				add_cmd_tag(yyvsp[-2], yyvsp[-1]);
			}
break;
case 26:
#line 102 "op_commands.yy"
{
				add_cmd_help_tag(yyvsp[0]);
			}
break;
case 27:
#line 105 "op_commands.yy"
{
				add_cmd_help_string(yyvsp[0]);
			}
break;
case 28:
#line 110 "op_commands.yy"
{ opcmderror("syntax error"); }
break;
#line 871 "y.opcmd_tab.c"
    }
    yyssp -= yym;
    yystate = *yyssp;
    yyvsp -= yym;
    yym = yylhs[yyn];
    if (yystate == 0 && yym == 0)
    {
#if YYDEBUG
        if (yydebug)
            printf("%sdebug: after reduction, shifting from state 0 to\
 state %d\n", YYPREFIX, YYFINAL);
#endif
        yystate = YYFINAL;
        *++yyssp = YYFINAL;
        *++yyvsp = yyval;
        if (yychar < 0)
        {
            if ((yychar = yylex()) < 0) yychar = 0;
#if YYDEBUG
            if (yydebug)
            {
                yys = 0;
                if (yychar <= YYMAXTOKEN) yys = yyname[yychar];
                if (!yys) yys = "illegal-symbol";
                printf("%sdebug: state %d, reading %d (%s)\n",
                        YYPREFIX, YYFINAL, yychar, yys);
            }
#endif
        }
        if (yychar == 0) goto yyaccept;
        goto yyloop;
    }
    if ((yyn = yygindex[yym]) && (yyn += yystate) >= 0 &&
            yyn <= YYTABLESIZE && yycheck[yyn] == yystate)
        yystate = yytable[yyn];
    else
        yystate = yydgoto[yym];
#if YYDEBUG
    if (yydebug)
        printf("%sdebug: after reduction, shifting from state %d \
to state %d\n", YYPREFIX, *yyssp, yystate);
#endif
    if (yyssp >= yysslim && yygrowstack())
    {
        goto yyoverflow;
    }
    *++yyssp = yystate;
    *++yyvsp = yyval;
    goto yyloop;
yyoverflow:
    yyerror("yacc stack overflow");
yyabort:
    return (1);
yyaccept:
    return (0);
}
