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
#define yyparse bootparse
#define yylex bootlex
#define yyerror booterror
#define yychar bootchar
#define yyval bootval
#define yylval bootlval
#define yydebug bootdebug
#define yynerrs bootnerrs
#define yyerrflag booterrflag
#define yyss bootss
#define yyssp bootssp
#define yyvs bootvs
#define yyvsp bootvsp
#define yylhs bootlhs
#define yylen bootlen
#define yydefred bootdefred
#define yydgoto bootdgoto
#define yysindex bootsindex
#define yyrindex bootrindex
#define yygindex bootgindex
#define yytable boottable
#define yycheck bootcheck
#define yyname bootname
#define yyrule bootrule
#define yysslim bootsslim
#define yystacksize bootstacksize
#define YYPREFIX "boot"
#line 2 "boot.yy"
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
#line 67 "y.boot_tab.c"
#define YYERRCODE 256
#define UPLEVEL 257
#define DOWNLEVEL 258
#define END 259
#define ASSIGN_OPERATOR 260
#define BOOL_VALUE 261
#define UINT_VALUE 262
#define UINTRANGE_VALUE 263
#define IPV4_VALUE 264
#define IPV4RANGE_VALUE 265
#define IPV4NET_VALUE 266
#define IPV6_VALUE 267
#define IPV6RANGE_VALUE 268
#define IPV6NET_VALUE 269
#define MACADDR_VALUE 270
#define URL_FILE_VALUE 271
#define URL_FTP_VALUE 272
#define URL_HTTP_VALUE 273
#define URL_TFTP_VALUE 274
#define LITERAL 275
#define STRING 276
#define ARITH 277
#define INFIX_OPERATOR 278
#define SYNTAX_ERROR 279
#define CONFIG_NODE_ID 280
const short bootlhs[] = {                                        -1,
    0,    0,    0,    0,    1,    1,    6,    4,    8,    8,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    5,    5,    7,
   10,   10,   11,   11,   11,    2,   13,   13,   12,   12,
   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
   12,   12,   12,   12,   12,    3,
};
const short bootlen[] = {                                         2,
    0,    2,    2,    1,    2,    2,    1,    1,    1,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    2,    2,    2,    1,    1,    3,
    0,    2,    1,    1,    1,    1,    1,    2,    2,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    1,
};
const short bootdefred[] = {                                      0,
   36,    9,   56,    0,    0,    0,    0,    4,    0,    0,
    0,    0,   10,    2,    3,    0,   29,    5,   28,    6,
   14,   16,   15,   18,   17,   19,   21,   20,   22,   23,
   24,   25,   26,   27,   13,   12,   11,    0,    0,   34,
   35,    0,    0,   33,    0,    0,   30,   32,   39,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,   41,   43,   42,   45,
   44,   46,   48,   47,   49,   50,   51,   52,   53,   54,
   40,   55,
};
const short bootdgoto[] = {                                       5,
    6,    7,    8,    9,   18,   10,   19,   11,   12,   42,
   43,   44,   45,
};
const short bootsindex[] = {                                   -210,
    0,    0,    0, -270,    0, -210, -210,    0, -233, -225,
 -171, -250,    0,    0,    0, -208,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0, -222,    0,
    0, -201, -208,    0, -256,    0,    0,    0,    0, -188,
 -200, -199, -198, -197, -196, -195, -193, -191, -172, -153,
 -152, -151, -150, -149, -148, -147,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
const short bootrindex[] = {                                     58,
    0,    0,    0,    0,    0,   58,   58,    0,    0,    0,
 -186, -205,    0,    0,    0, -145,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -255,    0,    0,
    0,    0, -145,    0,    0, -228,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
const short bootgindex[] = {                                     49,
  -16,  -15,    0,    0,    0,    0,  104,  103,    0,   73,
    0,    0,    0,
};
#define YYTABLESIZE 116
const short boottable[] = {                                      40,
   41,    9,   49,   37,   13,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,   50,   37,   16,    2,   17,   40,   41,   10,    4,
   38,   16,   10,   10,   10,   10,   10,   10,   10,   10,
   10,   10,   10,   10,   10,   10,   10,   10,    1,   38,
    1,    8,   46,    8,   14,   15,   47,    1,   67,   68,
   69,   70,   71,   72,    2,   73,   38,   74,    3,    4,
    7,   39,   51,   52,   53,   54,   55,   56,   57,   58,
   59,   60,   61,   62,   63,   64,   75,   65,   66,   21,
   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
   32,   33,   34,   35,   36,   76,   77,   78,   79,   80,
   81,   82,   31,   20,   37,   48,
};
const short bootcheck[] = {                                      16,
   16,  257,  259,  259,  275,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  274,  275,
  276,  278,  278,  257,  275,  259,   43,   43,  257,  280,
  259,  257,  261,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  275,  276,  259,  278,
  259,  257,  275,  259,    6,    7,  258,    0,  259,  259,
  259,  259,  259,  259,  275,  259,  275,  259,  279,  280,
  257,  280,  261,  262,  263,  264,  265,  266,  267,  268,
  269,  270,  271,  272,  273,  274,  259,  276,  277,  261,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  274,  275,  276,  259,  259,  259,  259,  259,
  259,  259,  258,   10,   12,   43,
};
#define YYFINAL 5
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 280
#if YYDEBUG
const char * const bootname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"UPLEVEL","DOWNLEVEL","END",
"ASSIGN_OPERATOR","BOOL_VALUE","UINT_VALUE","UINTRANGE_VALUE","IPV4_VALUE",
"IPV4RANGE_VALUE","IPV4NET_VALUE","IPV6_VALUE","IPV6RANGE_VALUE",
"IPV6NET_VALUE","MACADDR_VALUE","URL_FILE_VALUE","URL_FTP_VALUE",
"URL_HTTP_VALUE","URL_TFTP_VALUE","LITERAL","STRING","ARITH","INFIX_OPERATOR",
"SYNTAX_ERROR","CONFIG_NODE_ID",
};
const char * const bootrule[] = {
"$accept : input",
"input :",
"input : definition input",
"input : emptystatement input",
"input : syntax_error",
"definition : long_nodename nodegroup",
"definition : short_nodename long_nodegroup",
"short_nodename : literal",
"long_nodename : literals",
"literal : LITERAL",
"literal : CONFIG_NODE_ID LITERAL",
"literals : literals literal",
"literals : literal STRING",
"literals : literal LITERAL",
"literals : literal BOOL_VALUE",
"literals : literal UINTRANGE_VALUE",
"literals : literal UINT_VALUE",
"literals : literal IPV4RANGE_VALUE",
"literals : literal IPV4_VALUE",
"literals : literal IPV4NET_VALUE",
"literals : literal IPV6RANGE_VALUE",
"literals : literal IPV6_VALUE",
"literals : literal IPV6NET_VALUE",
"literals : literal MACADDR_VALUE",
"literals : literal URL_FILE_VALUE",
"literals : literal URL_FTP_VALUE",
"literals : literal URL_HTTP_VALUE",
"literals : literal URL_TFTP_VALUE",
"nodegroup : long_nodegroup",
"nodegroup : END",
"long_nodegroup : UPLEVEL statements DOWNLEVEL",
"statements :",
"statements : statement statements",
"statement : terminal",
"statement : definition",
"statement : emptystatement",
"emptystatement : END",
"term_literal : LITERAL",
"term_literal : CONFIG_NODE_ID LITERAL",
"terminal : term_literal END",
"terminal : term_literal INFIX_OPERATOR STRING END",
"terminal : term_literal INFIX_OPERATOR BOOL_VALUE END",
"terminal : term_literal INFIX_OPERATOR UINTRANGE_VALUE END",
"terminal : term_literal INFIX_OPERATOR UINT_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV4RANGE_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV4_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV4NET_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV6RANGE_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV6_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV6NET_VALUE END",
"terminal : term_literal INFIX_OPERATOR MACADDR_VALUE END",
"terminal : term_literal INFIX_OPERATOR URL_FILE_VALUE END",
"terminal : term_literal INFIX_OPERATOR URL_FTP_VALUE END",
"terminal : term_literal INFIX_OPERATOR URL_HTTP_VALUE END",
"terminal : term_literal INFIX_OPERATOR URL_TFTP_VALUE END",
"terminal : term_literal INFIX_OPERATOR ARITH END",
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
#line 192 "boot.yy"

extern void boot_scan_string(const char *configuration);
extern int boot_linenum;
extern "C" int bootparse();
extern int bootlex();

void booterror(const char *s) throw (ParseError);

static ConfigTree *config_tree = NULL;
static string boot_filename;
static string lastsymbol;
static string node_id;


static void
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
#line 417 "y.boot_tab.c"
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
case 7:
#line 60 "boot.yy"
{ push_path(); }
break;
case 8:
#line 63 "boot.yy"
{ push_path(); }
break;
case 9:
#line 66 "boot.yy"
{ node_id = ""; extend_path(yyvsp[0], NODE_VOID, node_id); }
break;
case 10:
#line 67 "boot.yy"
{ node_id = yyvsp[-1];
					   free(yyvsp[-1]);
					   extend_path(yyvsp[0], NODE_VOID, node_id); }
break;
case 12:
#line 73 "boot.yy"
{ extend_path(yyvsp[0], NODE_TEXT, node_id); }
break;
case 13:
#line 74 "boot.yy"
{ extend_path(yyvsp[0], NODE_TEXT, node_id); }
break;
case 14:
#line 75 "boot.yy"
{ extend_path(yyvsp[0], NODE_BOOL, node_id); }
break;
case 15:
#line 76 "boot.yy"
{ extend_path(yyvsp[0], NODE_UINTRANGE, node_id); }
break;
case 16:
#line 77 "boot.yy"
{ extend_path(yyvsp[0], NODE_UINT, node_id); }
break;
case 17:
#line 78 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4RANGE, node_id); }
break;
case 18:
#line 79 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4, node_id); }
break;
case 19:
#line 80 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4NET, node_id); }
break;
case 20:
#line 81 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6RANGE, node_id); }
break;
case 21:
#line 82 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6, node_id); }
break;
case 22:
#line 83 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6NET, node_id); }
break;
case 23:
#line 84 "boot.yy"
{ extend_path(yyvsp[0], NODE_MACADDR, node_id); }
break;
case 24:
#line 85 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_FILE, node_id); }
break;
case 25:
#line 86 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_FTP, node_id); }
break;
case 26:
#line 87 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_HTTP, node_id); }
break;
case 27:
#line 88 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_TFTP, node_id); }
break;
case 29:
#line 92 "boot.yy"
{ pop_path(); }
break;
case 30:
#line 95 "boot.yy"
{ pop_path(); }
break;
case 37:
#line 110 "boot.yy"
{ node_id = ""; extend_path(yyvsp[0], NODE_VOID, node_id); }
break;
case 38:
#line 111 "boot.yy"
{ node_id = yyvsp[-1];
					   free(yyvsp[-1]);
					   extend_path(yyvsp[0], NODE_VOID, node_id);}
break;
case 39:
#line 116 "boot.yy"
{
			terminal(strdup(""), NODE_VOID, OP_NONE);
		}
break;
case 40:
#line 119 "boot.yy"
{
			terminal(yyvsp[-1], NODE_TEXT, boot_lookup_operator(yyvsp[-2])); 
			free(yyvsp[-2]);
		}
break;
case 41:
#line 123 "boot.yy"
{
			terminal(yyvsp[-1], NODE_BOOL, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 42:
#line 127 "boot.yy"
{
			terminal(yyvsp[-1], NODE_UINTRANGE, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 43:
#line 131 "boot.yy"
{
			terminal(yyvsp[-1], NODE_UINT, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 44:
#line 135 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV4RANGE, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 45:
#line 139 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV4, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 46:
#line 143 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV4NET, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 47:
#line 147 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV6RANGE, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 48:
#line 151 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV6, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 49:
#line 155 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV6NET, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 50:
#line 159 "boot.yy"
{
			terminal(yyvsp[-1], NODE_MACADDR, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 51:
#line 163 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_FILE, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 52:
#line 167 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_FTP, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 53:
#line 171 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_HTTP, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 54:
#line 175 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_TFTP, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 55:
#line 179 "boot.yy"
{
			terminal(yyvsp[-1], NODE_ARITH, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 56:
#line 185 "boot.yy"
{
			booterror("syntax error");
		}
break;
#line 836 "y.boot_tab.c"
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
