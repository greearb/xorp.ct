#ifndef lint
#ifdef __unused
__unused
#endif
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.37 2003/02/12 18:03:55 davidc Exp $";
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
#define yyparse yy_policy_backend_parserparse
#define yylex yy_policy_backend_parserlex
#define yyerror yy_policy_backend_parsererror
#define yychar yy_policy_backend_parserchar
#define yyval yy_policy_backend_parserval
#define yylval yy_policy_backend_parserlval
#define yydebug yy_policy_backend_parserdebug
#define yynerrs yy_policy_backend_parsernerrs
#define yyerrflag yy_policy_backend_parsererrflag
#define yyss yy_policy_backend_parserss
#define yyssp yy_policy_backend_parserssp
#define yyvs yy_policy_backend_parservs
#define yyvsp yy_policy_backend_parservsp
#define yylhs yy_policy_backend_parserlhs
#define yylen yy_policy_backend_parserlen
#define yydefred yy_policy_backend_parserdefred
#define yydgoto yy_policy_backend_parserdgoto
#define yysindex yy_policy_backend_parsersindex
#define yyrindex yy_policy_backend_parserrindex
#define yygindex yy_policy_backend_parsergindex
#define yytable yy_policy_backend_parsertable
#define yycheck yy_policy_backend_parsercheck
#define yyname yy_policy_backend_parsername
#define yyrule yy_policy_backend_parserrule
#define yysslim yy_policy_backend_parsersslim
#define yystacksize yy_policy_backend_parserstacksize
#define YYPREFIX "yy_policy_backend_parser"
#line 2 "backend.y"
/*
 * yacc -d -p yy_policy_backend_parser -o yacc.yy_policy_backend_parser.cc backend.y
 */

#include "config.h"
#include "policy_backend_parser.hh"

#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"

#include "instruction.hh"
#include "term_instr.hh"
#include "policy_instr.hh"

#include <vector>

extern int yylex(void);
extern void yyerror(const char*);

using namespace policy_backend_parser;

static ElementFactory _ef;

#line 27 "backend.y"
typedef union {
	char* c_str;
} YYSTYPE;
#line 78 "yacc.yy_policy_backend_parser.cc"
#define YYERRCODE 256
#define YY_ARG 257
#define YY_NEWLINE 258
#define YY_BLANK 259
#define YY_POLICY_START 260
#define YY_POLICY_END 261
#define YY_TERM_START 262
#define YY_TERM_END 263
#define YY_PUSH 264
#define YY_PUSH_SET 265
#define YY_EQ 266
#define YY_NE 267
#define YY_LT 268
#define YY_GT 269
#define YY_LE 270
#define YY_GE 271
#define YY_NOT 272
#define YY_AND 273
#define YY_OR 274
#define YY_XOR 275
#define YY_ADD 276
#define YY_SUB 277
#define YY_MUL 278
#define YY_ONFALSE_EXIT 279
#define YY_REGEX 280
#define YY_LOAD 281
#define YY_STORE 282
#define YY_ACCEPT 283
#define YY_REJECT 284
#define YY_SET 285
const short yy_policy_backend_parserlhs[] = {                                        -1,
    0,    0,    0,    2,    1,    3,    3,    4,    4,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
};
const short yy_policy_backend_parserlen[] = {                                         2,
    2,    2,    0,    4,    6,    7,    0,    3,    0,    3,
    2,    1,    2,    2,    2,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
};
const short yy_policy_backend_parserdefred[] = {                                      3,
    0,    0,    0,    1,    2,    0,    0,    7,    0,    0,
    4,    0,    0,    5,    0,    9,    0,    0,    0,    0,
   18,   19,   20,   21,   22,   23,   24,   25,   27,   26,
   28,   29,   30,   12,    0,    0,    0,   16,   17,    0,
    6,    0,   11,   13,   14,   15,    8,   10,
};
const short yy_policy_backend_parserdgoto[] = {                                       1,
    4,    5,   10,   17,   40,
};
const short yy_policy_backend_parsersindex[] = {                                      0,
 -260, -231, -230,    0,    0, -229, -227,    0, -226, -238,
    0, -225, -223,    0, -222,    0, -262, -221, -219, -218,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0, -217, -216, -215,    0,    0, -214,
    0, -212,    0,    0,    0,    0,    0,    0,
};
const short yy_policy_backend_parserrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,
};
const short yy_policy_backend_parsergindex[] = {                                      0,
    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 45
const short yy_policy_backend_parsertable[] = {                                       2,
   18,   19,   20,   21,   22,   23,   24,   25,   26,   27,
   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,
   38,   39,   12,   13,    3,    6,    7,    0,    8,    9,
    0,   11,   14,   15,    0,   16,   41,   42,   43,   44,
   45,   46,    0,   47,   48,
};
const short yy_policy_backend_parsercheck[] = {                                     260,
  263,  264,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  275,  276,  277,  278,  279,  280,  281,  282,
  283,  284,  261,  262,  285,  257,  257,   -1,  258,  257,
   -1,  258,  258,  257,   -1,  258,  258,  257,  257,  257,
  257,  257,   -1,  258,  257,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 285
#if YYDEBUG
const char * const yy_policy_backend_parsername[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"YY_ARG","YY_NEWLINE","YY_BLANK",
"YY_POLICY_START","YY_POLICY_END","YY_TERM_START","YY_TERM_END","YY_PUSH",
"YY_PUSH_SET","YY_EQ","YY_NE","YY_LT","YY_GT","YY_LE","YY_GE","YY_NOT","YY_AND",
"YY_OR","YY_XOR","YY_ADD","YY_SUB","YY_MUL","YY_ONFALSE_EXIT","YY_REGEX",
"YY_LOAD","YY_STORE","YY_ACCEPT","YY_REJECT","YY_SET",
};
const char * const yy_policy_backend_parserrule[] = {
"$accept : program",
"program : program policy",
"program : program set",
"program :",
"set : YY_SET YY_ARG YY_ARG YY_NEWLINE",
"policy : YY_POLICY_START YY_ARG YY_NEWLINE terms YY_POLICY_END YY_NEWLINE",
"terms : terms YY_TERM_START YY_ARG YY_NEWLINE statements YY_TERM_END YY_NEWLINE",
"terms :",
"statements : statements statement YY_NEWLINE",
"statements :",
"statement : YY_PUSH YY_ARG YY_ARG",
"statement : YY_PUSH_SET YY_ARG",
"statement : YY_ONFALSE_EXIT",
"statement : YY_REGEX YY_ARG",
"statement : YY_LOAD YY_ARG",
"statement : YY_STORE YY_ARG",
"statement : YY_ACCEPT",
"statement : YY_REJECT",
"statement : YY_EQ",
"statement : YY_NE",
"statement : YY_LT",
"statement : YY_GT",
"statement : YY_LE",
"statement : YY_GE",
"statement : YY_NOT",
"statement : YY_AND",
"statement : YY_XOR",
"statement : YY_OR",
"statement : YY_ADD",
"statement : YY_SUB",
"statement : YY_MUL",
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
    int yym, yyn, yystate;
#if YYDEBUG
    const char *yys;

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
#line 64 "backend.y"
{
	  	/* XXX: doesn't delete old*/
		(*_yy_sets)[yyvsp[-2].c_str] = _ef.create("set",yyvsp[-1].c_str);
		free(yyvsp[-2].c_str); free(yyvsp[-1].c_str);
	  }
break;
case 5:
#line 71 "backend.y"
{
			PolicyInstr* pi = new PolicyInstr(yyvsp[-4].c_str,_yy_terms);
			_yy_terms = new vector<TermInstr*>();
			_yy_policies->push_back(pi);
			free(yyvsp[-4].c_str);
			}
break;
case 6:
#line 80 "backend.y"
{
	  
			TermInstr* ti = new TermInstr(yyvsp[-4].c_str,_yy_instructions);
			_yy_instructions = new vector<Instruction*>();
			_yy_terms->push_back(ti);
			free(yyvsp[-4].c_str);
			}
break;
case 10:
#line 97 "backend.y"
{
	  			Instruction* i = new Push(_ef.create(yyvsp[-1].c_str,yyvsp[0].c_str));
				_yy_instructions->push_back(i);
	  			free(yyvsp[-1].c_str); free(yyvsp[0].c_str);
	  			}
break;
case 11:
#line 102 "backend.y"
{
				_yy_instructions->push_back(new PushSet(yyvsp[0].c_str));
				free(yyvsp[0].c_str);
				}
break;
case 12:
#line 107 "backend.y"
{
				_yy_instructions->push_back(new OnFalseExit());
				}
break;
case 13:
#line 111 "backend.y"
{
				_yy_instructions->push_back(new Regex(yyvsp[0].c_str));
				free(yyvsp[0].c_str);
				}
break;
case 14:
#line 116 "backend.y"
{
				_yy_instructions->push_back(new Load(yyvsp[0].c_str));
				free(yyvsp[0].c_str);
				}
break;
case 15:
#line 120 "backend.y"
{
				_yy_instructions->push_back(new Store(yyvsp[0].c_str));
				free(yyvsp[0].c_str);
				}
break;
case 16:
#line 125 "backend.y"
{ _yy_instructions->push_back(new Accept()); }
break;
case 17:
#line 126 "backend.y"
{ _yy_instructions->push_back(new Reject()); }
break;
case 18:
#line 128 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpEq)); }
break;
case 19:
#line 129 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpNe)); }
break;
case 20:
#line 130 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpLt)); }
break;
case 21:
#line 131 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpGt)); }
break;
case 22:
#line 132 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpLe)); }
break;
case 23:
#line 133 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpGe)); }
break;
case 24:
#line 135 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpNot)); }
break;
case 25:
#line 136 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpAnd)); }
break;
case 26:
#line 137 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpXor)); }
break;
case 27:
#line 138 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpOr)); }
break;
case 28:
#line 140 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpAdd)); }
break;
case 29:
#line 141 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpSub)); }
break;
case 30:
#line 142 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpMul)); }
break;
#line 564 "yacc.yy_policy_backend_parser.cc"
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
