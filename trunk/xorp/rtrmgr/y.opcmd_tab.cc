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
#include "rtrmgr_module.h"
#include "op_commands.hh"
/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize opcmdstacksize
#define yysslim opcmdsslim
#line 56 "y.opcmd_tab.c"
#define YYERRCODE 256
#define UPLEVEL 257
#define DOWNLEVEL 258
#define END 259
#define COLON 260
#define COMMAND 261
#define VARIABLE 262
#define LITERAL 263
#define SYNTAX_ERROR 264
const short opcmdlhs[] = {                                        -1,
    0,    0,    0,    1,    1,    3,    5,    5,    5,    4,
    6,    8,    7,    7,    9,    2,
};
const short opcmdlen[] = {                                         2,
    0,    2,    1,    2,    4,    1,    0,    2,    2,    3,
    1,    1,    1,    2,    4,    1,
};
const short opcmddefred[] = {                                      0,
    6,   16,    0,    0,    3,    0,    2,   11,    0,    4,
    0,    0,    0,    0,    0,    0,    0,    9,    8,    5,
    0,   12,   10,   14,    0,   15,
};
const short opcmddgoto[] = {                                       3,
    4,    5,   13,   10,   14,   11,   16,   23,   17,
};
const short opcmdsindex[] = {                                   -261,
    0,    0,    0, -261,    0, -257,    0,    0, -254,    0,
 -249, -254, -254, -253, -247, -244, -249,    0,    0,    0,
 -248,    0,    0,    0, -243,    0,
};
const short opcmdrindex[] = {                                     17,
    0,    0,    0,   17,    0,    0,    0,    0, -239,    0,
    0, -239, -239,    0,    0,    0, -238,    0,    0,    0,
    0,    0,    0,    0,    0,    0,
};
const short opcmdgindex[] = {                                     15,
    0,    0,    1,    7,   -2,    0,    5,    0,    0,
};
#define YYTABLESIZE 22
const short opcmdtable[] = {                                       8,
    6,    1,    2,    8,    6,    1,    9,   12,    1,   18,
   19,   15,   21,   22,   25,   26,    1,    7,    7,   13,
   20,   24,
};
const short opcmdcheck[] = {                                     257,
    0,  263,  264,  257,    4,  263,    6,  262,  263,   12,
   13,  261,  260,  258,  263,  259,    0,  257,    4,  258,
   14,   17,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 264
#if YYDEBUG
const char * const opcmdname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"UPLEVEL","DOWNLEVEL","END",
"COLON","COMMAND","VARIABLE","LITERAL","SYNTAX_ERROR",
};
const char * const opcmdrule[] = {
"$accept : input",
"input :",
"input : definition input",
"input : syntax_error",
"definition : word specification",
"definition : word word word_or_var_list specification",
"word : LITERAL",
"word_or_var_list :",
"word_or_var_list : word word_or_var_list",
"word_or_var_list : VARIABLE word_or_var_list",
"specification : startspec attributes endspec",
"startspec : UPLEVEL",
"endspec : DOWNLEVEL",
"attributes : attribute",
"attributes : attribute attributes",
"attribute : COMMAND COLON LITERAL END",
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
#line 69 "op_commands.yy"

extern "C" int opcmdparse();
extern int opcmdlex();
extern FILE *opcmdin;
extern int opcmd_linenum;

static OpCommandList *ocl = NULL;
static string opcmd_filename;
static string lastsymbol;
static string current_cmd;
static list<string> cmd_list;


static void
append_path(char *segment)
{
    lastsymbol = segment;

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
    lastsymbol = cmd;

    ocl->add_cmd(cmd);
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
end_cmd()
{
    ocl->add_cmd_action(current_cmd, cmd_list);
    cmd_list.clear();
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
#line 269 "y.opcmd_tab.c"
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
#line 29 "op_commands.yy"
{ }
break;
case 5:
#line 30 "op_commands.yy"
{ }
break;
case 6:
#line 33 "op_commands.yy"
{ append_path(yyvsp[0]); }
break;
case 9:
#line 39 "op_commands.yy"
{ append_path(yyvsp[-1]); }
break;
case 10:
#line 42 "op_commands.yy"
{ }
break;
case 11:
#line 45 "op_commands.yy"
{ push_path(); }
break;
case 12:
#line 48 "op_commands.yy"
{ pop_path(); }
break;
case 15:
#line 55 "op_commands.yy"
{
			add_cmd(yyvsp[-3]);
			append_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 16:
#line 62 "op_commands.yy"
{
			opcmderror("syntax error");
		}
break;
#line 506 "y.opcmd_tab.c"
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
