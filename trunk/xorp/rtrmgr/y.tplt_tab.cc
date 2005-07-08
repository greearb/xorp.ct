#include <sys/cdefs.h>
#ifndef lint
#if 0
static char yysccsid[] = "@(#)yaccpar	1.9 (Berkeley) 02/21/93";
#else
__IDSTRING(yyrcsid, "$NetBSD: skeleton.c,v 1.14 1997/10/20 03:41:16 lukem Exp $");
#endif
#endif
#include <stdlib.h>
#define YYBYACC 1
#define YYMAJOR 1
#define YYMINOR 9
#define YYLEX yylex()
#define YYEMPTY -1
#define yyclearin (yychar=(YYEMPTY))
#define yyerrok (yyerrflag=0)
#define YYRECOVERING (yyerrflag!=0)
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
#define yysslim tpltsslim
#define yyssp tpltssp
#define yyvs tpltvs
#define yyvsp tpltvsp
#define yystacksize tpltstacksize
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
#define YYPREFIX "tplt"
#line 2 "template.yy"
#define YYSTYPE char*

#include <assert.h>
#include <stdio.h>

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"

#include "template_tree_node.hh"
#include "template_tree.hh"
extern void add_cmd_adaptor(char *cmd, TemplateTree* tt);
extern void add_cmd_action_adaptor(const string& cmd, 
			    const list<string>& action, TemplateTree* tt);

/* XXX: sigh, the -p flag to yacc should do this for us */
#define yystacksize tpltstacksize
#define yysslim tpltsslim
#line 64 "y.tplt_tab.c"
#define UPLEVEL 257
#define DOWNLEVEL 258
#define END 259
#define COLON 260
#define ASSIGN_DEFAULT 261
#define LISTNEXT 262
#define RETURN 263
#define TEXT_TYPE 264
#define INT_TYPE 265
#define UINT_TYPE 266
#define BOOL_TYPE 267
#define TOGGLE_TYPE 268
#define IPV4_TYPE 269
#define IPV4NET_TYPE 270
#define IPV6_TYPE 271
#define IPV6NET_TYPE 272
#define MACADDR_TYPE 273
#define URL_FILE_TYPE 274
#define URL_FTP_TYPE 275
#define URL_HTTP_TYPE 276
#define URL_TFTP_TYPE 277
#define BOOL_VALUE 278
#define INTEGER_VALUE 279
#define IPV4_VALUE 280
#define IPV4NET_VALUE 281
#define IPV6_VALUE 282
#define IPV6NET_VALUE 283
#define MACADDR_VALUE 284
#define URL_FILE_VALUE 285
#define URL_FTP_VALUE 286
#define URL_HTTP_VALUE 287
#define URL_TFTP_VALUE 288
#define VARDEF 289
#define COMMAND 290
#define VARIABLE 291
#define LITERAL 292
#define STRING 293
#define SYNTAX_ERROR 294
#define YYERRCODE 256
short tpltlhs[] = {                                        -1,
    0,    0,    0,    1,    3,    3,    6,    6,    5,    5,
    5,    7,    7,    7,    7,    7,    7,    7,    7,    7,
    7,    7,    7,    7,    7,    8,    8,    8,    8,    8,
    8,    8,    8,    8,    8,    8,    8,    8,    8,    4,
    9,    9,   10,   10,   10,   11,   11,   14,   13,   12,
   12,   15,   16,   17,   18,   18,   19,   19,   19,   19,
   19,   19,   19,   19,   19,   20,   20,    2,
};
short tpltlen[] = {                                         2,
    0,    2,    1,    2,    1,    1,    2,    4,    1,    2,
    2,    1,    1,    1,    1,    1,    1,    1,    1,    1,
    1,    1,    1,    1,    1,    3,    3,    3,    3,    3,
    3,    3,    3,    3,    3,    3,    3,    3,    3,    3,
    0,    2,    1,    1,    1,    1,    1,    4,    4,    1,
    1,    4,    3,    1,    1,    3,    1,    2,    4,    3,
    2,    3,    1,    2,    1,    1,    2,    1,
};
short tpltdefred[] = {                                      0,
    0,   68,    0,    0,    3,    0,    0,    6,    0,    2,
    0,    4,    0,   11,    0,   54,    0,   45,    0,    0,
   43,   44,   46,   47,   50,   51,    0,   12,   13,   14,
   15,   16,   17,   18,   19,   20,   21,   22,   23,   24,
   25,    8,    0,   40,   42,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,   53,    0,    0,   66,    0,   55,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,   48,   49,    0,    0,    0,    0,    0,
   52,    0,   67,   26,   27,   28,   29,   30,   31,   32,
   33,   34,   35,   36,   37,   38,   39,   60,   62,    0,
   56,   59,
};
short tpltdgoto[] = {                                       3,
    4,    5,    6,   12,    7,    8,   42,   62,   19,   20,
   21,   22,   23,   24,   25,   26,   27,   67,   68,   69,
};
short tpltsindex[] = {                                   -282,
 -284,    0,    0, -282,    0, -222, -221,    0, -187,    0,
 -218,    0, -284,    0, -249,    0, -246,    0, -181, -218,
    0,    0,    0,    0,    0,    0, -182,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0, -219,    0,    0, -252, -180, -179, -178, -177,
 -176, -175, -174, -173, -172, -171, -170, -169, -168, -167,
 -164, -163,    0, -214, -226,    0, -253,    0, -213, -196,
 -166, -165, -162, -161, -160, -183, -159, -184, -158, -185,
 -157, -186, -156,    0,    0, -213, -249, -191, -155, -213,
    0, -223,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0, -190,
    0,    0,
};
short tpltrindex[] = {                                    104,
 -256,    0,    0,  104,    0,    0, -152,    0, -255,    0,
 -151,    0, -254,    0,    0,    0, -256,    0,    0, -151,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0, -153, -150, -149, -148,
 -147, -144, -141, -140, -138, -137, -135, -134, -132, -131,
    0,    0,    0, -230, -229,    0,    0,    0, -228,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0, -200,    0, -199, -251, -198,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,    0,    0,    0,    0,    0,    0,    0,    0,
    0,    0,
};
short tpltgindex[] = {                                    126,
   -7,    0,    0,    0,    0,  124,  -43,    0,  113,    0,
    0,    0,    0,    0,    0,    0,    0,    0,   42,   11,
};
#define YYTABLESIZE 134
short tplttable[] = {                                      61,
    9,    7,   10,   18,    9,   91,   63,   66,   92,    1,
   66,    2,   18,   43,   28,   29,   30,   31,   32,   33,
   34,   35,   36,   37,   38,   39,   40,   41,   63,   65,
   57,   63,   65,   57,   11,    9,    7,   10,   64,   65,
   66,   66,    9,  108,   47,   48,   49,   50,   51,   52,
   53,   54,   55,   56,   57,   58,   59,   60,   64,   61,
   58,   64,   61,   58,   87,   88,   89,   64,   65,   66,
   13,   16,   15,   17,   86,   90,   44,   46,   66,   93,
   70,   71,   72,   73,   74,   75,   76,   77,   78,   79,
   80,   81,   82,   83,   84,   85,   94,  100,  102,  104,
  106,  109,  112,    1,    5,   12,   41,  110,   13,   14,
   15,   16,   95,   96,   17,   97,   98,   18,   19,   99,
   20,   21,  101,   22,   23,  103,   24,   25,  105,   10,
   14,  107,   45,  111,
};
short tpltcheck[] = {                                      43,
  257,  257,  257,   11,  289,  259,  259,  259,  262,  292,
  262,  294,   20,  260,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  259,  259,
  259,  262,  262,  262,  257,  292,  292,  292,  291,  292,
  293,  293,  289,   87,  264,  265,  266,  267,  268,  269,
  270,  271,  272,  273,  274,  275,  276,  277,  259,  259,
  259,  262,  262,  262,  291,  292,  293,  291,  292,  293,
  292,  290,  260,  292,   64,   65,  258,  260,  293,  293,
  261,  261,  261,  261,  261,  261,  261,  261,  261,  261,
  261,  261,  261,  261,  259,  259,  293,  281,  283,  285,
  287,  293,  293,    0,  257,  259,  258,  263,  259,  259,
  259,  259,  279,  279,  259,  278,  278,  259,  259,  280,
  259,  259,  282,  259,  259,  284,  259,  259,  286,    4,
    7,  288,   20,   92,
};
#define YYFINAL 3
#ifndef YYDEBUG
#define YYDEBUG 0
#endif
#define YYMAXTOKEN 294
#if YYDEBUG
char *tpltname[] = {
"end-of-file",0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,"UPLEVEL","DOWNLEVEL","END",
"COLON","ASSIGN_DEFAULT","LISTNEXT","RETURN","TEXT_TYPE","INT_TYPE","UINT_TYPE",
"BOOL_TYPE","TOGGLE_TYPE","IPV4_TYPE","IPV4NET_TYPE","IPV6_TYPE","IPV6NET_TYPE",
"MACADDR_TYPE","URL_FILE_TYPE","URL_FTP_TYPE","URL_HTTP_TYPE","URL_TFTP_TYPE",
"BOOL_VALUE","INTEGER_VALUE","IPV4_VALUE","IPV4NET_VALUE","IPV6_VALUE",
"IPV6NET_VALUE","MACADDR_VALUE","URL_FILE_VALUE","URL_FTP_VALUE",
"URL_HTTP_VALUE","URL_TFTP_VALUE","VARDEF","COMMAND","VARIABLE","LITERAL",
"STRING","SYNTAX_ERROR",
};
char *tpltrule[] = {
"$accept : input",
"input :",
"input : definition input",
"input : syntax_error",
"definition : nodename nodegroup",
"nodename : literals",
"nodename : named_literal",
"named_literal : LITERAL VARDEF",
"named_literal : LITERAL VARDEF COLON type",
"literals : LITERAL",
"literals : literals LITERAL",
"literals : literals named_literal",
"type : TEXT_TYPE",
"type : INT_TYPE",
"type : UINT_TYPE",
"type : BOOL_TYPE",
"type : TOGGLE_TYPE",
"type : IPV4_TYPE",
"type : IPV4NET_TYPE",
"type : IPV6_TYPE",
"type : IPV6NET_TYPE",
"type : MACADDR_TYPE",
"type : URL_FILE_TYPE",
"type : URL_FTP_TYPE",
"type : URL_HTTP_TYPE",
"type : URL_TFTP_TYPE",
"init_type : TEXT_TYPE ASSIGN_DEFAULT STRING",
"init_type : INT_TYPE ASSIGN_DEFAULT INTEGER_VALUE",
"init_type : UINT_TYPE ASSIGN_DEFAULT INTEGER_VALUE",
"init_type : BOOL_TYPE ASSIGN_DEFAULT BOOL_VALUE",
"init_type : TOGGLE_TYPE ASSIGN_DEFAULT BOOL_VALUE",
"init_type : IPV4_TYPE ASSIGN_DEFAULT IPV4_VALUE",
"init_type : IPV4NET_TYPE ASSIGN_DEFAULT IPV4NET_VALUE",
"init_type : IPV6_TYPE ASSIGN_DEFAULT IPV6_VALUE",
"init_type : IPV6NET_TYPE ASSIGN_DEFAULT IPV6NET_VALUE",
"init_type : MACADDR_TYPE ASSIGN_DEFAULT MACADDR_VALUE",
"init_type : URL_FILE_TYPE ASSIGN_DEFAULT URL_FILE_VALUE",
"init_type : URL_FTP_TYPE ASSIGN_DEFAULT URL_FTP_VALUE",
"init_type : URL_HTTP_TYPE ASSIGN_DEFAULT URL_HTTP_VALUE",
"init_type : URL_TFTP_TYPE ASSIGN_DEFAULT URL_TFTP_VALUE",
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
"cmd : list_of_cmd_strings",
"cmd : LITERAL list_of_cmd_strings",
"cmd : LITERAL STRING RETURN STRING",
"cmd : LITERAL VARIABLE type",
"cmd : LITERAL LITERAL",
"cmd : LITERAL LITERAL STRING",
"cmd : VARIABLE",
"cmd : VARIABLE list_of_cmd_strings",
"cmd : LITERAL",
"list_of_cmd_strings : STRING",
"list_of_cmd_strings : list_of_cmd_strings STRING",
"syntax_error : SYNTAX_ERROR",
};
#endif
#ifndef YYSTYPE
typedef int YYSTYPE;
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
#line 264 "template.yy"

extern char *lstr;
extern char *vstr;
extern char *cstr;
extern char *sstr;
extern char *istr;
extern FILE *tpltin;
extern int tplt_linenum;
extern "C" int tpltparse();
extern int tpltlex();

static TemplateTree* tt = NULL;
static string tplt_filename;
static string lastsymbol;
static int tplt_type;
static char *tplt_initializer = NULL;
static string current_cmd;
static list<string> cmd_list;


static void
extend_path(char *segment, bool is_tag)
{
    lastsymbol = segment;

    string segname;
    segname = segment;
    tt->extend_path(segname, is_tag);
    free(segment);
}

static void
push_path()
{
    tt->push_path(tplt_type, tplt_initializer);
    tplt_type = NODE_VOID;
    if (tplt_initializer != NULL) {
	free(tplt_initializer);
	tplt_initializer = NULL;
    }
}

static void
pop_path()
{
    tt->pop_path();
    tplt_type = NODE_VOID;
    if (tplt_initializer != NULL) {
	free(tplt_initializer);
	tplt_initializer = NULL;
    }
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
    lastsymbol = cmd;

    add_cmd_adaptor(cmd, tt);
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
prepend_cmd(char *s)
{
    lastsymbol = s;

    cmd_list.push_front(string(s));
    free(s);
}

static void
end_cmd()
{
    add_cmd_action_adaptor(current_cmd, cmd_list, tt);
    cmd_list.clear();
}

void
tplterror(const char *s) throw (ParseError)
{
    string errmsg;

    errmsg = c_format("PARSE ERROR [Template File: %s line %d]: %s",
		      tplt_filename.c_str(), tplt_linenum, s);
    errmsg += c_format("; Last symbol parsed was \"%s\"", lastsymbol.c_str());

    xorp_throw(ParseError, errmsg);
}

int
init_template_parser(const char *filename, TemplateTree *c)
{
    tt = c;
    tplt_linenum = 1;

    tpltin = fopen(filename, "r");
    if (tpltin == NULL)
	return -1;

    tplt_type = NODE_VOID;
    tplt_initializer = NULL;
    tplt_filename = filename;
    return 0;
}

void
complete_template_parser()
{
    if (tpltin != NULL)
        fclose(tpltin);
}

void
parse_template() throw (ParseError)
{
    if (tpltparse() != 0)
	tplterror("unknown error");
}
#line 465 "y.tplt_tab.c"
/* allocate initial stack or double stack size, up to YYMAXDEPTH */
int yyparse __P((void));
static int yygrowstack __P((void));
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
    if ((newss = (short *)realloc(yyss, newsize * sizeof *newss)) == NULL)
        return -1;
    yyss = newss;
    yyssp = newss + i;
    if ((newvs = (YYSTYPE *)realloc(yyvs, newsize * sizeof *newvs)) == NULL)
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
int
yyparse()
{
    int yym, yyn, yystate;
#if YYDEBUG
    char *yys;

    if ((yys = getenv("YYDEBUG")) != NULL)
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
    if ((yyn = yydefred[yystate]) != 0) goto yyreduce;
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
    goto yynewerror;
yynewerror:
    yyerror("syntax error");
    goto yyerrlab;
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
case 5:
#line 71 "template.yy"
{ push_path(); }
break;
case 6:
#line 72 "template.yy"
{ push_path(); }
break;
case 7:
#line 75 "template.yy"
{
			extend_path(yyvsp[-1], true);
			extend_path(yyvsp[0], false);
		}
break;
case 8:
#line 79 "template.yy"
{
			extend_path(yyvsp[-3], true);
			extend_path(yyvsp[-2], false);
		}
break;
case 9:
#line 85 "template.yy"
{ extend_path(yyvsp[0], false); }
break;
case 10:
#line 86 "template.yy"
{ extend_path(yyvsp[0], false); }
break;
case 12:
#line 90 "template.yy"
{ tplt_type = NODE_TEXT; }
break;
case 13:
#line 91 "template.yy"
{ tplt_type = NODE_INT; }
break;
case 14:
#line 92 "template.yy"
{ tplt_type = NODE_UINT; }
break;
case 15:
#line 93 "template.yy"
{ tplt_type = NODE_BOOL; }
break;
case 16:
#line 94 "template.yy"
{ tplt_type = NODE_TOGGLE; }
break;
case 17:
#line 95 "template.yy"
{ tplt_type = NODE_IPV4; }
break;
case 18:
#line 96 "template.yy"
{ tplt_type = NODE_IPV4NET; }
break;
case 19:
#line 97 "template.yy"
{ tplt_type = NODE_IPV6; }
break;
case 20:
#line 98 "template.yy"
{ tplt_type = NODE_IPV6NET; }
break;
case 21:
#line 99 "template.yy"
{ tplt_type = NODE_MACADDR; }
break;
case 22:
#line 100 "template.yy"
{ tplt_type = NODE_URL_FILE; }
break;
case 23:
#line 101 "template.yy"
{ tplt_type = NODE_URL_FTP; }
break;
case 24:
#line 102 "template.yy"
{ tplt_type = NODE_URL_HTTP; }
break;
case 25:
#line 103 "template.yy"
{ tplt_type = NODE_URL_TFTP; }
break;
case 26:
#line 106 "template.yy"
{
			tplt_type = NODE_TEXT;
			tplt_initializer = yyvsp[0];
		}
break;
case 27:
#line 110 "template.yy"
{
			tplt_type = NODE_INT;
			tplt_initializer = yyvsp[0];
		}
break;
case 28:
#line 114 "template.yy"
{
			tplt_type = NODE_UINT;
			tplt_initializer = yyvsp[0];
		}
break;
case 29:
#line 118 "template.yy"
{
			tplt_type = NODE_BOOL;
			tplt_initializer = yyvsp[0];
		}
break;
case 30:
#line 122 "template.yy"
{
			tplt_type = NODE_TOGGLE;
			tplt_initializer = yyvsp[0];
		}
break;
case 31:
#line 126 "template.yy"
{
			tplt_type = NODE_IPV4;
			tplt_initializer = yyvsp[0];
		}
break;
case 32:
#line 130 "template.yy"
{
			tplt_type = NODE_IPV4NET;
			tplt_initializer = yyvsp[0];
		}
break;
case 33:
#line 134 "template.yy"
{
			tplt_type = NODE_IPV6;
			tplt_initializer = yyvsp[0];
		}
break;
case 34:
#line 138 "template.yy"
{
			tplt_type = NODE_IPV6NET;
			tplt_initializer = yyvsp[0];
		}
break;
case 35:
#line 142 "template.yy"
{
			tplt_type = NODE_MACADDR;
			tplt_initializer = yyvsp[0];
		}
break;
case 36:
#line 146 "template.yy"
{
			tplt_type = NODE_URL_FILE;
			tplt_initializer = yyvsp[0];
		}
break;
case 37:
#line 150 "template.yy"
{
			tplt_type = NODE_URL_FTP;
			tplt_initializer = yyvsp[0];
		}
break;
case 38:
#line 154 "template.yy"
{
			tplt_type = NODE_URL_HTTP;
			tplt_initializer = yyvsp[0];
		}
break;
case 39:
#line 158 "template.yy"
{
			tplt_type = NODE_URL_TFTP;
			tplt_initializer = yyvsp[0];
		}
break;
case 40:
#line 164 "template.yy"
{ pop_path(); }
break;
case 48:
#line 180 "template.yy"
{ terminal(yyvsp[-3]); }
break;
case 49:
#line 183 "template.yy"
{ terminal(yyvsp[-3]); }
break;
case 54:
#line 196 "template.yy"
{ add_cmd(yyvsp[0]); }
break;
case 57:
#line 203 "template.yy"
{
			end_cmd();
		}
break;
case 58:
#line 206 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 59:
#line 210 "template.yy"
{
			append_cmd(yyvsp[-3]);
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 60:
#line 217 "template.yy"
{ /* e.g.: set FOOBAR ipv4 */
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 61:
#line 223 "template.yy"
{
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 62:
#line 228 "template.yy"
{
			append_cmd(yyvsp[-2]);
			append_cmd(yyvsp[-1]);
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 63:
#line 234 "template.yy"
{
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 64:
#line 238 "template.yy"
{
			prepend_cmd(yyvsp[-1]);
			end_cmd();
		}
break;
case 65:
#line 242 "template.yy"
{
			append_cmd(yyvsp[0]);
			end_cmd();
		}
break;
case 66:
#line 249 "template.yy"
{
			append_cmd(yyvsp[0]);
		}
break;
case 67:
#line 252 "template.yy"
{
			append_cmd(yyvsp[0]);
		}
break;
case 68:
#line 257 "template.yy"
{
			tplterror("syntax error");
		}
break;
#line 919 "y.tplt_tab.c"
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
