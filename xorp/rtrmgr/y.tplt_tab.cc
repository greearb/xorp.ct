#ifndef lint

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
#define yyparse tpltparse
#define yylex tpltlex
#define yyerror tplterror
#define yychar tpltchar
#define yyval tpltval
#define yylval tpltlval
#define yydebug tpltdebug
#define yynerrs tpltnerrs
#define yyerrflag tplterrflag
#define yyss tpltss
#define yyssp tpltssp
#define yyvs tpltvs
#define yyvsp tpltvsp
#define yylhs tpltlhs
#define yylen tpltlen
#define yydefred tpltdefred
#define yydgoto tpltdgoto
#define yysindex tpltsindex
#define yyrindex tpltrindex
#define yygindex tpltgindex
#define yytable tplttable
#define yycheck tpltcheck
#define yyname tpltname
#define yyrule tpltrule
#define yysslim tpltsslim
#define yystacksize tpltstacksize
#define YYPREFIX "tplt"
#line 2 "template.yy"
#define YYSTYPE char*

#include <assert.h>
#include <stdio.h>

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "template_tree_node.hh"
#include "template_tree.hh"
extern void add_cmd_adaptor(char *cmd, TemplateTree* tt) throw (ParseError);
extern void add_cmd_action_adaptor(const string& cmd,
				   const list<string>& action,
				   TemplateTree* tt) throw (ParseError);

/* XXX: sigh, the -p flag to yacc should do this for us */
#define yystacksize tpltstacksize
#define yysslim tpltsslim
#line 67 "y.tplt_tab.c"
#define YYERRCODE 256
#define UPLEVEL 257
#define DOWNLEVEL 258
#define END 259
#define COLON 260
#define ASSIGN_DEFAULT 261
#define LISTNEXT 262
#define RETURN 263
#define TEXT_TYPE 264
#define INT_TYPE 265
#define UINT_TYPE 266
#define UINTRANGE_TYPE 267
#define BOOL_TYPE 268
#define TOGGLE_TYPE 269
#define IPV4_TYPE 270
#define IPV4RANGE_TYPE 271
#define IPV4NET_TYPE 272
#define IPV6_TYPE 273
#define IPV6RANGE_TYPE 274
#define IPV6NET_TYPE 275
#define MACADDR_TYPE 276
#define URL_FILE_TYPE 277
#define URL_FTP_TYPE 278
#define URL_HTTP_TYPE 279
#define URL_TFTP_TYPE 280
#define BOOL_VALUE 281
#define INTEGER_VALUE 282
#define UINTRANGE_VALUE 283
#define IPV4_VALUE 284
#define IPV4RANGE_VALUE 285
#define IPV4NET_VALUE 286
#define IPV6_VALUE 287
#define IPV6RANGE_VALUE 288
#define IPV6NET_VALUE 289
#define MACADDR_VALUE 290
#define URL_FILE_VALUE 291
#define URL_FTP_VALUE 292
#define URL_HTTP_VALUE 293
#define URL_TFTP_VALUE 294
#define VARDEF 295
#define COMMAND 296
#define VARIABLE 297
#define LITERAL 298
#define STRING 299
#define SYNTAX_ERROR 300
const short tpltlhs[] = {                                        -1,
    0,    0,    0,    1,    3,    3,    6,    6,    5,    5,
    5,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    7,    7,    7,    8,    8,
    8,    8,    8,    8,    8,    8,    8,    8,    8,    8,
    8,    8,    8,    8,    8,    4,    9,    9,   10,   10,
   10,   11,   11,   14,   13,   12,   12,   15,   16,   17,
   18,   18,   19,   19,   19,   19,   19,   19,   19,   19,
   19,   20,   20,   20,   20,    2,
};
const short tpltlen[] = {                                         2,
    0,    2,    1,    2,    1,    1,    2,    4,    1,    2,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    0,    2,    1,    1,
    1,    1,    1,    4,    4,    1,    1,    4,    3,    1,
    1,    3,    1,    2,    4,    3,    2,    3,    1,    2,
    1,    1,    2,    2,    3,    1,
};
const short tpltdefred[] = {                                      0,
    0,   76,    0,    0,    3,    0,    0,    6,    0,    2,
    0,    4,    0,   11,    0,   60,    0,   51,    0,    0,
   49,   50,   52,   53,   56,   57,    0,   12,   13,   14,
   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
   25,   26,   27,   28,    8,    0,   46,   48,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   59,    0,
    0,    0,   72,    0,   61,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   54,   55,   73,    0,    0,    0,    0,
    0,   58,    0,    0,   74,   29,   30,   31,   32,   33,
   34,   35,   36,   37,   38,   39,   40,   41,   42,   43,
   44,   45,   66,   68,    0,   62,   75,   65,
};
const short tpltdgoto[] = {                                       3,
    4,    5,    6,   12,    7,    8,   45,   68,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   74,   75,   76,
};
const short tpltsindex[] = {                                   -249,
 -289,    0,    0, -249,    0, -240, -257,    0, -169,    0,
 -210,    0, -289,    0, -244,    0, -241,    0, -173, -210,
    0,    0,    0,    0,    0,    0, -168,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -209,    0,    0, -251, -167,
 -166, -165, -164, -163, -162, -161, -160, -159, -158, -157,
 -156, -155, -154, -153, -152, -151, -148, -147,    0, -146,
 -292, -287,    0, -187,    0, -281, -206, -150, -149, -170,
 -145, -144, -143, -142, -171, -141, -172, -140, -139, -174,
 -138, -137, -176,    0,    0,    0, -281, -244, -178, -136,
 -281,    0, -259, -135,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -177,    0,    0,    0,
};
const short tpltrindex[] = {                                    119,
 -256,    0,    0,  119,    0,    0, -134,    0, -255,    0,
 -173,    0, -254,    0,    0,    0, -256,    0,    0, -173,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -133,
 -131, -130, -129, -128, -125, -124, -121, -120, -119, -117,
 -115, -114, -112, -111, -109, -107,    0,    0,    0,    0,
 -186, -185,    0,    0,    0, -181,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -180,    0, -179, -246,
 -175,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};
const short tpltgindex[] = {                                    116,
   -6,    0,    0,    0,    0,  117,  -46,    0,  133,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   52,   18,
};
#define YYTABLESIZE 156
const short tplttable[] = {                                      67,
    9,    7,   10,   70,   18,    9,   73,   69,   70,   98,
   99,  100,   72,   18,  104,   72,   11,  105,   46,   28,
   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
   39,   40,   41,   42,   43,   44,   70,   71,   72,   73,
   13,    9,    7,   10,   70,   71,   72,   73,    1,   72,
    2,  123,   72,    9,   50,   51,   52,   53,   54,   55,
   56,   57,   58,   59,   60,   61,   62,   63,   64,   65,
   66,  102,   69,   71,  103,   69,   71,   63,   70,   67,
   63,   70,   67,   64,   47,   16,   64,   17,   97,  101,
   15,   49,  106,   77,   78,   79,   80,   81,   82,   83,
   84,   85,   86,   87,   88,   89,   90,   91,   92,   93,
   94,   95,  109,   96,  114,  116,  119,  122,    1,   10,
  124,  128,    5,   14,  127,   12,  125,   13,   14,   15,
   16,  107,  108,   17,   18,  110,  111,   19,   20,   21,
  112,   22,  113,   23,   24,  115,   25,   26,  117,   27,
  118,   28,   48,  120,  126,  121,
};
const short tpltcheck[] = {                                      46,
  257,  257,  257,  296,   11,  295,  299,  259,  296,  297,
  298,  299,  259,   20,  296,  262,  257,  299,  260,  264,
  265,  266,  267,  268,  269,  270,  271,  272,  273,  274,
  275,  276,  277,  278,  279,  280,  296,  297,  298,  299,
  298,  298,  298,  298,  296,  297,  298,  299,  298,  296,
  300,   98,  299,  295,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  278,  279,
  280,  259,  259,  259,  262,  262,  262,  259,  259,  259,
  262,  262,  262,  259,  258,  296,  262,  298,   71,   72,
  260,  260,  299,  261,  261,  261,  261,  261,  261,  261,
  261,  261,  261,  261,  261,  261,  261,  261,  261,  261,
  259,  259,  283,  260,  286,  288,  291,  294,    0,    4,
  299,  299,  257,    7,  260,  259,  263,  259,  259,  259,
  259,  282,  282,  259,  259,  281,  281,  259,  259,  259,
  284,  259,  285,  259,  259,  287,  259,  259,  289,  259,
  290,  259,   20,  292,  103,  293,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 300
#if YYDEBUG
const char * const tpltname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"UPLEVEL","DOWNLEVEL","END",
"COLON","ASSIGN_DEFAULT","LISTNEXT","RETURN","TEXT_TYPE","INT_TYPE","UINT_TYPE",
"UINTRANGE_TYPE","BOOL_TYPE","TOGGLE_TYPE","IPV4_TYPE","IPV4RANGE_TYPE",
"IPV4NET_TYPE","IPV6_TYPE","IPV6RANGE_TYPE","IPV6NET_TYPE","MACADDR_TYPE",
"URL_FILE_TYPE","URL_FTP_TYPE","URL_HTTP_TYPE","URL_TFTP_TYPE","BOOL_VALUE",
"INTEGER_VALUE","UINTRANGE_VALUE","IPV4_VALUE","IPV4RANGE_VALUE",
"IPV4NET_VALUE","IPV6_VALUE","IPV6RANGE_VALUE","IPV6NET_VALUE","MACADDR_VALUE",
"URL_FILE_VALUE","URL_FTP_VALUE","URL_HTTP_VALUE","URL_TFTP_VALUE","VARDEF",
"COMMAND","VARIABLE","LITERAL","STRING","SYNTAX_ERROR",
};
const char * const tpltrule[] = {
"$accept : input",
"input :",
"input : definition input",
"input : syntax_error",
"definition : nodename nodegroup",
"nodename : literals",
"nodename : named_literal",
"named_literal : LITERAL VARDEF",
"named_literal : LITERAL VARDEF COLON type",
"literals : LITERAL",
"literals : literals LITERAL",
"literals : literals named_literal",
"type : TEXT_TYPE",
"type : INT_TYPE",
"type : UINT_TYPE",
"type : UINTRANGE_TYPE",
"type : BOOL_TYPE",
"type : TOGGLE_TYPE",
"type : IPV4_TYPE",
"type : IPV4RANGE_TYPE",
"type : IPV4NET_TYPE",
"type : IPV6_TYPE",
"type : IPV6RANGE_TYPE",
"type : IPV6NET_TYPE",
"type : MACADDR_TYPE",
"type : URL_FILE_TYPE",
"type : URL_FTP_TYPE",
"type : URL_HTTP_TYPE",
"type : URL_TFTP_TYPE",
"init_type : TEXT_TYPE ASSIGN_DEFAULT STRING",
"init_type : INT_TYPE ASSIGN_DEFAULT INTEGER_VALUE",
"init_type : UINT_TYPE ASSIGN_DEFAULT INTEGER_VALUE",
"init_type : UINTRANGE_TYPE ASSIGN_DEFAULT UINTRANGE_VALUE",
"init_type : BOOL_TYPE ASSIGN_DEFAULT BOOL_VALUE",
"init_type : TOGGLE_TYPE ASSIGN_DEFAULT BOOL_VALUE",
"init_type : IPV4_TYPE ASSIGN_DEFAULT IPV4_VALUE",
"init_type : IPV4RANGE_TYPE ASSIGN_DEFAULT IPV4RANGE_VALUE",
"init_type : IPV4NET_TYPE ASSIGN_DEFAULT IPV4NET_VALUE",
"init_type : IPV6_TYPE ASSIGN_DEFAULT IPV6_VALUE",
"init_type : IPV6RANGE_TYPE ASSIGN_DEFAULT IPV6RANGE_VALUE",
"init_type : IPV6NET_TYPE ASSIGN_DEFAULT IPV6NET_VALUE",
"init_type : MACADDR_TYPE ASSIGN_DEFAULT MACADDR_VALUE",
"init_type : URL_FILE_TYPE ASSIGN_DEFAULT URL_FILE_VALUE",
"init_type : URL_FTP_TYPE ASSIGN_DEFAULT URL_FTP_VALUE",
"init_type : URL_HTTP_TYPE ASSIGN_DEFAULT URL_HTTP_VALUE",
"init_type : URL_TFTP_TYPE ASSIGN_DEFAULT URL_TFTP_VALUE",
"nodegroup : UPLEVEL statements DOWNLEVEL",
"statements :",
"statements : statement statements",
"statement : terminal",
"statement : command",
"statement : definition",
"terminal : default_terminal",
"terminal : regular_terminal",
"regular_terminal : LITERAL COLON type END",
"default_terminal : LITERAL COLON init_type END",
"command : cmd_val",
"command : cmd_default",
"cmd_val : command_name COLON cmd_list END",
"cmd_default : command_name COLON END",
"command_name : COMMAND",
"cmd_list : cmd",
"cmd_list : cmd_list LISTNEXT cmd",
"cmd : list_of_cmd_strings",
"cmd : LITERAL list_of_cmd_strings",
"cmd : LITERAL STRING RETURN STRING",
"cmd : LITERAL VARIABLE type",
"cmd : LITERAL LITERAL",
"cmd : LITERAL LITERAL STRING",
"cmd : VARIABLE",
"cmd : VARIABLE list_of_cmd_strings",
"cmd : LITERAL",
"list_of_cmd_strings : STRING",
"list_of_cmd_strings : COMMAND COLON",
"list_of_cmd_strings : list_of_cmd_strings STRING",
"list_of_cmd_strings : list_of_cmd_strings COMMAND COLON",
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
#line 293 "template.yy"

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

    add_cmd_adaptor(cmd, tt);
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
    add_cmd_action_adaptor(current_cmd, cmd_list, tt);
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
complete_template_parser()
{
    if (tpltin != NULL)
        fclose(tpltin);
}

void
parse_template() throw (ParseError)
{
    if (tpltparse() != 0)
	tplterror("unknown error");
}
#line 495 "y.tplt_tab.c"
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
case 5:
#line 79 "template.yy"
{ push_path(); }
break;
case 6:
#line 80 "template.yy"
{ push_path(); }
break;
case 7:
#line 83 "template.yy"
{
			extend_path(yyvsp[-1], true);
			extend_path(yyvsp[0], false);
		}
break;
case 8:
#line 87 "template.yy"
{
			extend_path(yyvsp[-3], true);
			extend_path(yyvsp[-2], false);
		}
break;
case 9:
#line 93 "template.yy"
{ extend_path(yyvsp[0], false); }
break;
case 10:
#line 94 "template.yy"
{ extend_path(yyvsp[0], false); }
break;
case 12:
#line 98 "template.yy"
{ tplt_type = NODE_TEXT; }
break;
case 13:
#line 99 "template.yy"
{ tplt_type = NODE_INT; }
break;
case 14:
#line 100 "template.yy"
{ tplt_type = NODE_UINT; }
break;
case 15:
#line 101 "template.yy"
{ tplt_type = NODE_UINTRANGE; }
break;
case 16:
#line 102 "template.yy"
{ tplt_type = NODE_BOOL; }
break;
case 17:
#line 103 "template.yy"
{ tplt_type = NODE_TOGGLE; }
break;
case 18:
#line 104 "template.yy"
{ tplt_type = NODE_IPV4; }
break;
case 19:
#line 105 "template.yy"
{ tplt_type = NODE_IPV4RANGE; }
break;
case 20:
#line 106 "template.yy"
{ tplt_type = NODE_IPV4NET; }
break;
case 21:
#line 107 "template.yy"
{ tplt_type = NODE_IPV6; }
break;
case 22:
#line 108 "template.yy"
{ tplt_type = NODE_IPV6RANGE; }
break;
case 23:
#line 109 "template.yy"
{ tplt_type = NODE_IPV6NET; }
break;
case 24:
#line 110 "template.yy"
{ tplt_type = NODE_MACADDR; }
break;
case 25:
#line 111 "template.yy"
{ tplt_type = NODE_URL_FILE; }
break;
case 26:
#line 112 "template.yy"
{ tplt_type = NODE_URL_FTP; }
break;
case 27:
#line 113 "template.yy"
{ tplt_type = NODE_URL_HTTP; }
break;
case 28:
#line 114 "template.yy"
{ tplt_type = NODE_URL_TFTP; }
break;
case 29:
#line 117 "template.yy"
{
			tplt_type = NODE_TEXT;
			tplt_initializer = yyvsp[0];
		}
break;
case 30:
#line 121 "template.yy"
{
			tplt_type = NODE_INT;
			tplt_initializer = yyvsp[0];
		}
break;
case 31:
#line 125 "template.yy"
{
			tplt_type = NODE_UINT;
			tplt_initializer = yyvsp[0];
		}
break;
case 32:
#line 129 "template.yy"
{
			tplt_type = NODE_UINTRANGE;
			tplt_initializer = yyvsp[0];
		}
break;
case 33:
#line 133 "template.yy"
{
			tplt_type = NODE_BOOL;
			tplt_initializer = yyvsp[0];
		}
break;
case 34:
#line 137 "template.yy"
{
			tplt_type = NODE_TOGGLE;
			tplt_initializer = yyvsp[0];
		}
break;
case 35:
#line 141 "template.yy"
{
			tplt_type = NODE_IPV4;
			tplt_initializer = yyvsp[0];
		}
break;
case 36:
#line 145 "template.yy"
{
			tplt_type = NODE_IPV4RANGE;
			tplt_initializer = yyvsp[0];
		}
break;
case 37:
#line 149 "template.yy"
{
			tplt_type = NODE_IPV4NET;
			tplt_initializer = yyvsp[0];
		}
break;
case 38:
#line 153 "template.yy"
{
			tplt_type = NODE_IPV6;
			tplt_initializer = yyvsp[0];
		}
break;
case 39:
#line 157 "template.yy"
{
			tplt_type = NODE_IPV6RANGE;
			tplt_initializer = yyvsp[0];
		}
break;
case 40:
#line 161 "template.yy"
{
			tplt_type = NODE_IPV6NET;
			tplt_initializer = yyvsp[0];
		}
break;
case 41:
#line 165 "template.yy"
{
			tplt_type = NODE_MACADDR;
			tplt_initializer = yyvsp[0];
		}
break;
case 42:
#line 169 "template.yy"
{
			tplt_type = NODE_URL_FILE;
			tplt_initializer = yyvsp[0];
		}
break;
case 43:
#line 173 "template.yy"
{
			tplt_type = NODE_URL_FTP;
			tplt_initializer = yyvsp[0];
		}
break;
case 44:
#line 177 "template.yy"
{
			tplt_type = NODE_URL_HTTP;
			tplt_initializer = yyvsp[0];
		}
break;
case 45:
#line 181 "template.yy"
{
			tplt_type = NODE_URL_TFTP;
			tplt_initializer = yyvsp[0];
		}
break;
case 46:
#line 187 "template.yy"
{ pop_path(); }
break;
case 54:
#line 203 "template.yy"
{ terminal(yyvsp[-3]); }
break;
case 55:
#line 206 "template.yy"
{ terminal(yyvsp[-3]); }
break;
case 59:
#line 216 "template.yy"
{ end_cmd(); }
break;
case 60:
#line 219 "template.yy"
{ add_cmd(yyvsp[0]); }
break;
case 63:
#line 226 "template.yy"
{
			end_cmd();
		}
break;
case 64:
#line 229 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 65:
#line 233 "template.yy"
{
			append_cmd(yyvsp[-3]);
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 66:
#line 240 "template.yy"
{ /* e.g.: set FOOBAR ipv4 */
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 67:
#line 246 "template.yy"
{
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 68:
#line 251 "template.yy"
{
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 69:
#line 257 "template.yy"
{
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 70:
#line 261 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 71:
#line 265 "template.yy"
{
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 72:
#line 272 "template.yy"
{
			append_cmd(yyvsp[0]);
		}
break;
case 73:
#line 275 "template.yy"
{
			append_cmd(yyvsp[-1]);
		}
break;
case 74:
#line 278 "template.yy"
{
			append_cmd(yyvsp[0]);
		}
break;
case 75:
#line 281 "template.yy"
{
			append_cmd(yyvsp[-1]);
		}
break;
case 76:
#line 286 "template.yy"
{
			tplterror("syntax error");
		}
break;
#line 1027 "y.tplt_tab.c"
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
