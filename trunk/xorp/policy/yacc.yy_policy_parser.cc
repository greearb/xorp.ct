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

#include <vector>

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "policy/common/element.hh"
#include "policy/common/element_factory.hh"
#include "policy/common/operator.hh"
#include "policy_parser.hh"

extern int  yylex(void);
extern void yyerror(const char *m);

using namespace policy_parser;

static ElementFactory _ef;

#line 32 "policy.y"
typedef union {
	char*		c_str;
	Node*		node;
	BinOper*	op;
} YYSTYPE;
#line 85 "yacc.yy_policy_parser.cc"
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
#define YY_SEMICOLON 269
#define YY_LPAR 270
#define YY_RPAR 271
#define YY_ASSIGN 272
#define YY_SET 273
#define YY_REGEX 274
#define YY_ACCEPT 275
#define YY_REJECT 276
#define YY_PROTOCOL 277
#define YY_NEXT 278
#define YY_POLICY 279
#define YY_PLUS_EQUALS 280
#define YY_MINUS_EQUALS 281
#define YY_TERM 282
#define YY_NOT 283
#define YY_AND 284
#define YY_XOR 285
#define YY_OR 286
#define YY_HEAD 287
#define YY_CTR 288
#define YY_NE_INT 289
#define YY_EQ 290
#define YY_NE 291
#define YY_LE 292
#define YY_GT 293
#define YY_LT 294
#define YY_GE 295
#define YY_IPNET_EQ 296
#define YY_IPNET_LE 297
#define YY_IPNET_GT 298
#define YY_IPNET_LT 299
#define YY_IPNET_GE 300
#define YY_ADD 301
#define YY_SUB 302
#define YY_MUL 303
const short yy_policy_parserlhs[] = {                                        -1,
    0,    0,    0,    1,    2,    2,    2,    2,    2,    6,
    7,    7,    7,    3,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    4,    4,    4,    4,    4,
    4,    4,    4,    4,    4,    5,    5,    5,    5,    5,
    5,    5,    5,    5,    5,    5,    5,    5,    5,    5,
    5,    5,    5,    5,
};
const short yy_policy_parserlen[] = {                                         2,
    2,    2,    0,    2,    1,    1,    1,    2,    2,    3,
    1,    1,    1,    2,    3,    2,    2,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    2,    3,
    3,    1,    1,    1,    1,    1,    1,    2,    1,    1,
    1,    1,    1,    1,
};
const short yy_policy_parserdefred[] = {                                      3,
    0,   46,   45,   43,   44,   42,    0,   49,   50,   53,
   51,   52,   54,    0,    0,    6,    7,    0,    0,    0,
    0,    0,    0,    1,    0,    2,    0,    0,    5,   11,
   12,   13,    0,   47,    0,    0,   48,    0,    8,    9,
   17,   16,    0,    0,    0,    4,   14,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   34,   41,
   15,    0,    0,   18,   19,   20,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   38,
};
const short yy_policy_parserdgoto[] = {                                       1,
   24,   25,   26,   27,   28,   29,   33,
};
const short yy_policy_parsersindex[] = {                                      0,
   57,    0,    0,    0,    0,    0, -266,    0,    0,    0,
    0,    0,    0,   89, -258,    0,    0, -279, -213, -249,
   89,  121,  121,    0, -246,    0,  226,  122,    0,    0,
    0,    0,  121,    0,   11,   10,    0, -238,    0,    0,
    0,    0,  121, -178, -203,    0,    0,   89,   89,   89,
  121,  121,  121,  121,  121,  121,  121,  121,  121,  121,
  121,  121,  121,  121,  121,  121,  121, -178,    0,    0,
    0, -269, -178,    0,    0,    0, -178, -178, -178, -178,
 -178, -178, -178, -178, -178, -178, -178, -178, -178, -178,
 -275, -275,    0,
};
const short yy_policy_parserrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,  152,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,  -64,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0, -239,    0,    0,
    0,    0,  -20,    0,    0,    0, -268, -264, -259, -197,
 -194, -190,  187,  190,  193,  196,  199,  217,  220,  223,
 -156, -110,    0,
};
const short yy_policy_parsergindex[] = {                                      0,
    0,    0,    0,   72,  -14,    0,    0,
};
#define YYTABLESIZE 512
const short yy_policy_parsertable[] = {                                      36,
   35,   70,   35,   37,   28,   30,   28,   44,   45,   33,
   38,   33,   41,   31,   32,   35,   35,   35,   68,   28,
   28,   28,   46,   71,   33,   33,   33,   67,   72,   10,
   73,   65,   66,   67,    0,    0,   77,   78,   79,   80,
   81,   82,   83,   84,   85,   86,   87,   88,   89,   90,
   91,   92,   93,    2,    3,    4,    5,    6,   34,    8,
    9,   10,   11,   12,   13,   39,   43,    0,   40,   15,
    0,   21,    0,   21,   22,    0,   22,    0,   25,    0,
   25,    0,    0,   22,   23,   35,   21,   21,   21,   22,
   22,   22,   42,   25,   25,   25,    0,   65,   66,   67,
   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,
   36,   36,   36,   36,   36,    0,   36,   36,    0,   74,
   75,   76,   65,   66,   67,    0,   36,   36,   36,   36,
   36,   36,   36,   36,   36,   36,   36,   36,   36,   36,
   36,   36,   36,   36,   36,   36,   37,   37,   37,   37,
   37,   37,   37,   37,   37,   37,   37,   37,   37,   37,
   37,    0,   37,   37,    0,    0,    0,    0,    0,    0,
    0,    0,   37,   37,   37,   37,   37,   37,   37,   37,
   37,   37,   37,   37,   37,   37,   37,   37,   37,   37,
   37,   37,   39,   39,   39,   39,   39,   39,   39,   39,
   39,   39,   39,   39,   39,   39,   39,    0,   39,   39,
    0,    0,    0,    0,    0,    0,    0,    0,   39,   39,
   39,   39,   39,   39,   39,   39,   39,   39,   39,   39,
   39,   39,   39,   39,   39,   39,   40,   40,   40,   40,
   40,   40,   40,   40,   40,   40,   40,   40,   40,   40,
   40,    0,   40,   40,    0,    0,    0,    0,    0,    0,
    0,    0,   40,   40,   40,   40,   40,   40,   40,   40,
   40,   40,   40,   40,   40,   40,   40,   40,   40,   40,
   70,   69,    0,   51,    0,    0,    0,    0,    0,    0,
    0,    0,   52,    0,   48,   49,   50,    0,   53,   54,
   55,   56,   57,   58,   59,   60,   61,   62,   63,   64,
   65,   66,   67,    2,    3,    4,    5,    6,    7,    8,
    9,   10,   11,   12,   13,    0,   14,    0,    0,   15,
    0,   16,   17,   18,   19,   20,    0,    0,    0,   21,
    0,    0,    0,   22,   23,    2,    3,    4,    5,    6,
   34,    8,    9,   10,   11,   12,   13,    0,   14,    0,
    0,   15,    0,    0,    0,   18,    0,   20,    0,    0,
    0,   21,    0,    0,    0,   22,   23,    2,    3,    4,
    5,    6,   34,    8,    9,   10,   11,   12,   13,    0,
   43,    0,    0,   15,    0,   51,    0,    0,    0,    0,
    0,    0,    0,    0,   52,    0,    0,   22,   23,    0,
   53,   54,   55,   56,   57,   58,   59,   60,   61,   62,
   63,   64,   65,   66,   67,   47,    0,    0,    0,    0,
    0,    0,    0,    0,   47,    0,    0,    0,    0,    0,
   47,   47,   47,   47,   47,   47,   47,   47,   47,   47,
   47,   47,   47,   47,   47,   24,    0,   24,   23,    0,
   23,   26,    0,   26,   27,    0,   27,   31,    0,   31,
   24,   24,   24,   23,   23,   23,   26,   26,   26,   27,
   27,   27,   31,   31,   31,   30,    0,   30,   29,    0,
   29,   32,    0,   32,   47,    0,    0,    0,    0,    0,
   30,   30,   30,   29,   29,   29,   32,   32,   32,   48,
   49,   50,
};
const short yy_policy_parsercheck[] = {                                      14,
  269,  271,  271,  262,  269,  272,  271,   22,   23,  269,
  290,  271,  262,  280,  281,  284,  285,  286,   33,  284,
  285,  286,  269,  262,  284,  285,  286,  303,   43,  269,
   45,  301,  302,  303,   -1,   -1,   51,   52,   53,   54,
   55,   56,   57,   58,   59,   60,   61,   62,   63,   64,
   65,   66,   67,  257,  258,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,  279,  270,   -1,  282,  273,
   -1,  269,   -1,  271,  269,   -1,  271,   -1,  269,   -1,
  271,   -1,   -1,  287,  288,   14,  284,  285,  286,  284,
  285,  286,   21,  284,  285,  286,   -1,  301,  302,  303,
  257,  258,  259,  260,  261,  262,  263,  264,  265,  266,
  267,  268,  269,  270,  271,   -1,  273,  274,   -1,   48,
   49,   50,  301,  302,  303,   -1,  283,  284,  285,  286,
  287,  288,  289,  290,  291,  292,  293,  294,  295,  296,
  297,  298,  299,  300,  301,  302,  257,  258,  259,  260,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
  271,   -1,  273,  274,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  298,  299,  300,
  301,  302,  257,  258,  259,  260,  261,  262,  263,  264,
  265,  266,  267,  268,  269,  270,  271,   -1,  273,  274,
   -1,   -1,   -1,   -1,   -1,   -1,   -1,   -1,  283,  284,
  285,  286,  287,  288,  289,  290,  291,  292,  293,  294,
  295,  296,  297,  298,  299,  300,  257,  258,  259,  260,
  261,  262,  263,  264,  265,  266,  267,  268,  269,  270,
  271,   -1,  273,  274,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  283,  284,  285,  286,  287,  288,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  298,  299,  300,
  271,  271,   -1,  274,   -1,   -1,   -1,   -1,   -1,   -1,
   -1,   -1,  283,   -1,  284,  285,  286,   -1,  289,  290,
  291,  292,  293,  294,  295,  296,  297,  298,  299,  300,
  301,  302,  303,  257,  258,  259,  260,  261,  262,  263,
  264,  265,  266,  267,  268,   -1,  270,   -1,   -1,  273,
   -1,  275,  276,  277,  278,  279,   -1,   -1,   -1,  283,
   -1,   -1,   -1,  287,  288,  257,  258,  259,  260,  261,
  262,  263,  264,  265,  266,  267,  268,   -1,  270,   -1,
   -1,  273,   -1,   -1,   -1,  277,   -1,  279,   -1,   -1,
   -1,  283,   -1,   -1,   -1,  287,  288,  257,  258,  259,
  260,  261,  262,  263,  264,  265,  266,  267,  268,   -1,
  270,   -1,   -1,  273,   -1,  274,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  283,   -1,   -1,  287,  288,   -1,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  299,  300,  301,  302,  303,  274,   -1,   -1,   -1,   -1,
   -1,   -1,   -1,   -1,  283,   -1,   -1,   -1,   -1,   -1,
  289,  290,  291,  292,  293,  294,  295,  296,  297,  298,
  299,  300,  301,  302,  303,  269,   -1,  271,  269,   -1,
  271,  269,   -1,  271,  269,   -1,  271,  269,   -1,  271,
  284,  285,  286,  284,  285,  286,  284,  285,  286,  284,
  285,  286,  284,  285,  286,  269,   -1,  271,  269,   -1,
  271,  269,   -1,  271,  269,   -1,   -1,   -1,   -1,   -1,
  284,  285,  286,  284,  285,  286,  284,  285,  286,  284,
  285,  286,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 303
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
"YY_IPV6RANGE","YY_IPV6NET","YY_SEMICOLON","YY_LPAR","YY_RPAR","YY_ASSIGN",
"YY_SET","YY_REGEX","YY_ACCEPT","YY_REJECT","YY_PROTOCOL","YY_NEXT","YY_POLICY",
"YY_PLUS_EQUALS","YY_MINUS_EQUALS","YY_TERM","YY_NOT","YY_AND","YY_XOR","YY_OR",
"YY_HEAD","YY_CTR","YY_NE_INT","YY_EQ","YY_NE","YY_LE","YY_GT","YY_LT","YY_GE",
"YY_IPNET_EQ","YY_IPNET_LE","YY_IPNET_GT","YY_IPNET_LT","YY_IPNET_GE","YY_ADD",
"YY_SUB","YY_MUL",
};
const char * const yy_policy_parserrule[] = {
"$accept : statement",
"statement : statement actionstatement",
"statement : statement boolstatement",
"statement :",
"actionstatement : action YY_SEMICOLON",
"action : assignexpr",
"action : YY_ACCEPT",
"action : YY_REJECT",
"action : YY_NEXT YY_POLICY",
"action : YY_NEXT YY_TERM",
"assignexpr : YY_ID assignop expr",
"assignop : YY_ASSIGN",
"assignop : YY_PLUS_EQUALS",
"assignop : YY_MINUS_EQUALS",
"boolstatement : boolexpr YY_SEMICOLON",
"boolexpr : YY_PROTOCOL YY_EQ YY_ID",
"boolexpr : YY_NOT boolexpr",
"boolexpr : YY_POLICY YY_ID",
"boolexpr : boolexpr YY_AND boolexpr",
"boolexpr : boolexpr YY_XOR boolexpr",
"boolexpr : boolexpr YY_OR boolexpr",
"boolexpr : expr YY_EQ expr",
"boolexpr : expr YY_NE expr",
"boolexpr : expr YY_LT expr",
"boolexpr : expr YY_GT expr",
"boolexpr : expr YY_LE expr",
"boolexpr : expr YY_GE expr",
"boolexpr : expr YY_IPNET_EQ expr",
"boolexpr : expr YY_NOT expr",
"boolexpr : expr YY_IPNET_LT expr",
"boolexpr : expr YY_IPNET_GT expr",
"boolexpr : expr YY_IPNET_LE expr",
"boolexpr : expr YY_IPNET_GE expr",
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
case 1:
#line 55 "policy.y"
{ _parser_nodes->push_back(yyvsp[0].node); }
break;
case 2:
#line 56 "policy.y"
{ _parser_nodes->push_back(yyvsp[0].node); }
break;
case 4:
#line 61 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 6:
#line 66 "policy.y"
{ yyval.node = new NodeAccept(_parser_lineno); }
break;
case 7:
#line 67 "policy.y"
{ yyval.node = new NodeReject(_parser_lineno); }
break;
case 8:
#line 69 "policy.y"
{ yyval.node = new NodeNext(_parser_lineno, NodeNext::POLICY); }
break;
case 9:
#line 71 "policy.y"
{ yyval.node = new NodeNext(_parser_lineno, NodeNext::TERM); }
break;
case 10:
#line 76 "policy.y"
{ yyval.node = new NodeAssign(yyvsp[-2].c_str, yyvsp[-1].op, yyvsp[0].node, _parser_lineno); free(yyvsp[-2].c_str); }
break;
case 11:
#line 80 "policy.y"
{ yyval.op = NULL; }
break;
case 12:
#line 81 "policy.y"
{ yyval.op = new OpAdd; }
break;
case 13:
#line 82 "policy.y"
{ yyval.op = new OpSub; }
break;
case 14:
#line 86 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 15:
#line 90 "policy.y"
{ yyval.node = new NodeProto(yyvsp[0].c_str,_parser_lineno); free(yyvsp[0].c_str); }
break;
case 16:
#line 91 "policy.y"
{ yyval.node = new NodeUn(new OpNot,yyvsp[0].node,_parser_lineno); }
break;
case 17:
#line 92 "policy.y"
{ yyval.node = new NodeSubr(_parser_lineno, yyvsp[0].c_str); free(yyvsp[0].c_str); }
break;
case 18:
#line 93 "policy.y"
{ yyval.node = new NodeBin(new OpAnd,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 19:
#line 94 "policy.y"
{ yyval.node = new NodeBin(new OpXor,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 20:
#line 95 "policy.y"
{ yyval.node = new NodeBin(new OpOr,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 21:
#line 97 "policy.y"
{ yyval.node = new NodeBin(new OpEq,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 22:
#line 98 "policy.y"
{ yyval.node = new NodeBin(new OpNe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 23:
#line 100 "policy.y"
{ yyval.node = new NodeBin(new OpLt,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 24:
#line 101 "policy.y"
{ yyval.node = new NodeBin(new OpGt,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 25:
#line 102 "policy.y"
{ yyval.node = new NodeBin(new OpLe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 26:
#line 103 "policy.y"
{ yyval.node = new NodeBin(new OpGe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 27:
#line 105 "policy.y"
{ yyval.node = new NodeBin(new OpEq,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 28:
#line 106 "policy.y"
{ yyval.node = new NodeBin(new OpNe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 29:
#line 107 "policy.y"
{ yyval.node = new NodeBin(new OpLt,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 30:
#line 108 "policy.y"
{ yyval.node = new NodeBin(new OpGt,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 31:
#line 109 "policy.y"
{ yyval.node = new NodeBin(new OpLe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 32:
#line 110 "policy.y"
{ yyval.node = new NodeBin(new OpGe,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 33:
#line 112 "policy.y"
{ yyval.node = new NodeBin(new OpNEInt, yyvsp[-2].node, yyvsp[0].node, _parser_lineno); }
break;
case 34:
#line 114 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 35:
#line 116 "policy.y"
{ yyval.node = new NodeBin(new OpRegex, yyvsp[-2].node, yyvsp[0].node, _parser_lineno); }
break;
case 36:
#line 120 "policy.y"
{ yyval.node = new NodeBin(new OpAdd,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 37:
#line 121 "policy.y"
{ yyval.node = new NodeBin(new OpSub,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 38:
#line 122 "policy.y"
{ yyval.node = new NodeBin(new OpMul,yyvsp[-2].node,yyvsp[0].node,_parser_lineno); }
break;
case 39:
#line 124 "policy.y"
{ yyval.node = new NodeUn(new OpHead, yyvsp[0].node, _parser_lineno); }
break;
case 40:
#line 125 "policy.y"
{ yyval.node = new NodeBin(new OpCtr, yyvsp[-1].node, yyvsp[0].node, _parser_lineno); }
break;
case 41:
#line 127 "policy.y"
{ yyval.node = yyvsp[-1].node; }
break;
case 42:
#line 129 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemStr::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 43:
#line 130 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemU32::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 44:
#line 131 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemU32Range::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 45:
#line 132 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemInt32::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 46:
#line 133 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemBool::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str);}
break;
case 47:
#line 134 "policy.y"
{ yyval.node = new NodeVar(yyvsp[0].c_str,_parser_lineno); free(yyvsp[0].c_str); }
break;
case 48:
#line 135 "policy.y"
{ yyval.node = new NodeSet(yyvsp[0].c_str,_parser_lineno); free(yyvsp[0].c_str); }
break;
case 49:
#line 136 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv4::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 50:
#line 137 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv4Range::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 51:
#line 138 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv6::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 52:
#line 139 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv6Range::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 53:
#line 140 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv4Net::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
case 54:
#line 141 "policy.y"
{ yyval.node = new NodeElem(_ef.create(ElemIPv6Net::id,yyvsp[0].c_str),_parser_lineno); free(yyvsp[0].c_str); }
break;
#line 810 "yacc.yy_policy_parser.cc"
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
