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

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"

#include "conf_tree.hh"
#include "conf_tree_node.hh"
#include "template_tree.hh"
#include "template_tree_node.hh"

/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize bootstacksize
#define yysslim bootsslim
#line 64 "y.boot_tab.c"
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
#define COMPARATOR 275
#define MODIFIER 276
#define SYNTAX_ERROR 277
#define LINENUM 278
const short bootlhs[] = {                                        -1,
    0,    0,    0,    0,    1,    1,    6,    4,    8,    8,
    9,    9,    9,    9,    9,    9,    9,    9,    9,    9,
    9,    9,    9,    5,    5,    7,   10,   10,   11,   11,
   11,    2,   13,   13,   12,   12,   12,   12,   12,   12,
   12,   12,   12,   12,   12,   12,   12,   12,   12,   12,
   12,    3,
};
const short bootlen[] = {                                         2,
    0,    2,    2,    1,    2,    2,    1,    1,    1,    2,
    2,    2,    2,    2,    2,    2,    2,    2,    2,    2,
    2,    2,    2,    1,    1,    3,    0,    2,    1,    1,
    1,    1,    1,    2,    2,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    1,
};
const short bootdefred[] = {                                      0,
   32,    9,   52,    0,    0,    0,    0,    4,    0,    0,
    0,    0,   10,    2,    3,    0,   25,    5,   24,    6,
   13,   14,   15,   16,   17,   18,   19,   20,   21,   22,
   23,   12,   11,    0,    0,   30,   31,    0,    0,   29,
    0,    0,   26,   28,   35,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,   37,   38,   39,   40,   41,   42,
   43,   44,   45,   46,   47,   36,   49,   48,   51,   50,
};
const short bootdgoto[] = {                                       5,
    6,    7,    8,    9,   18,   10,   19,   11,   12,   38,
   39,   40,   41,
};
const short bootsindex[] = {                                   -218,
    0,    0,    0, -269,    0, -218, -218,    0, -209, -235,
 -171, -227,    0,    0,    0, -217,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -246,    0,    0, -206, -217,    0,
 -213,    0,    0,    0,    0, -197, -187, -185, -210, -202,
 -181, -180, -179, -178, -177, -176, -175, -174, -173, -157,
 -156, -155, -154, -153,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
const short bootrindex[] = {                                     53,
    0,    0,    0,    0,    0,   53,   53,    0,    0,    0,
 -169, -201,    0,    0,    0, -151,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0, -255,    0,    0,    0,    0, -151,    0,
    0, -232,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
};
const short bootgindex[] = {                                     12,
  -16,  -15,    0,    0,    0,    0,   98,   97,    0,   71,
    0,    0,    0,
};
#define YYTABLESIZE 110
const short boottable[] = {                                      36,
   37,    9,   13,   33,   33,    9,    9,    9,    9,    9,
    9,    9,    9,    9,    9,    9,    9,   14,   15,   33,
   33,   16,   36,   37,   10,   42,   34,   34,   10,   10,
   10,   10,   10,   10,   10,   10,   10,   10,   10,   10,
    1,    1,   34,   34,    2,   45,   46,   16,   65,   17,
    4,   43,    1,    2,   34,    8,   66,    8,    3,    4,
   35,   47,   48,   49,   50,   51,   52,   53,   54,   55,
   56,   57,   58,   59,   61,   60,   63,   67,   68,   69,
   70,   71,   72,   73,   74,   75,   62,    7,   64,   21,
   22,   23,   24,   25,   26,   27,   28,   29,   30,   31,
   32,   76,   77,   78,   79,   80,   27,   20,   33,   44,
};
const short bootcheck[] = {                                      16,
   16,  257,  272,  259,  260,  261,  262,  263,  264,  265,
  266,  267,  268,  269,  270,  271,  272,    6,    7,  275,
  276,  257,   39,   39,  257,  272,  259,  260,  261,  262,
  263,  264,  265,  266,  267,  268,  269,  270,  271,  272,
  259,  259,  275,  276,  272,  259,  260,  257,  259,  259,
  278,  258,    0,  272,  272,  257,  259,  259,  277,  278,
  278,  275,  276,  261,  262,  263,  264,  265,  266,  267,
  268,  269,  270,  271,  262,  273,  262,  259,  259,  259,
  259,  259,  259,  259,  259,  259,  274,  257,  274,  261,
  262,  263,  264,  265,  266,  267,  268,  269,  270,  271,
  272,  259,  259,  259,  259,  259,  258,   10,   12,   39,
};
#define YYFINAL 5
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 278
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
"URL_HTTP_VALUE","URL_TFTP_VALUE","LITERAL","STRING","ARITH","COMPARATOR",
"MODIFIER","SYNTAX_ERROR","LINENUM",
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
"terminal : term_literal ASSIGN_OPERATOR STRING END",
"terminal : term_literal ASSIGN_OPERATOR BOOL_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR UINT_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR IPV4_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR IPV4NET_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR IPV6_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR IPV6NET_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR MACADDR_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR URL_FILE_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR URL_FTP_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR URL_HTTP_VALUE END",
"terminal : term_literal ASSIGN_OPERATOR URL_TFTP_VALUE END",
"terminal : term_literal COMPARATOR ARITH END",
"terminal : term_literal COMPARATOR UINT_VALUE END",
"terminal : term_literal MODIFIER ARITH END",
"terminal : term_literal MODIFIER UINT_VALUE END",
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
#line 167 "boot.yy"

extern void boot_scan_string(const char *configuration);
extern int boot_linenum;
extern "C" int bootparse();
extern int bootlex();

static ConfigTree *config_tree = NULL;
static string boot_filename;
static string lastsymbol;
static uint64_t nodenum;


static void
extend_path(char *segment, int type, uint64_t node_num)
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
terminal(const string& value, int type, ConfigOperator op)
{
    push_path();

    lastsymbol = value;

    config_tree->terminal_value(value, type, op);
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

ConfigOperator lookup_comparator(char *s)
{
    char *s1, *s2;

    /* skip leading spaces */
    s1 = s;
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

    if (strcmp(s1, "==")==0) {
	return OP_EQ;
    } else if (strcmp(s1, "<")==0) {
	return OP_LT;
    } else if (strcmp(s1, "<=")==0) {
	return OP_LTE;
    } else if (strcmp(s1, ">")==0) {
	return OP_GT;
    } else if (strcmp(s1, ">=")==0) {
	return OP_GTE;
    } 

    /*something's wrong*/
    string errmsg;
    errmsg = c_format("Bad comparator %s [Line %d]", s1, boot_linenum);
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());
    xorp_throw(ParseError, errmsg);
}

ConfigOperator lookup_modifier(char *s)
{
    char *s1, *s2;

    /* skip leading spaces */
    s1 = s;
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

    if (strcmp(s1, ":")==0) {
	return OP_ASSIGN;
    } else if (strcmp(s1, "=")==0) {
	return OP_ASSIGN;
    } else if (strcmp(s1, "+")==0) {
	return OP_ADD;
    } else if (strcmp(s1, "add")==0) {
	return OP_ADD;
    } else if (strcmp(s1, "-")==0) {
	return OP_SUB;
    } else if (strcmp(s1, "sub")==0) {
	return OP_SUB;
    }

    /*something's wrong*/
    string errmsg;
    errmsg = c_format("Bad modifier %s [Line %d]", s1, boot_linenum);
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());
    xorp_throw(ParseError, errmsg);
}
#line 434 "y.boot_tab.c"
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
#line 55 "boot.yy"
{ push_path(); }
break;
case 8:
#line 58 "boot.yy"
{ push_path(); }
break;
case 9:
#line 61 "boot.yy"
{ nodenum += 100;
                          extend_path(yyvsp[0], NODE_VOID, nodenum); }
break;
case 10:
#line 63 "boot.yy"
{ nodenum = strtoll(yyvsp[-1], (char **)NULL, 10);
				    extend_path(yyvsp[0], NODE_VOID, nodenum); }
break;
case 12:
#line 68 "boot.yy"
{ extend_path(yyvsp[0], NODE_TEXT, nodenum); }
break;
case 13:
#line 69 "boot.yy"
{ extend_path(yyvsp[0], NODE_BOOL, nodenum); }
break;
case 14:
#line 70 "boot.yy"
{ extend_path(yyvsp[0], NODE_UINT, nodenum); }
break;
case 15:
#line 71 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4, nodenum); }
break;
case 16:
#line 72 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV4NET, nodenum); }
break;
case 17:
#line 73 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6, nodenum); }
break;
case 18:
#line 74 "boot.yy"
{ extend_path(yyvsp[0], NODE_IPV6NET, nodenum); }
break;
case 19:
#line 75 "boot.yy"
{ extend_path(yyvsp[0], NODE_MACADDR, nodenum); }
break;
case 20:
#line 76 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_FILE, nodenum); }
break;
case 21:
#line 77 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_FTP, nodenum); }
break;
case 22:
#line 78 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_HTTP, nodenum); }
break;
case 23:
#line 79 "boot.yy"
{ extend_path(yyvsp[0], NODE_URL_TFTP, nodenum); }
break;
case 25:
#line 83 "boot.yy"
{ pop_path(); }
break;
case 26:
#line 86 "boot.yy"
{ pop_path(); }
break;
case 33:
#line 101 "boot.yy"
{ nodenum += 100; 
			  extend_path(yyvsp[0], NODE_VOID, nodenum); }
break;
case 34:
#line 103 "boot.yy"
{ nodenum = strtoll(yyvsp[-1], (char **)NULL, 10);
				    extend_path(yyvsp[0], NODE_VOID, nodenum);}
break;
case 35:
#line 107 "boot.yy"
{
			terminal(string(""), NODE_VOID, OP_NONE);
		}
break;
case 36:
#line 110 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_TEXT, OP_ASSIGN);
		}
break;
case 37:
#line 113 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_BOOL, OP_ASSIGN);
		}
break;
case 38:
#line 116 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_UINT, OP_ASSIGN);
		}
break;
case 39:
#line 119 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_IPV4, OP_ASSIGN);
		}
break;
case 40:
#line 122 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_IPV4NET, OP_ASSIGN);
		}
break;
case 41:
#line 125 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_IPV6, OP_ASSIGN);
		}
break;
case 42:
#line 128 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_IPV6NET, OP_ASSIGN);
		}
break;
case 43:
#line 131 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_MACADDR, OP_ASSIGN);
		}
break;
case 44:
#line 134 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_URL_FILE, OP_ASSIGN);
		}
break;
case 45:
#line 137 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_URL_FTP, OP_ASSIGN);
		}
break;
case 46:
#line 140 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_URL_HTTP, OP_ASSIGN);
		}
break;
case 47:
#line 143 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_URL_TFTP, OP_ASSIGN);
		}
break;
case 48:
#line 146 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_ARITH, lookup_comparator(yyvsp[-2]));
		}
break;
case 49:
#line 149 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_UINT, lookup_comparator(yyvsp[-2]));
		}
break;
case 50:
#line 152 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_ARITH, lookup_modifier(yyvsp[-2]));
		}
break;
case 51:
#line 155 "boot.yy"
{
			terminal(string(yyvsp[-2]), NODE_UINT, lookup_modifier(yyvsp[-2]));
		}
break;
case 52:
#line 160 "boot.yy"
{
			booterror("syntax error");
		}
break;
#line 821 "y.boot_tab.c"
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
