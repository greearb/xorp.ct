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
#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_tree.hh"
#include "conf_tree.hh"
/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize bootstacksize
#define yysslim bootsslim
#line 58 "y.boot_tab.c"
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
#define LITERAL 268
#define STRING 269
#define SYNTAX_ERROR 270
const short bootlhs[] = {                                        -1,
    0,    0,    0,    0,    1,    1,    6,    4,    8,    9,
    9,    9,    9,    9,    9,    9,    9,    9,    5,    5,
    7,   10,   10,   11,   11,   11,    2,   12,   12,   12,
   12,   12,   12,   12,   12,   12,    3,
};
const short bootlen[] = {                                         2,
    0,    2,    2,    1,    2,    2,    1,    1,    1,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    1,    1,
    3,    0,    2,    1,    1,    1,    1,    2,    4,    4,
    4,    4,    4,    4,    4,    4,    1,
};
const short bootdefred[] = {                                      0,
   27,    9,   37,    0,    0,    0,    4,    0,    0,    0,
    0,    2,    3,    0,   20,    5,   19,    6,   12,   13,
   14,   15,   16,   17,   18,   11,   10,    0,   25,   26,
    0,    0,   24,   28,    0,   21,   23,    0,    0,    0,
    0,    0,    0,    0,    0,   30,   31,   32,   33,   34,
   35,   36,   29,
};
const short bootdgoto[] = {                                       4,
    5,    6,    7,    8,   16,    9,   17,   10,   11,   31,
   32,   33,
};
const short bootsindex[] = {                                   -254,
    0,    0,    0,    0, -254, -254,    0, -237, -240, -227,
 -247,    0,    0, -244,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -256,    0,    0,
 -235, -244,    0,    0, -236,    0,    0, -216, -212, -211,
 -210, -209, -208, -207, -206,    0,    0,    0,    0,    0,
    0,    0,    0,
};
const short bootrindex[] = {                                     32,
    0,    0,    0,    0,   32,   32,    0,    0,    0, -203,
 -215,    0,    0, -202,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -255,    0,    0,
    0, -202,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
const short bootgindex[] = {                                     40,
  -14,  -13,    0,    0,    0,    0,   46,   47,    0,   25,
    0,    0,
};
#define YYTABLESIZE 58
const short boottable[] = {                                      29,
   30,    9,   34,   35,    1,    9,    9,    9,    9,    9,
    9,    9,    9,    2,    1,    3,   14,   29,   30,   14,
    2,   15,   36,   28,   38,   39,   40,   41,   42,   43,
   44,    1,   45,   19,   20,   21,   22,   23,   24,   25,
   26,    8,   46,    8,   12,   13,   47,   48,   49,   50,
   51,   52,   53,    7,   18,   22,   37,   27,
};
const short bootcheck[] = {                                      14,
   14,  257,  259,  260,  259,  261,  262,  263,  264,  265,
  266,  267,  268,  268,  259,  270,  257,   32,   32,  257,
  268,  259,  258,  268,  261,  262,  263,  264,  265,  266,
  267,    0,  269,  261,  262,  263,  264,  265,  266,  267,
  268,  257,  259,  259,    5,    6,  259,  259,  259,  259,
  259,  259,  259,  257,    9,  258,   32,   11,
};
#define YYFINAL 4
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 270
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
"IPV6_VALUE","IPV6NET_VALUE","MACADDR_VALUE","LITERAL","STRING","SYNTAX_ERROR",
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
"literals : literals literal",
"literals : literal LITERAL",
"literals : literal BOOL_VALUE",
"literals : literal UINT_VALUE",
"literals : literal IPV4_VALUE",
"literals : literal IPV4NET_VALUE",
"literals : literal IPV6_VALUE",
"literals : literal IPV6NET_VALUE",
"literals : literal MACADDR_VALUE",
"nodegroup : long_nodegroup",
"nodegroup : END",
"long_nodegroup : UPLEVEL statements DOWNLEVEL",
"statements :",
"statements : statement statements",
"statement : terminal",
"statement : definition",
"statement : emptystatement",
"emptystatement : END",
"terminal : LITERAL END",
"terminal : LITERAL ASSIGN_OPERATOR STRING END",
"terminal : LITERAL ASSIGN_OPERATOR BOOL_VALUE END",
"terminal : LITERAL ASSIGN_OPERATOR UINT_VALUE END",
"terminal : LITERAL ASSIGN_OPERATOR IPV4_VALUE END",
"terminal : LITERAL ASSIGN_OPERATOR IPV4NET_VALUE END",
"terminal : LITERAL ASSIGN_OPERATOR IPV6_VALUE END",
"terminal : LITERAL ASSIGN_OPERATOR IPV6NET_VALUE END",
"terminal : LITERAL ASSIGN_OPERATOR MACADDR_VALUE END",
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
#line 117 "boot.yy"

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
#line 304 "y.boot_tab.c"
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
#line 42 "boot.yy"
{ push_path(); }
break;
case 8:
#line 45 "boot.yy"
{ push_path(); }
break;
case 9:
#line 48 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 11:
#line 52 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 12:
#line 53 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 13:
#line 54 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 14:
#line 55 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 15:
#line 56 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 16:
#line 57 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 17:
#line 58 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 18:
#line 59 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 20:
#line 63 "boot.yy"
{ pop_path(); }
break;
case 21:
#line 66 "boot.yy"
{ pop_path(); }
break;
case 28:
#line 81 "boot.yy"
{
			terminal(yyvsp[-1], strdup(""), NODE_VOID);
		}
break;
case 29:
#line 84 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_TEXT);
		}
break;
case 30:
#line 87 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_BOOL);
		}
break;
case 31:
#line 90 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_UINT);
		}
break;
case 32:
#line 93 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV4);
		}
break;
case 33:
#line 96 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV4PREFIX);
		}
break;
case 34:
#line 99 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV6);
		}
break;
case 35:
#line 102 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV6PREFIX);
		}
break;
case 36:
#line 105 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_MACADDR);
		}
break;
case 37:
#line 110 "boot.yy"
{
			booterror("syntax error");
		}
break;
#line 611 "y.boot_tab.c"
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
