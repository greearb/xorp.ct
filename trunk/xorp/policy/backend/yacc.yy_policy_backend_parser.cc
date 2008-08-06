#include <stdlib.h>
#ifndef lint
#ifdef __unused
__unused
#endif
static char const 
yyrcsid[] = "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.37 2003/02/12 18:03:55 davidc Exp $";
#endif
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

#include "libxorp/xorp.h"

#include <vector>

#include "policy/common/varrw.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"

#include "policy_backend_parser.hh"
#include "instruction.hh"
#include "term_instr.hh"
#include "policy_instr.hh"


extern int yylex(void);
extern void yyerror(const char*);

using namespace policy_backend_parser;

static ElementFactory _ef;

#line 29 "backend.y"
typedef union {
	char* c_str;
} YYSTYPE;
#line 80 "yacc.yy_policy_backend_parser.cc"
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
#define YY_HEAD 276
#define YY_CTR 277
#define YY_NE_INT 278
#define YY_ADD 279
#define YY_SUB 280
#define YY_MUL 281
#define YY_ONFALSE_EXIT 282
#define YY_REGEX 283
#define YY_LOAD 284
#define YY_STORE 285
#define YY_ACCEPT 286
#define YY_REJECT 287
#define YY_SET 288
const short yy_policy_backend_parserlhs[] = {                                        -1,
    0,    0,    0,    2,    1,    3,    3,    4,    4,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,
};
const short yy_policy_backend_parserlen[] = {                                         2,
    2,    2,    0,    5,    6,    7,    0,    3,    0,    3,
    2,    1,    2,    2,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,
};
const short yy_policy_backend_parserdefred[] = {                                      3,
    0,    0,    0,    1,    2,    0,    0,    7,    0,    0,
    0,    0,    0,    4,    5,    0,    9,    0,    0,    0,
    0,   17,   18,   19,   20,   21,   22,   23,   24,   26,
   25,   30,   31,   32,   27,   28,   29,   12,   33,    0,
    0,   15,   16,    0,    6,    0,   11,   13,   14,    8,
   10,
};
const short yy_policy_backend_parserdgoto[] = {                                       1,
    4,    5,   10,   18,   44,
};
const short yy_policy_backend_parsersindex[] = {                                      0,
 -260, -228, -227,    0,    0, -226, -224,    0, -223, -235,
 -222, -221, -219,    0,    0, -218,    0, -262, -217, -215,
 -214,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -213,
 -212,    0,    0, -211,    0, -209,    0,    0,    0,    0,
    0,
};
const short yy_policy_backend_parserrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,
};
const short yy_policy_backend_parsergindex[] = {                                      0,
    0,    0,    0,    0,    0,
};
#define YYTABLESIZE 48
const short yy_policy_backend_parsertable[] = {                                       2,
   19,   20,   21,   22,   23,   24,   25,   26,   27,   28,
   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,
   39,   40,   41,   42,   43,   12,   13,    3,    6,    7,
    0,    8,    9,   11,    0,   14,   15,   16,    0,   17,
   45,   46,   47,   48,   49,    0,   50,   51,
};
const short yy_policy_backend_parsercheck[] = {                                     260,
  263,  264,  265,  266,  267,  268,  269,  270,  271,  272,
  273,  274,  275,  276,  277,  278,  279,  280,  281,  282,
  283,  284,  285,  286,  287,  261,  262,  288,  257,  257,
   -1,  258,  257,  257,   -1,  258,  258,  257,   -1,  258,
  258,  257,  257,  257,  257,   -1,  258,  257,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 288
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
"YY_OR","YY_XOR","YY_HEAD","YY_CTR","YY_NE_INT","YY_ADD","YY_SUB","YY_MUL",
"YY_ONFALSE_EXIT","YY_REGEX","YY_LOAD","YY_STORE","YY_ACCEPT","YY_REJECT",
"YY_SET",
};
const char * const yy_policy_backend_parserrule[] = {
"$accept : program",
"program : program policy",
"program : program set",
"program :",
"set : YY_SET YY_ARG YY_ARG YY_ARG YY_NEWLINE",
"policy : YY_POLICY_START YY_ARG YY_NEWLINE terms YY_POLICY_END YY_NEWLINE",
"terms : terms YY_TERM_START YY_ARG YY_NEWLINE statements YY_TERM_END YY_NEWLINE",
"terms :",
"statements : statements statement YY_NEWLINE",
"statements :",
"statement : YY_PUSH YY_ARG YY_ARG",
"statement : YY_PUSH_SET YY_ARG",
"statement : YY_ONFALSE_EXIT",
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
"statement : YY_HEAD",
"statement : YY_CTR",
"statement : YY_NE_INT",
"statement : YY_REGEX",
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
#line 55 "backend.y"
{
	  	/* XXX: doesn't delete old*/
		(*_yy_sets)[yyvsp[-2].c_str] = _ef.create(yyvsp[-3].c_str, yyvsp[-1].c_str);
		free(yyvsp[-3].c_str); free(yyvsp[-2].c_str); free(yyvsp[-1].c_str);
	  }
break;
case 5:
#line 62 "backend.y"
{
			PolicyInstr* pi = new PolicyInstr(yyvsp[-4].c_str,_yy_terms);

			pi->set_trace(_yy_trace);
			_yy_trace = false;

			_yy_terms = new vector<TermInstr*>();
			_yy_policies->push_back(pi);

			free(yyvsp[-4].c_str);
			}
break;
case 6:
#line 76 "backend.y"
{
	  
			TermInstr* ti = new TermInstr(yyvsp[-4].c_str,_yy_instructions);
			_yy_instructions = new vector<Instruction*>();
			_yy_terms->push_back(ti);
			free(yyvsp[-4].c_str);
			}
break;
case 10:
#line 93 "backend.y"
{
	  			Instruction* i = new Push(_ef.create(yyvsp[-1].c_str,yyvsp[0].c_str));
				_yy_instructions->push_back(i);
	  			free(yyvsp[-1].c_str); free(yyvsp[0].c_str);
	  			}
break;
case 11:
#line 98 "backend.y"
{
				_yy_instructions->push_back(new PushSet(yyvsp[0].c_str));
				free(yyvsp[0].c_str);
				}
break;
case 12:
#line 103 "backend.y"
{
				_yy_instructions->push_back(new OnFalseExit());
				}
break;
case 13:
#line 107 "backend.y"
{
				char* err = NULL;
				bool is_error = false;
				VarRW::Id id = strtoul(yyvsp[0].c_str, &err, 10);
				if ((err != NULL) && (*err != '\0'))
				    is_error = true;
				free(yyvsp[0].c_str);
				if (is_error) {
					yyerror("Need numeric var ID");
					YYERROR;
				}
				_yy_instructions->push_back(new Load(id));
				}
break;
case 14:
#line 121 "backend.y"
{
				char* err = NULL;
				bool is_error = false;
				VarRW::Id id = strtoul(yyvsp[0].c_str, &err, 10);

				if ((err != NULL) && (*err != '\0'))
				    is_error = true;

				free(yyvsp[0].c_str);

				if (is_error) {
					yyerror("Need numeric var ID");
					YYERROR;
				}

				if (id == VarRW::VAR_TRACE)
					_yy_trace = true;

				_yy_instructions->push_back(new Store(id));
				}
break;
case 15:
#line 142 "backend.y"
{ _yy_instructions->push_back(new Accept()); }
break;
case 16:
#line 143 "backend.y"
{ _yy_instructions->push_back(new Reject()); }
break;
case 17:
#line 145 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpEq)); }
break;
case 18:
#line 146 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpNe)); }
break;
case 19:
#line 147 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpLt)); }
break;
case 20:
#line 148 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpGt)); }
break;
case 21:
#line 149 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpLe)); }
break;
case 22:
#line 150 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpGe)); }
break;
case 23:
#line 152 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpNot)); }
break;
case 24:
#line 153 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpAnd)); }
break;
case 25:
#line 154 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpXor)); }
break;
case 26:
#line 155 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpOr)); }
break;
case 27:
#line 157 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpAdd)); }
break;
case 28:
#line 158 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpSub)); }
break;
case 29:
#line 159 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpMul)); }
break;
case 30:
#line 160 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpHead));}
break;
case 31:
#line 161 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpCtr));}
break;
case 32:
#line 162 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpNEInt));}
break;
case 33:
#line 163 "backend.y"
{ _yy_instructions->push_back(new NaryInstr(new OpRegex));}
break;
#line 617 "yacc.yy_policy_backend_parser.cc"
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
