#include <stdlib.h>
#ifndef lint
#ident "$FreeBSD: src/usr.bin/yacc/skeleton.c,v 1.37 2003/02/12 18:03:55 davidc Exp $"
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
#define yyparse yy_compile_policyparse
#define yylex yy_compile_policylex
#define yyerror yy_compile_policyerror
#define yychar yy_compile_policychar
#define yyval yy_compile_policyval
#define yylval yy_compile_policylval
#define yydebug yy_compile_policydebug
#define yynerrs yy_compile_policynerrs
#define yyerrflag yy_compile_policyerrflag
#define yyss yy_compile_policyss
#define yyssp yy_compile_policyssp
#define yyvs yy_compile_policyvs
#define yyvsp yy_compile_policyvsp
#define yylhs yy_compile_policylhs
#define yylen yy_compile_policylen
#define yydefred yy_compile_policydefred
#define yydgoto yy_compile_policydgoto
#define yysindex yy_compile_policysindex
#define yyrindex yy_compile_policyrindex
#define yygindex yy_compile_policygindex
#define yytable yy_compile_policytable
#define yycheck yy_compile_policycheck
#define yyname yy_compile_policyname
#define yyrule yy_compile_policyrule
#define yysslim yy_compile_policysslim
#define yystacksize yy_compile_policystacksize
#define YYPREFIX "yy_compile_policy"
#line 2 "compilepolicy.y"
/*
 * yacc -d -p yy_compile_policy -o yacc.yy_compile_policy.cc compilepolicy.y
 */

#include "policy/policy_module.h"

#include "libxorp/xorp.h"

#include <list>
#include <vector>

#include "libproto/config_node_id.hh"

#include "policy/configuration.hh"
#include "policy/common/policy_utils.hh"
#include "policy/test/compilepolicy.hh"


extern int yylex(void);
extern void yyerror(const char *);
extern Configuration _yy_configuration;

struct yy_tb {
	string name;
	yy_statements* block[3];
};

static vector<yy_tb*> _yy_terms;
static yy_statements* _yy_statements = NULL;


/* add blocks to configuration, and delete stuff from memory*/
static void
add_blocks(const string& pname, const string& tname, yy_tb& term)
{

	/* source, action, dest*/
	for(int i = 0; i < 3; i++) {
		yy_statements* statements = term.block[i];

		/* empty blocks!*/
		if(statements == 0)
			continue;

		ConfigNodeId order_generator(0, i);
		ConfigNodeId prev_order(ConfigNodeId::ZERO());
		ConfigNodeId order(ConfigNodeId::ZERO());
		for(yy_statements::iterator j = statements->begin();
		    j != statements->end(); ++j) {

		    yy_statement* statement = *j;

		    order = order_generator.generate_unique_node_id();
		    order.set_position(prev_order.unique_node_id());
		    prev_order = order;
		    _yy_configuration.update_term_block(pname, tname, i, order,
		    					*statement);
		    delete statement;
		}
		delete statements;
	}
}

#line 67 "compilepolicy.y"
typedef union {
	char *c_str;
	yy_statements* statements;
} YYSTYPE;
#line 119 "yacc.yy_compile_policy.cc"
#define YYERRCODE 256
#define YY_INT 257
#define YY_STR 258
#define YY_ID 259
#define YY_STATEMENT 260
#define YY_SOURCEBLOCK 261
#define YY_DESTBLOCK 262
#define YY_ACTIONBLOCK 263
#define YY_IPV4 264
#define YY_IPV4NET 265
#define YY_IPV6 266
#define YY_IPV6NET 267
#define YY_SEMICOLON 268
#define YY_LBRACE 269
#define YY_RBRACE 270
#define YY_POLICY_STATEMENT 271
#define YY_TERM 272
#define YY_SOURCE 273
#define YY_DEST 274
#define YY_ACTION 275
#define YY_SET 276
#define YY_EXPORT 277
#define YY_IMPORT 278
const short yy_compile_policylhs[] = {                                        -1,
    0,    0,    0,    0,    0,    5,    4,    6,    6,    1,
    2,    3,    7,    7,
};
const short yy_compile_policylen[] = {                                         2,
    2,    2,    5,    5,    0,    5,    5,    8,    0,    4,
    4,    4,    3,    0,
};
const short yy_compile_policydefred[] = {                                      5,
    0,    0,    0,    0,    0,    1,    2,    0,    0,    0,
    0,    9,    0,    0,    0,    0,    0,    3,    4,    7,
    0,    6,    0,    0,    0,    0,   14,    0,    0,    0,
   14,    0,    0,    0,   10,    0,   14,    8,   13,   11,
    0,   12,
};
const short yy_compile_policydgoto[] = {                                       1,
   26,   29,   33,    6,    7,   16,   30,
};
const short yy_compile_policysindex[] = {                                      0,
 -263, -254, -251, -243, -242,    0,    0, -250, -241, -238,
 -237,    0, -236, -245, -244, -266, -240,    0,    0,    0,
 -234,    0, -239, -247, -235, -233,    0, -232, -248, -260,
    0, -231, -230, -229,    0, -259,    0,    0,    0,    0,
 -258,    0,
};
const short yy_compile_policyrindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
const short yy_compile_policygindex[] = {                                      0,
    0,    0,    0,    0,    0,    0,  -28,
};
#define YYTABLESIZE 41
const short yy_compile_policytable[] = {                                      34,
   34,   34,   36,   20,    8,   21,    9,    2,   41,   35,
   40,   42,    3,    4,    5,   10,   11,   13,   12,   14,
   15,   17,   18,   19,   23,   25,   32,   22,    0,   24,
    0,    0,    0,   27,    0,    0,   31,   37,   39,   38,
   28,
};
const short yy_compile_policycheck[] = {                                     260,
  260,  260,   31,  270,  259,  272,  258,  271,   37,  270,
  270,  270,  276,  277,  278,  259,  259,  259,  269,  258,
  258,  258,  268,  268,  259,  273,  275,  268,   -1,  269,
   -1,   -1,   -1,  269,   -1,   -1,  269,  269,  268,  270,
  274,
};
#define YYFINAL 1
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 278
#if YYDEBUG
const char * const yy_compile_policyname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"YY_INT","YY_STR","YY_ID",
"YY_STATEMENT","YY_SOURCEBLOCK","YY_DESTBLOCK","YY_ACTIONBLOCK","YY_IPV4",
"YY_IPV4NET","YY_IPV6","YY_IPV6NET","YY_SEMICOLON","YY_LBRACE","YY_RBRACE",
"YY_POLICY_STATEMENT","YY_TERM","YY_SOURCE","YY_DEST","YY_ACTION","YY_SET",
"YY_EXPORT","YY_IMPORT",
};
const char * const yy_compile_policyrule[] = {
"$accept : configuration",
"configuration : configuration policy_statement",
"configuration : configuration set",
"configuration : configuration YY_EXPORT YY_ID YY_STR YY_SEMICOLON",
"configuration : configuration YY_IMPORT YY_ID YY_STR YY_SEMICOLON",
"configuration :",
"set : YY_SET YY_STR YY_ID YY_STR YY_SEMICOLON",
"policy_statement : YY_POLICY_STATEMENT YY_ID YY_LBRACE terms YY_RBRACE",
"terms : terms YY_TERM YY_ID YY_LBRACE source dest action YY_RBRACE",
"terms :",
"source : YY_SOURCE YY_LBRACE statements YY_RBRACE",
"dest : YY_DEST YY_LBRACE statements YY_RBRACE",
"action : YY_ACTION YY_LBRACE statements YY_RBRACE",
"statements : statements YY_STATEMENT YY_SEMICOLON",
"statements :",
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
case 3:
#line 90 "compilepolicy.y"
{
		list<string> tmp;

		string proto = yyvsp[-2].c_str;
		string pols = yyvsp[-1].c_str;

		free(yyvsp[-2].c_str);
		free(yyvsp[-1].c_str);
	
		policy_utils::str_to_list(pols,tmp);

		_yy_configuration.update_exports(proto, tmp, "");
	}
break;
case 4:
#line 105 "compilepolicy.y"
{
		list<string> tmp;

		string proto = yyvsp[-2].c_str;
		string pols = yyvsp[-1].c_str;

		free(yyvsp[-2].c_str);
		free(yyvsp[-1].c_str);
	
		policy_utils::str_to_list(pols,tmp);

		_yy_configuration.update_imports(proto, tmp, "");
	}
break;
case 6:
#line 122 "compilepolicy.y"
{
	  	string type = yyvsp[-3].c_str;
	  	string id = yyvsp[-2].c_str;
		string sets = yyvsp[-1].c_str;

		free(yyvsp[-3].c_str); free(yyvsp[-2].c_str); free(yyvsp[-1].c_str);
	  	
		_yy_configuration.create_set(id);
		_yy_configuration.update_set(type, id, sets);
	  }
break;
case 7:
#line 136 "compilepolicy.y"
{
		string pname = yyvsp[-3].c_str;
		free(yyvsp[-3].c_str);

		_yy_configuration.create_policy(pname);

		ConfigNodeId order_generator(ConfigNodeId::ZERO());
		ConfigNodeId prev_order(ConfigNodeId::ZERO());
		ConfigNodeId order(ConfigNodeId::ZERO());
		for(vector<yy_tb*>::iterator i = _yy_terms.begin();
		    i != _yy_terms.end(); ++i) {

			yy_tb* term = *i;

			string& tname = term->name;
			order = order_generator.generate_unique_node_id();
			order.set_position(prev_order.unique_node_id());
			prev_order = order;
			_yy_configuration.create_term(pname, order, tname);

			add_blocks(pname, tname, *term);

			delete term;
		}

	  	_yy_terms.clear();
	  }
break;
case 8:
#line 168 "compilepolicy.y"
{
	  	yy_tb* tb = new yy_tb;

		tb->name = yyvsp[-5].c_str;
		tb->block[0] = yyvsp[-3].statements;
		tb->block[1] = yyvsp[-2].statements;
		tb->block[2] = yyvsp[-1].statements;
		
		free(yyvsp[-5].c_str); 
		_yy_terms.push_back(tb);
	  }
break;
case 10:
#line 184 "compilepolicy.y"
{
		yy_statements* tmp = _yy_statements;
		_yy_statements = NULL;
		yyval.statements = tmp;
	}
break;
case 11:
#line 193 "compilepolicy.y"
{
		yy_statements* tmp = _yy_statements;
		_yy_statements = NULL;
		yyval.statements = tmp;
	}
break;
case 12:
#line 202 "compilepolicy.y"
{
		yy_statements* tmp = _yy_statements;
		_yy_statements = NULL;
		yyval.statements = tmp;
	}
break;
case 13:
#line 210 "compilepolicy.y"
{
	       
	       	if (_yy_statements == NULL) {
			_yy_statements = new yy_statements;
		}
		
		yy_statement* statement = new yy_statement(yyvsp[-1].c_str);
		statement->append(";");
	
		free(yyvsp[-1].c_str);

		_yy_statements->push_back(statement);
	       }
break;
#line 580 "yacc.yy_compile_policy.cc"
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
