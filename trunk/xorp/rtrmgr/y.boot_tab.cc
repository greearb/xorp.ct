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
#define STRING 257
#define UPLEVEL 258
#define DOWNLEVEL 259
#define ASSIGN_VALUE 260
#define ASSIGN_DEFAULT 261
#define END 262
#define LITERAL 263
#define SLASH 264
#define LISTNEXT 265
#define TEXT 266
#define UINT 267
#define INTEGER 268
#define BOOL 269
#define IPV4 270
#define IPV4NET 271
#define IPV6 272
#define IPV6NET 273
const short bootlhs[] = {                                        -1,
    0,    0,    0,    1,    3,    5,    5,    5,    5,    5,
    5,    5,    4,    4,    6,    6,    7,    7,    7,    2,
    8,    8,    8,    8,    8,    8,    8,    8,
};
const short bootlen[] = {                                         2,
    0,    2,    2,    2,    1,    1,    2,    2,    2,    2,
    2,    2,    3,    1,    0,    2,    1,    1,    1,    1,
    2,    4,    4,    4,    4,    4,    4,    4,
};
const short bootdefred[] = {                                      0,
   20,    6,    0,    0,    0,    0,    0,    2,    3,    0,
   14,    4,    7,   12,    8,    9,   10,   11,    0,   18,
   19,    0,    0,   17,    0,   21,   13,   16,    0,    0,
    0,    0,    0,    0,    0,   22,   23,   24,   25,   26,
   27,   28,
};
const short bootdgoto[] = {                                       3,
    4,    5,    6,   12,    7,   22,   23,   24,
};
const short bootsindex[] = {                                   -261,
    0,    0,    0, -261, -261, -255, -235,    0,    0, -253,
    0,    0,    0,    0,    0,    0,    0,    0, -239,    0,
    0, -233, -253,    0, -257,    0,    0,    0, -242, -228,
 -223, -222, -221, -220, -219,    0,    0,    0,    0,    0,
    0,    0,
};
const short bootrindex[] = {                                     44,
    0,    0,    0,   44,   44,    0, -254,    0,    0, -214,
    0,    0,    0,    0,    0,    0,    0,    0, -241,    0,
    0,    0, -214,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
const short bootgindex[] = {                                     20,
   -5,   -4,    0,    0,    0,   23,    0,    0,
};
#define YYTABLESIZE 46
const short boottable[] = {                                      29,
    1,    2,   10,    5,   20,   21,   11,    5,    1,   19,
   30,   31,   32,   33,   34,   35,    6,   20,   21,   36,
   25,    6,   26,    8,    9,   27,    6,   13,    6,    6,
    6,    6,   14,   37,   15,   16,   17,   18,   38,   39,
   40,   41,   42,    1,   15,   28,
};
const short bootcheck[] = {                                     257,
  262,  263,  258,  258,   10,   10,  262,  262,  262,  263,
  268,  269,  270,  271,  272,  273,  258,   23,   23,  262,
  260,  263,  262,    4,    5,  259,  268,  263,  270,  271,
  272,  273,  268,  262,  270,  271,  272,  273,  262,  262,
  262,  262,  262,    0,  259,   23,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 273
#if YYDEBUG
const char * const bootname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"STRING","UPLEVEL","DOWNLEVEL",
"ASSIGN_VALUE","ASSIGN_DEFAULT","END","LITERAL","SLASH","LISTNEXT","TEXT",
"UINT","INTEGER","BOOL","IPV4","IPV4NET","IPV6","IPV6NET",
};
const char * const bootrule[] = {
"$accept : input",
"input :",
"input : definition input",
"input : emptystatement input",
"definition : nodename nodegroup",
"nodename : literals",
"literals : LITERAL",
"literals : literals LITERAL",
"literals : literals IPV4",
"literals : literals IPV4NET",
"literals : literals IPV6",
"literals : literals IPV6NET",
"literals : literals INTEGER",
"nodegroup : UPLEVEL statements DOWNLEVEL",
"nodegroup : END",
"statements :",
"statements : statement statements",
"statement : terminal",
"statement : definition",
"statement : emptystatement",
"emptystatement : END",
"terminal : LITERAL END",
"terminal : LITERAL ASSIGN_VALUE STRING END",
"terminal : LITERAL ASSIGN_VALUE INTEGER END",
"terminal : LITERAL ASSIGN_VALUE BOOL END",
"terminal : LITERAL ASSIGN_VALUE IPV4 END",
"terminal : LITERAL ASSIGN_VALUE IPV4NET END",
"terminal : LITERAL ASSIGN_VALUE IPV6 END",
"terminal : LITERAL ASSIGN_VALUE IPV6NET END",
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
#line 87 "boot.yy"

/* extern FILE *bootin; */
extern void boot_scan_string(const char *);
extern int bootlinenum;

#define MAXSTACK 20
#define MAXPATH 256
static ConfigTree *cf;
static string boot_filename;
static char lastsymbol[256];
extern "C" int bootparse();
extern int bootlex();


string
booterrormsg(const char *s)
{
    string errmsg;

    if (boot_filename.empty()) {
	errmsg = c_format("PARSE ERROR [Config File %s, line %d]: %s\n", 
			  boot_filename.c_str(),
			  bootlinenum, s);
    } else {
	errmsg = c_format("PARSE ERROR [line %d]: %s\n", bootlinenum, s);
    }
    return errmsg;
}

void
booterror(const char *s) throw (ParseError)
{
    xorp_throw(ParseError, booterrormsg(s));
}

static void
extend_path(char *segment)
{
    strncpy(lastsymbol, segment, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    cf->extend_path(string(segment));
    free(segment);
}

static void
push_path()
{
    cf->push_path();
}

static void
pop_path()
{
    cf->pop_path();
}

static void
terminal(char *segment, char *value, int type)
{
    extend_path(segment);
    push_path();

    strncpy(lastsymbol, value, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    cf->terminal_value(value, type);
    free(value);
    pop_path();
}

int
init_bootfile_parser(const char *configuration,
		     const char *filename,
		     ConfigTree *c)
{
    cf = c;
    boot_filename = filename;
    bootlinenum = 1;
    boot_scan_string(configuration);
    return 0;
}

int
parse_bootfile()
{
    bootparse();
    return 0;
}
#line 298 "y.boot_tab.c"
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
#line 37 "boot.yy"
{
			/* printf("DEFINITION\n"); */
		}
break;
case 5:
#line 41 "boot.yy"
{ push_path(); }
break;
case 6:
#line 43 "boot.yy"
{ extend_path(yyvsp[0]); }
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
#line 51 "boot.yy"
{ pop_path(); }
break;
case 14:
#line 52 "boot.yy"
{ pop_path(); }
break;
case 20:
#line 59 "boot.yy"
{ /* printf("EMPTY STATEMENT\n"); */ }
break;
case 21:
#line 61 "boot.yy"
{
			terminal(yyvsp[-1], strdup(""), NODE_VOID);
		}
break;
case 22:
#line 64 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_TEXT);
		}
break;
case 23:
#line 67 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_UINT);
		}
break;
case 24:
#line 70 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_BOOL);
		}
break;
case 25:
#line 73 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV4);
		}
break;
case 26:
#line 76 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV4PREFIX);
		}
break;
case 27:
#line 79 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV6);
		}
break;
case 28:
#line 82 "boot.yy"
{
			terminal(yyvsp[-3], yyvsp[-1], NODE_IPV6PREFIX);
		}
break;
#line 591 "y.boot_tab.c"
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
