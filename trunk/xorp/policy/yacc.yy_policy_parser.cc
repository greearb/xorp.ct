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
#define yyparse yy_policy_parserparse
#define yylex yy_policy_parserlex
#define yyerror yy_policy_parsererror
#define yychar yy_policy_parserchar
#define yyval yy_policy_parserval
#define yylval yy_policy_parserlval
#define yydebug yy_policy_parserdebug
#define yynerrs yy_policy_parsernerrs
#define yyerrflag yy_policy_parsererrflag
#define yyss yy_policy_parserss
#define yyssp yy_policy_parserssp
#define yyvs yy_policy_parservs
#define yyvsp yy_policy_parservsp
#define yylhs yy_policy_parserlhs
#define yylen yy_policy_parserlen
#define yydefred yy_policy_parserdefred
#define yydgoto yy_policy_parserdgoto
#define yysindex yy_policy_parsersindex
#define yyrindex yy_policy_parserrindex
#define yygindex yy_policy_parsergindex
#define yytable yy_policy_parsertable
#define yycheck yy_policy_parsercheck
#define yyname yy_policy_parsername
#define yyrule yy_policy_parserrule
#define yysslim yy_policy_parsersslim
#define yystacksize yy_policy_parserstacksize
#define YYPREFIX "yy_policy_parser"
#line 2 "policy.y"
/*
 * Grammar may be simplified, by allowing "any structure", semantic checking is
 * done at run time anyway...
 * By any structure i mean that you may add / multiple boolean expressions for
 * example. This will give more run time flexibility
 *
 * yacc -d -p yy_policy_parser -o yacc.yy_policy_parser.cc policy.y
 *
 * XXX: with my version of yacc i need to move the #include <stdlib.h> under the
 * yyrcsid
 */

#include "policy_module.h"

#include "libxorp/xorp.h"

#include <vector>

#include "policy/common/element.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"

#include "policy_parser.hh"


extern int yylex(void);

extern void yyerror(const char *m);

using namespace policy_parser;

static ElementFactory _ef;

#line 37 "policy.y"
typedef union {
	char *c_str;
	Node *node;
} YYSTYPE;
#line 86 "yacc.yy_policy_parser.cc"
#define YYERRCODE 256
#define YY_BOOL 257
#define YY_INT 258
#define YY_UINT 259
#define YY_UINTRANGE 260
#define YY_STR 261
#define YY_ID 262
#define YY_IPV4 263
#define YY_IPV4RANGE 264
#define YY_IPV4NET 265
#define YY_IPV6 266
#define YY_IPV6RANGE 267
#define YY_IPV6NET 268
#define YY_NOT 269
#define YY_AND 270
#define YY_XOR 271
#define YY_OR 272
#define YY_HEAD 273
#define YY_CTR 274
#define YY_NE_INT 275
#define YY_EQ 276
#define YY_NE 277
#define YY_LE 278
#define YY_GT 279
#define YY_LT 280
#define YY_GE 281
#define YY_ADD 282
#define YY_SUB 283
#define YY_MUL 284
#define YY_SEMICOLON 285
#define YY_LPAR 286
#define YY_RPAR 287
#define YY_ASSIGN 288
#define YY_SET 289
#define YY_REGEX 290
#define YY_ACCEPT 291
#define YY_REJECT 292
#define YY_PROTOCOL 293
const short yy_policy_parserlhs[] = {                                        -1,
    0,    0,    0,    1,    1,    1,    2,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
};
const short yy_policy_parserlen[] = {                                         2,
    2,    2,    0,    4,    2,    2,    2,    3,    2,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    2,    3,    3,    1,    1,    1,
    1,    1,    1,    2,    1,    1,    1,    1,    1,    1,
};
const short yy_policy_parserdefred[] = {                                      3,
    0,   32,   31,   29,   30,   28,    0,   35,   36,   39,
   37,   38,   40,    0,    0,    0,    0,    0,    0,    0,
    0,    1,    2,    0,    0,    0,   33,    9,    0,    0,
    0,    0,    0,   34,    5,    6,    0,    0,    0,    0,
    7,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,   20,   27,    8,   10,   11,
   12,    0,    0,    0,    0,    0,    0,    0,    0,    0,
   24,    0,    4,
};
const short yy_policy_parserdgoto[] = {                                       1,
   22,   23,   24,   25,
};
const short yy_policy_parsersindex[] = {                                      0,
 -219,    0,    0,    0,    0,    0, -279,    0,    0,    0,
    0,    0,    0, -182,   24,   24, -182, -252, -273, -272,
 -261,    0,    0, -214,   88,   24,    0,    0,   24, -258,
   -9,   59,   72,    0,    0,    0, -245, -182, -182, -182,
    0,   24,   24,   24,   24,   24,   24,   24,   24,   24,
   24,   24, -186, -223, -258,    0,    0,    0,    0,    0,
    0, -258, -258, -258, -258, -258, -258, -258, -265, -265,
    0, -258,    0,
};
const short yy_policy_parserrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,  104,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,  -77,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,  -43,    0,    0,    0,    0,    0,
    0, -267, -264, -177,   29,   32,   35,   53, -145, -111,
    0,   56,    0,
};
const short yy_policy_parsergindex[] = {                                      0,
    0,    0,  255,  -15,
};
#define YYTABLESIZE 394
const short yy_policy_parsertable[] = {                                      30,
   31,   33,   19,   19,   19,   13,   13,   13,   26,   34,
   53,   35,   36,   54,   37,   55,   58,   19,   51,   19,
   13,    0,   13,   49,   50,   51,   62,   63,   64,   65,
   66,   67,   68,   69,   70,   71,   72,    2,    3,    4,
    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
    0,    0,    0,   15,   16,   38,   39,   40,   49,   50,
   51,    0,    0,   57,    0,    0,   17,    0,    0,   18,
   41,   19,   20,   21,    2,    3,    4,    5,    6,   27,
    8,    9,   10,   11,   12,   13,   14,    0,    0,    0,
   15,   16,   14,   14,   14,   49,   50,   51,   73,    0,
    0,    0,    0,   17,    0,    0,   18,   14,    0,   14,
   21,   22,   22,   22,   22,   22,   22,   22,   22,   22,
   22,   22,   22,    0,   22,   22,   22,   22,   22,   22,
   22,   22,   22,   22,   22,   22,   22,   22,    0,   22,
   22,   22,    0,   22,   22,   23,   23,   23,   23,   23,
   23,   23,   23,   23,   23,   23,   23,    0,   23,   23,
   23,   23,   23,   23,   23,   23,   23,   23,   23,   23,
   23,   23,    0,   23,   23,   23,    0,   23,   23,   25,
   25,   25,   25,   25,   25,   25,   25,   25,   25,   25,
   25,    0,   25,   25,   25,   25,   25,   25,   25,   25,
   25,   25,   25,   25,    0,    0,    0,   25,   25,   25,
    0,   25,   25,   26,   26,   26,   26,   26,   26,   26,
   26,   26,   26,   26,   26,    0,   26,   26,   26,   26,
   26,   26,   26,   26,   26,   26,   26,   26,    0,    0,
    0,   26,   26,   26,    0,   26,   26,    2,    3,    4,
    5,    6,   27,    8,    9,   10,   11,   12,   13,    0,
    0,    0,    0,   15,   16,    0,    0,    0,   28,    0,
    0,   32,   49,   50,   51,    0,   29,    0,    0,   18,
    2,    3,    4,    5,    6,   27,    8,    9,   10,   11,
   12,   13,   59,   60,   61,    0,   15,   16,   17,   17,
   17,   16,   16,   16,   15,   15,   15,    0,    0,   29,
    0,    0,   18,   17,    0,   17,   16,    0,   16,   15,
    0,   15,   18,   18,   18,   21,   21,   21,   38,   39,
   40,    0,    0,    0,    0,    0,    0,   18,    0,   18,
   21,    0,   21,    0,    0,   56,   42,   43,   44,   45,
   46,   47,   48,   49,   50,   51,    0,    0,   57,    0,
    0,   52,   42,   43,   44,   45,   46,   47,   48,   49,
   50,   51,    0,    0,    0,    0,    0,   52,   33,   33,
   33,   33,   33,   33,   33,   33,   33,   33,    0,    0,
    0,    0,    0,   33,
};
const short yy_policy_parsercheck[] = {                                      15,
   16,   17,  270,  271,  272,  270,  271,  272,  288,  262,
   26,  285,  285,   29,  276,   31,  262,  285,  284,  287,
  285,   -1,  287,  282,  283,  284,   42,   43,   44,   45,
   46,   47,   48,   49,   50,   51,   52,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,  267,  268,  269,
   -1,   -1,   -1,  273,  274,  270,  271,  272,  282,  283,
  284,   -1,   -1,  287,   -1,   -1,  286,   -1,   -1,  289,
  285,  291,  292,  293,  257,  258,  259,  260,  261,  262,
  263,  264,  265,  266,  267,  268,  269,   -1,   -1,   -1,
  273,  274,  270,  271,  272,  282,  283,  284,  285,   -1,
   -1,   -1,   -1,  286,   -1,   -1,  289,  285,   -1,  287,
  293,  257,  258,  259,  260,  261,  262,  263,  264,  265,
  266,  267,  268,   -1,  270,  271,  272,  273,  274,  275,
  276,  277,  278,  279,  280,  281,  282,  283,   -1,  285,
  286,  287,   -1,  289,  290,  257,  258,  259,  260,  261,
  262,  263,  264,  265,  266,  267,  268,   -1,  270,  271,
  272,  273,  274,  275,  276,  277,  278,  279,  280,  281,
  282,  283,   -1,  285,  286,  287,   -1,  289,  290,  257,
  258,  259,  260,  261,  262,  263,  264,  265,  266,  267,
  268,   -1,  270,  271,  272,  273,  274,  275,  276,  277,
  278,  279,  280,  281,   -1,   -1,   -1,  285,  286,  287,
   -1,  289,  290,  257,  258,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,   -1,  270,  271,  272,  273,
  274,  275,  276,  277,  278,  279,  280,  281,   -1,   -1,
   -1,  285,  286,  287,   -1,  289,  290,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,  267,  268,   -1,
   -1,   -1,   -1,  273,  274,   -1,   -1,   -1,   14,   -1,
   -1,   17,  282,  283,  284,   -1,  286,   -1,   -1,  289,
  257,  258,  259,  260,  261,  262,  263,  264,  265,  266,
  267,  268,   38,   39,   40,   -1,  273,  274,  270,  271,
  272,  270,  271,  272,  270,  271,  272,   -1,   -1,  286,
   -1,   -1,  289,  285,   -1,  287,  285,   -1,  287,  285,
   -1,  287,  270,  271,  272,  270,  271,  272,  270,  271,
  272,   -1,   -1,   -1,   -1,   -1,   -1,  285,   -1,  287,
  285,   -1,  287,   -1,   -1,  287,  275,  276,  277,  278,
  279,  280,  281,  282,  283,  284,   -1,   -1,  287,   -1,
   -1,  290,  275,  276,  277,  278,  279,  280,  281,  282,
  283,  284,   -1,   -1,   -1,   -1,   -1,  290,  275,  276,
  277,  278,  279,  280,  281,  282,  283,  284,   -1,   -1,
   -1,   -1,   -1,  290,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 293
#if YYDEBUG
const char * const yy_policy_parsername[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"YY_BOOL","YY_INT","YY_UINT",
"YY_UINTRANGE","YY_STR","YY_ID","YY_IPV4","YY_IPV4RANGE","YY_IPV4NET","YY_IPV6",
"YY_IPV6RANGE","YY_IPV6NET","YY_NOT","YY_AND","YY_XOR","YY_OR","YY_HEAD",
"YY_CTR","YY_NE_INT","YY_EQ","YY_NE","YY_LE","YY_GT","YY_LT","YY_GE","YY_ADD",
"YY_SUB","YY_MUL","YY_SEMICOLON","YY_LPAR","YY_RPAR","YY_ASSIGN","YY_SET",
"YY_REGEX","YY_ACCEPT","YY_REJECT","YY_PROTOCOL",
};
const char * const yy_policy_parserrule[] = {
"$accept : statement",
"statement : statement actionstatement",
"statement : statement boolstatement",
"statement :",
"actionstatement : YY_ID YY_ASSIGN expr YY_SEMICOLON",
"actionstatement : YY_ACCEPT YY_SEMICOLON",
"actionstatement : YY_REJECT YY_SEMICOLON",
"boolstatement : boolexpr YY_SEMICOLON",
"boolexpr : YY_PROTOCOL YY_EQ YY_ID",
"boolexpr : YY_NOT boolexpr",
"boolexpr : boolexpr YY_AND boolexpr",
"boolexpr : boolexpr YY_XOR boolexpr",
"boolexpr : boolexpr YY_OR boolexpr",
"boolexpr : expr YY_EQ expr",
"boolexpr : expr YY_NE expr",
"boolexpr : expr YY_LT expr",
"boolexpr : expr YY_GT expr",
"boolexpr : expr YY_LE expr",
"boolexpr : expr YY_GE expr",
"boolexpr : expr YY_NE_INT expr",
"boolexpr : YY_LPAR boolexpr YY_RPAR",
"boolexpr : expr YY_REGEX expr",
"expr : expr YY_ADD expr",
"expr : expr YY_SUB expr",
"expr : expr YY_MUL expr",
"expr : YY_HEAD expr",
"expr : YY_CTR expr expr",
"expr : YY_LPAR expr YY_RPAR",
"expr : YY_STR",
"expr : YY_UINT",
"expr : YY_UINTRANGE",
"expr : YY_INT",
"expr : YY_BOOL",
"expr : YY_ID",
"expr : YY_SET YY_ID",
"expr : YY_IPV4",
"expr : YY_IPV4RANGE",
"expr : YY_IPV6",
"expr : YY_IPV6RANGE",
"expr : YY_IPV4NET",
"expr : YY_IPV6NET",
};
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
case 1:
#line 63 "policy.y"
{ _parser_nodes->push_back(yyvsp[0].node); }
break;
case 2:
#line 64 "policy.y"
{ _parser_nodes->push_back(yyvsp[0].node); }
break;
case 4:
#line 70 "policy.y"
{ yyval.node = new NodeAssign(yyvsp[-3].c_str,yyvsp[-1].node,_parser_lineno); free(yyvsp[-3].c_str); }
break;
case 5:
#line 71 "policy.y"
{ yyval.node = new NodeAccept(_parser_lineno); }
break;
case 6:
#line 72 "policy.y"
{ yyval.node = new NodeReject(_parser_lineno); }
break;
case 7:
#line 76 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 8:
#line 81 "policy.y"
{ yyval.node = new NodeProto(yyvsp[0].c_str,_parser_lineno); free(yyvsp[0].c_str); }
break;
case 9:
#line 82 "policy.y"
{ yyval.node = new NodeUn(new OpNot,yyvsp[0].node,_parser_lineno); }
break;
case 10:
#line 83 "policy.y"
{ yyval.node = new NodeBin(new OpAnd,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 11:
#line 84 "policy.y"
{ yyval.node = new NodeBin(new OpXor,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 12:
#line 85 "policy.y"
{ yyval.node = new NodeBin(new OpOr,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 13:
#line 87 "policy.y"
{ yyval.node = new NodeBin(new OpEq,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 14:
#line 88 "policy.y"
{ yyval.node = new NodeBin(new OpNe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 15:
#line 90 "policy.y"
{ yyval.node = new NodeBin(new OpLt,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 16:
#line 91 "policy.y"
{ yyval.node = new NodeBin(new OpGt,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 17:
#line 92 "policy.y"
{ yyval.node = new NodeBin(new OpLe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 18:
#line 93 "policy.y"
{ yyval.node = new NodeBin(new OpGe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 19:
#line 94 "policy.y"
{ yyval.node = new NodeBin(new OpNEInt, yyvsp[-2].node, yyvsp[0].node, _parser_lineno); }
break;
case 20:
#line 96 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 21:
#line 98 "policy.y"
{ yyval.node = new NodeBin(new OpRegex, yyvsp[-2].node, yyvsp[0].node, _parser_lineno); }
break;
case 22:
#line 102 "policy.y"
{ yyval.node = new NodeBin(new OpAdd,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 23:
#line 103 "policy.y"
{ yyval.node = new NodeBin(new OpSub,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 24:
#line 104 "policy.y"
{ yyval.node = new NodeBin(new OpMul,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 25:
#line 106 "policy.y"
{ yyval.node = new NodeUn(new OpHead, yyvsp[0].node, _parser_lineno); }
break;
case 26:
#line 107 "policy.y"
{ yyval.node = new NodeBin(new OpCtr, yyvsp[-1].node, yyvsp[0].node, _parser_lineno); }
break;
case 27:
#line 109 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 28:
#line 111 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemStr::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 29:
#line 112 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemU32::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 30:
#line 113 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemU32Range::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 31:
#line 114 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemInt32::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 32:
#line 115 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemBool::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 33:
#line 116 "policy.y"
{ yyval.node = new NodeVar(yyvsp[0].c_str,_parser_lineno); free(yyvsp[0].c_str); }
break;
case 34:
#line 117 "policy.y"
{ yyval.node = new NodeSet(yyvsp[0].c_str,_parser_lineno); free(yyvsp[0].c_str); }
break;
case 35:
#line 118 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv4::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 36:
#line 119 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv4Range::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 37:
#line 120 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv6::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 38:
#line 121 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv6Range::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 39:
#line 122 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv4Net::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 40:
#line 123 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv6Net::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
#line 699 "yacc.yy_policy_parser.cc"
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
