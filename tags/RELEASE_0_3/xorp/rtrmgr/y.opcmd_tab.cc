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
  /*sigh - -p flag to yacc should do this for us*/
#define yystacksize opcmdstacksize
#define yysslim opcmdsslim
#line 56 "y.opcmd_tab.c"
#define YYERRCODE 256
#define STRING 257
#define COMMAND 258
#define UPLEVEL 259
#define DOWNLEVEL 260
#define COLON 261
#define END 262
#define LITERAL 263
#define VARIABLE 264
const short opcmdlhs[] = {                                        -1,
    0,    0,    1,    1,    2,    4,    4,    4,    3,    5,
    7,    6,    6,    8,
};
const short opcmdlen[] = {                                         2,
    0,    2,    2,    4,    1,    0,    2,    2,    3,    1,
    1,    1,    2,    4,
};
const short opcmddefred[] = {                                      0,
    5,    0,    0,    0,    2,   10,    0,    3,    0,    0,
    0,    0,    0,    0,    0,    8,    7,    4,    0,   11,
    9,   13,    0,   14,
};
const short opcmddgoto[] = {                                       2,
    3,   11,    8,   12,    9,   14,   21,   15,
};
const short opcmdsindex[] = {                                   -263,
    0,    0, -263, -257,    0,    0, -256,    0, -255, -256,
 -256, -248, -249, -247, -255,    0,    0,    0, -246,    0,
    0,    0, -244,    0,
};
const short opcmdrindex[] = {                                     14,
    0,    0,   14,    0,    0,    0, -248,    0,    0, -248,
 -248,    0,    0,    0, -245,    0,    0,    0,    0,    0,
    0,    0,    0,    0,
};
const short opcmdgindex[] = {                                     13,
    0,    1,    7,   -1,    0,    5,    0,    0,
};
#define YYTABLESIZE 20
const short opcmdtable[] = {                                       1,
    4,    6,   13,    4,    7,    1,    1,   10,   16,   17,
    6,   19,   20,    1,   12,    5,   23,   24,   18,   22,
};
const short opcmdcheck[] = {                                     263,
    0,  259,  258,    3,    4,  263,  263,  264,   10,   11,
  259,  261,  260,    0,  260,    3,  263,  262,   12,   15,
};
#define YYFINAL 2
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
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"STRING","COMMAND","UPLEVEL",
"DOWNLEVEL","COLON","END","LITERAL","VARIABLE",
};
const char * const opcmdrule[] = {
"$accept : input",
"input :",
"input : definition input",
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
#line 63 "op_commands.yy"

extern FILE *opcmdin;
extern int op_linenum;
static const char *opcmd_filename;
static char lastsymbol[256];

static string current_cmd;
static list <string> cmd_list;
static OpCommandList *ocl;
extern "C" int opcmdparse();
extern int opcmdlex();

void opcmderror (const char *s) {
    printf("\n ERROR [Operational Command File: %s line %d]: %s\n", 
	   opcmd_filename, 
	   op_linenum, s);
    printf("\n Last symbol parsed was %s\n", lastsymbol);
    exit(1);
}


static void append_path(char *segment) {
    strncpy(lastsymbol, segment, 255);
    string segname;
    segname = segment;
    ocl->append_path(segname);
    free(segment);
}

static void push_path() {
    ocl->push_path();
}

static void pop_path() {
    ocl->pop_path();
}

static void add_cmd(char *cmd) {
    strncpy(lastsymbol, cmd, 255);
    ocl->add_cmd(cmd);
    current_cmd = cmd;
    free(cmd);
    cmd_list.clear();
}

static void append_cmd(char *s) {
    strncpy(lastsymbol, s, 255);
    cmd_list.push_back(string(s));
#ifdef DEBUG_OPCMD_PARSER
    printf("cmd_str now >");
    list <string>::const_iterator i;
    for (i = cmd_list.begin(); i!=cmd_list.end(); ++i) {	
	printf("%s ", i->c_str());
    }
    printf("\n");	
#endif
    free(s);
}

static void end_cmd() {
#ifdef DEBUG_OPCMD_PARSER
    printf("end_cmd\n");
#endif	
    ocl->add_cmd_action(current_cmd, cmd_list); 
    cmd_list.clear();
}

int init_opcmd_parser (const char *filename, OpCommandList *o) {
    ocl = o;
    op_linenum = 1;
    opcmdin = fopen(filename, "r");
    if (opcmdin == NULL)
	return -1;
    opcmd_filename = filename;
    return 0;
}

int parse_opcmd() {
    opcmdparse();
    return 0;
}






#line 256 "y.opcmd_tab.c"
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
case 3:
#line 25 "op_commands.yy"
{
	}
break;
case 4:
#line 27 "op_commands.yy"
{
	}
break;
case 5:
#line 30 "op_commands.yy"
{
		append_path(yyvsp[0]);
	}
break;
case 8:
#line 37 "op_commands.yy"
{ 
		append_path(yyvsp[-1]); 
	}
break;
case 9:
#line 41 "op_commands.yy"
{
	}
break;
case 10:
#line 44 "op_commands.yy"
{
		push_path();
	}
break;
case 11:
#line 48 "op_commands.yy"
{
		pop_path();
	}
break;
case 14:
#line 55 "op_commands.yy"
{
		add_cmd(yyvsp[-3]);
		append_cmd(yyvsp[-1]);
		end_cmd();
	}
break;
#line 498 "y.opcmd_tab.c"
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
