#ifndef lint
#ident "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.28.2.1 2001/07/19 05:46:39 peter Exp $"
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

#include "conf_tree.hh"
#include "conf_tree_node.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"
#include "config_operators.hh"

/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize bootstacksize
#define yysslim bootsslim
#line 66 "y.boot_tab.c"
#define YYERRCODE 256
#define UPLEVEL 257
#define DOWNLEVEL 258
#define END 259
#define ASSIGN_OPERATOR 260
#define BOOL_VALUE 261
#define UINT_VALUE 262
#define IPV4_VALUE 263
#define IPV4NET_VALUE 264
#define IPV6_VALUE 265
#define IPV6NET_VALUE 266
#define MACADDR_VALUE 267
#define URL_FILE_VALUE 268
#define URL_FTP_VALUE 269
#define URL_HTTP_VALUE 270
#define URL_TFTP_VALUE 271
#define LITERAL 272
#define STRING 273
#define ARITH 274
#define INFIX_OPERATOR 275
#define SYNTAX_ERROR 276
#define LINENUM 277
const short bootlhs[] = {                                        -1,
    0,    0,    0,    0,    1,    1,    6,    4,    8,    8,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    5,    5,    7,   10,   10,   11,
   11,   11,    2,   13,   13,   12,   12,   12,   12,   12,
   12,   12,   12,   12,   12,   12,   12,   12,   12,    3,
};
const short bootlen[] = {                                         2,
    0,    2,    2,    1,    2,    2,    1,    1,    1,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    2,    1,    1,    3,    0,    2,    1,
    1,    1,    1,    1,    2,    2,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    1,
};
const short bootdefred[] = {                                      0,
   33,    9,   50,    0,    0,    0,    0,    4,    0,    0,
    0,    0,   10,    2,    3,    0,   26,    5,   25,    6,
   14,   15,   16,   17,   18,   19,   20,   21,   22,   23,
   24,   13,   12,   11,    0,    0,   31,   32,    0,    0,
   30,    0,    0,   27,   29,   36,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   38,   39,   40,   41,   42,   43,   44,   45,   46,   47,
   48,   37,   49,
};
const short bootdgoto[] = {                                       5,
    6,    7,    8,    9,   18,   10,   19,   11,   12,   39,
   40,   41,   42,
};
const short bootsindex[] = {                                   -216,
    0,    0,    0, -267,    0, -216, -216,    0, -236, -228,
 -183, -250,    0,    0,    0, -214,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -225,    0,    0, -207, -214,
    0, -256,    0,    0,    0,    0, -197, -206, -205, -204,
 -202, -200, -184, -168, -167, -166, -165, -164, -163, -162,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
const short bootrindex[] = {                                     52,
    0,    0,    0,    0,    0,   52,   52,    0,    0,    0,
 -195, -211,    0,    0,    0, -160,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -255,    0,    0,    0,    0, -160,
    0,    0, -231,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
const short bootgindex[] = {                                     43,
  -16,  -15,    0,    0,    0,    0,   89,   88,    0,   61,
    0,    0,    0,
};
#define YYTABLESIZE 101
const short boottable[] = {                                      37,
   38,    9,   46,   34,   13,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,    9,   47,   34,
   16,    2,   17,   37,   38,   10,    4,   35,   16,   10,
   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,
   10,   10,    1,   35,    1,    8,   43,    8,   14,   15,
   44,    1,   61,   62,   63,    2,   64,   35,   65,    3,
    4,    7,   36,   48,   49,   50,   51,   52,   53,   54,
   55,   56,   57,   58,   66,   59,   60,   21,   22,   23,
   24,   25,   26,   27,   28,   29,   30,   31,   32,   33,
   67,   68,   69,   70,   71,   72,   73,   28,   20,   34,
   45,
};
const short bootcheck[] = {                                      16,
   16,  257,  259,  259,  272,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,  273,  275,  275,
  257,  272,  259,   40,   40,  257,  277,  259,  257,  261,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  273,  259,  275,  259,  257,  272,  259,    6,    7,
  258,    0,  259,  259,  259,  272,  259,  272,  259,  276,
  277,  257,  277,  261,  262,  263,  264,  265,  266,  267,
  268,  269,  270,  271,  259,  273,  274,  261,  262,  263,
  264,  265,  266,  267,  268,  269,  270,  271,  272,  273,
  259,  259,  259,  259,  259,  259,  259,  258,   10,   12,
   40,
};
#define YYFINAL 5
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 277
#if YYDEBUG
const char * const bootname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"UPLEVEL","DOWNLEVEL","END",
"ASSIGN_OPERATOR","BOOL_VALUE","UINT_VALUE","IPV4_VALUE","IPV4NET_VALUE",
"IPV6_VALUE","IPV6NET_VALUE","MACADDR_VALUE","URL_FILE_VALUE","URL_FTP_VALUE",
"URL_HTTP_VALUE","URL_TFTP_VALUE","LITERAL","STRING","ARITH","INFIX_OPERATOR",
"SYNTAX_ERROR","LINENUM",
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
"literal : LINENUM LITERAL",
"literals : literals literal",
"literals : literal STRING",
"literals : literal LITERAL",
"literals : literal BOOL_VALUE",
"literals : literal UINT_VALUE",
"literals : literal IPV4_VALUE",
"literals : literal IPV4NET_VALUE",
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
"term_literal : LINENUM LITERAL",
"terminal : term_literal END",
"terminal : term_literal INFIX_OPERATOR STRING END",
"terminal : term_literal INFIX_OPERATOR BOOL_VALUE END",
"terminal : term_literal INFIX_OPERATOR UINT_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV4_VALUE END",
"terminal : term_literal INFIX_OPERATOR IPV4NET_VALUE END",
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
#line 175 "boot.yy"

extern void boot_scan_string(const char *configuration);
extern int boot_linenum;
extern "C" int bootparse();
extern int bootlex();

static ConfigTree *config_tree = NULL;
static string boot_filename;
static string lastsymbol;
static uint64_t nodenum;


static void
extend_path(char* segment, int type, uint64_t node_num)
{
    lastsymbol = segment;
    config_tree->extend_path(string(segment), type, node_num);
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
#line 387 "y.boot_tab.c"
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
#line 56 "boot.yy"
{ push_path(); }
break;
case 8:
#line 59 "boot.yy"
{ push_path(); }
break;
case 9:
#line 62 "boot.yy"
{ nodenum = 0;
                          extend_path(yyvsp[0], NODE_VOID, nodenum); }
break;
case 10:
#line 64 "boot.yy"
{ nodenum = strtoll(yyvsp[-1], (char **)NULL, 10);
		                    free(yyvsp[-1]);
				    extend_path(yyvsp[0], NODE_VOID, nodenum); }
break;
case 12:
#line 70 "boot.yy"
{ extend_path(yyvsp[0], NODE_TEXT, nodenum); }
break;
case 13:
#line 71 "boot.yy"
{ extend_path(yyvsp[0], NODE_TEXT, nodenum); }
break;
case 14:
#line 72 "boot.yy"
{ extend_path(yyvsp[0], NODE_BOOL, nodenum); }
break;
case 15:
#line 73 "boot.yy"
{ extend_path(yyvsp[0], NODE_UINT, nodenum); }
break;
case 16:
#line 74 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4, nodenum); }
break;
case 17:
#line 75 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4NET, nodenum); }
break;
case 18:
#line 76 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6, nodenum); }
break;
case 19:
#line 77 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6NET, nodenum); }
break;
case 20:
#line 78 "boot.yy"
{ extend_path(yyvsp[0], NODE_MACADDR, nodenum); }
break;
case 21:
#line 79 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_FILE, nodenum); }
break;
case 22:
#line 80 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_FTP, nodenum); }
break;
case 23:
#line 81 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_HTTP, nodenum); }
break;
case 24:
#line 82 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_TFTP, nodenum); }
break;
case 26:
#line 86 "boot.yy"
{ pop_path(); }
break;
case 27:
#line 89 "boot.yy"
{ pop_path(); }
break;
case 34:
#line 104 "boot.yy"
{ nodenum = 0;
			  extend_path(yyvsp[0], NODE_VOID, nodenum); }
break;
case 35:
#line 106 "boot.yy"
{ nodenum = strtoll(yyvsp[-1], (char **)NULL, 10);
		                    free(yyvsp[-1]);
				    extend_path(yyvsp[0], NODE_VOID, nodenum);}
break;
case 36:
#line 111 "boot.yy"
{
			terminal(strdup(""), NODE_VOID, OP_NONE);
		}
break;
case 37:
#line 114 "boot.yy"
{
			terminal(yyvsp[-1], NODE_TEXT, boot_lookup_operator(yyvsp[-2])); 
			free(yyvsp[-2]);
		}
break;
case 38:
#line 118 "boot.yy"
{
			terminal(yyvsp[-1], NODE_BOOL, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 39:
#line 122 "boot.yy"
{
			terminal(yyvsp[-1], NODE_UINT, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 40:
#line 126 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV4, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 41:
#line 130 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV4NET, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 42:
#line 134 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV6, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 43:
#line 138 "boot.yy"
{
			terminal(yyvsp[-1], NODE_IPV6NET, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 44:
#line 142 "boot.yy"
{
			terminal(yyvsp[-1], NODE_MACADDR, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 45:
#line 146 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_FILE, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 46:
#line 150 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_FTP, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 47:
#line 154 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_HTTP, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 48:
#line 158 "boot.yy"
{
			terminal(yyvsp[-1], NODE_URL_TFTP, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 49:
#line 162 "boot.yy"
{
			terminal(yyvsp[-1], NODE_ARITH, boot_lookup_operator(yyvsp[-2]));
			free(yyvsp[-2]);
		}
break;
case 50:
#line 168 "boot.yy"
{
			booterror("syntax error");
		}
break;
#line 775 "y.boot_tab.c"
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
