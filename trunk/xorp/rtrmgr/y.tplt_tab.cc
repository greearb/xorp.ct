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
#include "template_tree_node.hh"
#include "template_tree.hh"
/* #define DEBUG_TEMPLATE_PARSER */
/* XXX: sigh - -p flag to yacc should do this for us */
#define yystacksize tpltstacksize
#define yysslim tpltsslim
#line 58 "y.tplt_tab.c"
#define YYERRCODE 256
#define STRING 257
#define COMMAND 258
#define UPLEVEL 259
#define DOWNLEVEL 260
#define COLON 261
#define ASSIGN_DEFAULT 262
#define END 263
#define LITERAL 264
#define SLASH 265
#define VARIABLE 266
#define VARDEF 267
#define LISTNEXT 268
#define RETURN 269
#define TEXT 270
#define UINT 271
#define INT 272
#define BOOL 273
#define TOGGLE 274
#define MACADDR 275
#define IPV4 276
#define IPV4PREFIX 277
#define IPV6 278
#define IPV6PREFIX 279
#define INTEGER 280
#define BOOL_INITIALIZER 281
#define IPV4_INITIALIZER 282
#define IPV4PREFIX_INITIALIZER 283
#define IPV6_INITIALIZER 284
#define IPV6PREFIX_INITIALIZER 285
#define MACADDR_INITIALIZER 286
const short tpltlhs[] = {                                        -1,
    0,    0,    1,    2,    2,    5,    5,    4,    4,    4,
    6,    6,    6,    6,    6,    6,    6,    6,    6,    6,
    7,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    3,    8,    8,    9,    9,    9,   10,   10,   13,   12,
   11,   11,   14,   15,   16,   17,   17,   18,   18,   18,
   18,   18,   18,   18,   18,   19,   19,
};
const short tpltlen[] = {                                         2,
    0,    2,    2,    1,    1,    2,    4,    1,    2,    2,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    0,    2,    1,    1,    1,    1,    1,    4,    4,
    1,    1,    4,    3,    1,    1,    3,    2,    4,    3,
    2,    3,    1,    2,    1,    1,    2,
};
const short tpltdefred[] = {                                      0,
    0,    0,    0,    0,    0,    5,    0,    2,    0,    3,
    0,   10,    0,   45,    0,   36,    0,    0,   34,   35,
   37,   38,   41,   42,    0,   11,   12,   13,   14,   15,
   20,   16,   17,   18,   19,    7,    0,   31,   33,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   44,    0,    0,    0,   46,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   39,   40,    0,
    0,    0,   48,    0,   54,   43,    0,   21,   22,   23,
   24,   25,   30,   26,   27,   28,   29,    0,   57,   52,
   50,   47,   49,
};
const short tpltdgoto[] = {                                       2,
    3,    4,   10,    5,    6,   36,   52,   17,   18,   19,
   20,   21,   22,   23,   24,   25,   56,   57,   89,
};
const short tpltsindex[] = {                                   -238,
 -237,    0, -238, -207, -220,    0, -202,    0, -227,    0,
 -237,    0, -268,    0, -228,    0, -199, -227,    0,    0,
    0,    0,    0,    0, -198,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -255,    0,    0, -206,
 -200, -197, -196, -195, -194, -193, -192, -191, -190, -189,
 -188, -187,    0, -232, -183, -225,    0, -180, -216, -201,
 -203, -186, -205, -185, -184, -204, -182,    0,    0, -256,
 -175, -268,    0, -183,    0,    0, -252,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -174,    0,    0,
    0,    0,    0,
};
const short tpltrindex[] = {                                     84,
 -219,    0,   84,    0, -173,    0, -218,    0, -172,    0,
 -217,    0,    0,    0, -219,    0,    0, -172,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
 -178, -176, -171, -170, -169, -167, -165, -163, -162, -161,
    0,    0,    0, -215, -214,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -213,
 -212,    0,    0, -213,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,
};
const short tpltgindex[] = {                                     86,
   18,    0,    0,    0,   85,  -37,    0,   73,    0,    0,
    0,    0,    0,    0,    0,    0,    0,   27,  -26,
};
#define YYTABLESIZE 104
const short tplttable[] = {                                      51,
   74,   26,   27,   28,   29,   30,   31,   32,   33,   34,
   35,   54,   88,   55,   41,   42,   43,   44,   45,   46,
   47,   48,   49,   50,   70,    1,   16,   73,   75,    7,
   14,   71,   37,   72,   91,   16,   15,   76,    7,    8,
    6,    9,   77,   11,    8,    6,    9,   55,   53,   56,
   51,    9,   55,   53,   56,   51,   53,   54,   13,   55,
   38,   58,   40,   79,   59,   60,   61,   62,   63,   64,
   65,   66,   67,   74,   68,   69,   78,   81,   80,   86,
   83,   90,   93,    1,   11,    4,   12,   32,    8,   12,
   39,   13,   14,   15,   82,   20,   84,   16,   85,   17,
   18,   19,   87,   92,
};
const short tpltcheck[] = {                                      37,
  257,  270,  271,  272,  273,  274,  275,  276,  277,  278,
  279,  264,  269,  266,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  257,  264,    9,   54,   55,  267,
  258,  264,  261,  266,   72,   18,  264,  263,  267,  259,
  259,  259,  268,  264,  264,  264,  264,  263,  263,  263,
  263,  259,  268,  268,  268,  268,  263,  264,  261,  266,
  260,  262,  261,  280,  262,  262,  262,  262,  262,  262,
  262,  262,  262,  257,  263,  263,  257,  281,  280,  284,
  286,  257,  257,    0,  263,  259,  263,  260,    3,    5,
   18,  263,  263,  263,  281,  263,  282,  263,  283,  263,
  263,  263,  285,   77,
};
#define YYFINAL 2
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 286
#if YYDEBUG
const char * const tpltname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"STRING","COMMAND","UPLEVEL",
"DOWNLEVEL","COLON","ASSIGN_DEFAULT","END","LITERAL","SLASH","VARIABLE",
"VARDEF","LISTNEXT","RETURN","TEXT","UINT","INT","BOOL","TOGGLE","MACADDR",
"IPV4","IPV4PREFIX","IPV6","IPV6PREFIX","INTEGER","BOOL_INITIALIZER",
"IPV4_INITIALIZER","IPV4PREFIX_INITIALIZER","IPV6_INITIALIZER",
"IPV6PREFIX_INITIALIZER","MACADDR_INITIALIZER",
};
const char * const tpltrule[] = {
"$accept : input",
"input :",
"input : definition input",
"definition : nodename nodegroup",
"nodename : literals",
"nodename : named_literal",
"named_literal : LITERAL VARDEF",
"named_literal : LITERAL VARDEF COLON type",
"literals : LITERAL",
"literals : literals LITERAL",
"literals : literals named_literal",
"type : TEXT",
"type : UINT",
"type : INT",
"type : BOOL",
"type : TOGGLE",
"type : IPV4",
"type : IPV4PREFIX",
"type : IPV6",
"type : IPV6PREFIX",
"type : MACADDR",
"init_type : TEXT ASSIGN_DEFAULT STRING",
"init_type : UINT ASSIGN_DEFAULT INTEGER",
"init_type : INT ASSIGN_DEFAULT INTEGER",
"init_type : BOOL ASSIGN_DEFAULT BOOL_INITIALIZER",
"init_type : TOGGLE ASSIGN_DEFAULT BOOL_INITIALIZER",
"init_type : IPV4 ASSIGN_DEFAULT IPV4_INITIALIZER",
"init_type : IPV4PREFIX ASSIGN_DEFAULT IPV4PREFIX_INITIALIZER",
"init_type : IPV6 ASSIGN_DEFAULT IPV6_INITIALIZER",
"init_type : IPV6PREFIX ASSIGN_DEFAULT IPV6PREFIX_INITIALIZER",
"init_type : MACADDR ASSIGN_DEFAULT MACADDR_INITIALIZER",
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
"cmd : LITERAL list_of_cmd_strings",
"cmd : LITERAL STRING RETURN STRING",
"cmd : LITERAL VARIABLE type",
"cmd : LITERAL LITERAL",
"cmd : LITERAL LITERAL STRING",
"cmd : VARIABLE",
"cmd : VARIABLE list_of_cmd_strings",
"cmd : LITERAL",
"list_of_cmd_strings : STRING",
"list_of_cmd_strings : STRING list_of_cmd_strings",
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
#line 194 "template.yy"

extern char *lstr;
extern char *vstr;
extern char *cstr;
extern char *sstr;
extern char *istr;
extern FILE *tpltin;
extern int linenum;
static int tplt_type;
static char *tplt_initializer;
static const char *tplt_filename;
static char lastsymbol[256];

#define MAXSTACK 20
#define MAXPATH 256
/* static string cmd_str; */
static string current_cmd;
/* static string path_hold; */
static list<string> cmd_list;
static TemplateTree* tt;
extern "C" int tpltparse();
extern int tpltlex();

void
tplterror(const char *s)
{
    printf("\n ERROR [Template File: %s line %d]: %s\n",
	tplt_filename, linenum, s);
    printf("\n Last symbol parsed was %s\n", lastsymbol);
    exit(1);
}

static void
extend_path(char *segment, bool is_tag)
{
    strncpy(lastsymbol, segment, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    string segname;
    segname = segment;
    tt->extend_path(segname, is_tag);
    /* free(segment); */
    /* printf("\n>>> extend path: %s\n", path); */
}

static void
push_path()
{
    /* printf("\n>>>PUSH: %s\n", path); */
    tt->push_path(tplt_type, tplt_initializer);
    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
}

static void
pop_path()
{
    tt->pop_path();
    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
    /* printf("\n>>>POP: %s\n", path); */
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
    strncpy(lastsymbol, cmd, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    tt->add_cmd(cmd);
    current_cmd = cmd;
    free(cmd);
    cmd_list.clear();
}

static void
append_cmd(char *s)
{
    strncpy(lastsymbol, s, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    cmd_list.push_back(string(s));

#ifdef DEBUG_TEMPLATE_PARSER
    printf("cmd_str now >");
    list<string>::const_iterator iter;
    for (iter = cmd_list.begin(); iter != cmd_list.end(); ++iter) {
	printf("%s ", iter->c_str());
    }
    printf("\n");
#endif /* DEBUG_TEMPLATE_PARSER */

    free(s);
}

static void
prepend_cmd(char *s)
{
    strncpy(lastsymbol, s, sizeof(lastsymbol) - 1);
    lastsymbol[sizeof(lastsymbol) - 1] = '\0';

    cmd_list.push_front(string(s));

#ifdef DEBUG_TEMPLATE_PARSER
    printf("cmd_str now >");
    list<string>::const_iterator iter;
    for (iter = cmd_list.begin(); iter != cmd_list.end(); ++iter) {
	printf("%s ", iter->c_str());
    }
    printf("\n");
#endif /* DEBUG_TEMPLATE_PARSER */

    free(s);
}

static void
end_cmd()
{
#ifdef DEBUG_TEMPLATE_PARSER
    printf("end_cmd\n");
#endif

    tt->add_cmd_action(current_cmd, cmd_list);
    cmd_list.clear();
}

int
init_template_parser (const char *filename, TemplateTree *c)
{
    tt = c;
    linenum = 1;

    tpltin = fopen(filename, "r");
    if (tpltin == NULL)
	return -1;

    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
    tplt_filename = filename;
    return 0;
}

int
parse_template()
{
    tpltparse();
    return 0;
}
#line 445 "y.tplt_tab.c"
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
#line 52 "template.yy"
{ push_path(); }
break;
case 5:
#line 53 "template.yy"
{ push_path(); }
break;
case 6:
#line 55 "template.yy"
{
			extend_path(yyvsp[-1], true);
			extend_path(yyvsp[0], false);
		}
break;
case 7:
#line 59 "template.yy"
{
			extend_path(yyvsp[-3], true);
			extend_path(yyvsp[-2], false);
		}
break;
case 8:
#line 64 "template.yy"
{ extend_path(yyvsp[0], false); }
break;
case 9:
#line 65 "template.yy"
{ extend_path(yyvsp[0], false); }
break;
case 11:
#line 68 "template.yy"
{ tplt_type = NODE_TEXT; }
break;
case 12:
#line 69 "template.yy"
{ tplt_type = NODE_UINT; }
break;
case 13:
#line 70 "template.yy"
{ tplt_type = NODE_INT; }
break;
case 14:
#line 71 "template.yy"
{ tplt_type = NODE_BOOL; }
break;
case 15:
#line 72 "template.yy"
{ tplt_type = NODE_TOGGLE; }
break;
case 16:
#line 73 "template.yy"
{ tplt_type = NODE_IPV4; }
break;
case 17:
#line 74 "template.yy"
{ tplt_type = NODE_IPV4PREFIX; }
break;
case 18:
#line 75 "template.yy"
{ tplt_type = NODE_IPV6; }
break;
case 19:
#line 76 "template.yy"
{ tplt_type = NODE_IPV6PREFIX; }
break;
case 20:
#line 77 "template.yy"
{ tplt_type = NODE_MACADDR; }
break;
case 21:
#line 79 "template.yy"
{
			tplt_type = NODE_TEXT;
			tplt_initializer = yyvsp[0];
		}
break;
case 22:
#line 83 "template.yy"
{
			tplt_type = NODE_UINT;
			tplt_initializer = yyvsp[0];
		}
break;
case 23:
#line 87 "template.yy"
{
			tplt_type = NODE_INT;
			tplt_initializer = yyvsp[0];
		}
break;
case 24:
#line 91 "template.yy"
{
			tplt_type = NODE_BOOL;
			tplt_initializer = yyvsp[0];
		}
break;
case 25:
#line 95 "template.yy"
{
			tplt_type = NODE_TOGGLE;
			tplt_initializer = yyvsp[0];
		}
break;
case 26:
#line 99 "template.yy"
{
			tplt_type = NODE_IPV4;
			tplt_initializer = yyvsp[0];
		}
break;
case 27:
#line 103 "template.yy"
{
			tplt_type = NODE_IPV4PREFIX;
			tplt_initializer = yyvsp[0];
		}
break;
case 28:
#line 107 "template.yy"
{
			tplt_type = NODE_IPV6;
			tplt_initializer = yyvsp[0];
		}
break;
case 29:
#line 111 "template.yy"
{
			tplt_type = NODE_IPV6PREFIX;
			tplt_initializer = yyvsp[0];
		}
break;
case 30:
#line 115 "template.yy"
{
			tplt_type = NODE_MACADDR;
			tplt_initializer = yyvsp[0];
		}
break;
case 31:
#line 120 "template.yy"
{ pop_path(); }
break;
case 39:
#line 129 "template.yy"
{ terminal(yyvsp[-3]); }
break;
case 40:
#line 131 "template.yy"
{ terminal(yyvsp[-3]); }
break;
case 45:
#line 140 "template.yy"
{ add_cmd(yyvsp[0]); }
break;
case 48:
#line 145 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 49:
#line 149 "template.yy"
{
			append_cmd(yyvsp[-3]);
			append_cmd(yyvsp[-2]);
			append_cmd(strdup("return"));
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 50:
#line 156 "template.yy"
{ /* eg: set FOOBAR ipv4 */
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 51:
#line 162 "template.yy"
{
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 52:
#line 167 "template.yy"
{
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 53:
#line 173 "template.yy"
{
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 54:
#line 177 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 55:
#line 181 "template.yy"
{
			append_cmd(yyvsp[0]);
		}
break;
case 56:
#line 186 "template.yy"
{
			prepend_cmd(yyvsp[0]);
		}
break;
case 57:
#line 189 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
		}
break;
#line 871 "y.tplt_tab.c"
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
