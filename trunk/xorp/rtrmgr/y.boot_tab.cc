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
    0,    0,    0,    0,    1,    4,    6,    6,    6,    6,
    6,    6,    6,    6,    6,    5,    5,    7,    7,    8,
    8,    8,    2,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    3,
};
const short bootlen[] = {                                         2,
    0,    2,    2,    1,    2,    1,    1,    2,    2,    2,
    2,    2,    2,    2,    2,    3,    1,    0,    2,    1,
    1,    1,    1,    2,    4,    4,    4,    4,    4,    4,
    4,    4,    1,
};
const short bootdefred[] = {                                      0,
   23,    7,   33,    0,    0,    0,    4,    0,    0,    2,
    3,    0,   17,    5,    9,   10,   11,   12,   13,   14,
   15,    8,    0,   21,   22,    0,    0,   20,   24,    0,
   16,   19,    0,    0,    0,    0,    0,    0,    0,    0,
   26,   27,   28,   29,   30,   31,   32,   25,
};
const short bootdgoto[] = {                                       4,
    5,    6,    7,    8,   14,    9,   26,   27,   28,
};
const short bootsindex[] = {                                   -245,
    0,    0,    0,    0, -245, -245,    0, -254, -225,    0,
    0, -242,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -238,    0,    0, -224, -242,    0,    0, -234,
    0,    0, -240, -235, -213, -212, -211, -210, -209, -208,
    0,    0,    0,    0,    0,    0,    0,    0,
};
const short bootrindex[] = {                                      4,
    0,    0,    0,    0,    4,    4,    0,    0, -239,    0,
    0, -206,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -255,    0,    0,    0, -206,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};
const short bootgindex[] = {                                     39,
  -12,  -11,    0,    0,    0,    0,   26,    0,    0,
};
#define YYTABLESIZE 53
const short boottable[] = {                                      24,
   25,    7,   12,    1,   13,    7,    7,    7,    7,    7,
    7,    7,    7,    1,   24,   25,    1,    6,   41,    6,
   29,   30,    2,   42,    3,   23,   33,   34,   35,   36,
   37,   38,   39,   31,   40,   15,   16,   17,   18,   19,
   20,   21,   22,   10,   11,   43,   44,   45,   46,   47,
   48,   18,   32,
};
const short bootcheck[] = {                                      12,
   12,  257,  257,    0,  259,  261,  262,  263,  264,  265,
  266,  267,  268,  259,   27,   27,  259,  257,  259,  259,
  259,  260,  268,  259,  270,  268,  261,  262,  263,  264,
  265,  266,  267,  258,  269,  261,  262,  263,  264,  265,
  266,  267,  268,    5,    6,  259,  259,  259,  259,  259,
  259,  258,   27,
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
"definition : nodename nodegroup",
"nodename : literals",
"literals : LITERAL",
"literals : literals LITERAL",
"literals : literals BOOL_VALUE",
"literals : literals UINT_VALUE",
"literals : literals IPV4_VALUE",
"literals : literals IPV4NET_VALUE",
"literals : literals IPV6_VALUE",
"literals : literals IPV6NET_VALUE",
"literals : literals MACADDR_VALUE",
"nodegroup : UPLEVEL statements DOWNLEVEL",
"nodegroup : END",
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
#line 107 "boot.yy"

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
#line 295 "y.boot_tab.c"
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
case 6:
#line 41 "boot.yy"
{ push_path(); }
break;
case 7:
#line 44 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 8:
#line 45 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 9:
#line 46 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 10:
#line 47 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 11:
#line 48 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 12:
#line 49 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 13:
#line 50 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 14:
#line 51 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 15:
#line 52 "boot.yy"
{ extend_path(yyvsp[0]); }
break;
case 16:
#line 55 "boot.yy"
{ pop_path(); }
break;
case 17:
#line 56 "boot.yy"
{ pop_path(); }
break;
case 24:
#line 71 "boot.yy"
{
			terminal(yyvsp[-1], strdup(""), NODE_VOID);
		}
break;
case 25:
#line 74 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_TEXT);
		}
break;
case 26:
#line 77 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_BOOL);
		}
break;
case 27:
#line 80 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_UINT);
		}
break;
case 28:
#line 83 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV4);
		}
break;
case 29:
#line 86 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV4PREFIX);
		}
break;
case 30:
#line 89 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV6);
		}
break;
case 31:
#line 92 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV6PREFIX);
		}
break;
case 32:
#line 95 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_MACADDR);
		}
break;
case 33:
#line 100 "boot.yy"
{
			booterror("syntax error");
		}
break;
#line 598 "y.boot_tab.c"
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
