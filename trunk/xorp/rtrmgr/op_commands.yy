%{
#define YYSTYPE char*
#include <assert.h>
#include <stdio.h>

#include <string>
#include <map>

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
%token CMD_MODULE
%token CMD_COMMAND
%token CMD_HELP
%token CMD_OPT_PARAMETER
%token CMD_TAG
%token VARIABLE
%token LITERAL
%token STRING
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

word:			LITERAL { append_path_word($1); }
			;

variable:		VARIABLE { append_path_variable($1); }
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

attribute:		cmd_module
			| cmd_command
			| cmd_opt_parameter
			| cmd_tag
			;

cmd_module:		CMD_MODULE COLON LITERAL END {
				add_cmd_module($3);
			}
			;

cmd_command:		CMD_COMMAND COLON STRING cmd_help END {
				add_cmd_command($3);
			}
			;

cmd_opt_parameter:	CMD_OPT_PARAMETER COLON STRING cmd_help END {
				add_cmd_opt_parameter($3);
			}
			;

cmd_tag:		CMD_TAG COLON LITERAL STRING END {
				add_cmd_tag($3, $4);
			}
			;

cmd_help:		CMD_HELP COLON LITERAL {
				add_cmd_help_tag($3);
			}
			| CMD_HELP COLON STRING {
				add_cmd_help_string($3);
			}
			;

syntax_error:		SYNTAX_ERROR { opcmderror("syntax error"); }
			;


%%

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

    command = strip_quotes_and_empty_space(command);
    if (command.empty()) {
	opcmderror("Invalid emtpy command");
    }

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
    if (! ocl->find_executable_filename(filename, executable_filename)) {
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
