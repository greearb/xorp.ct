#ifndef YYERRCODE
#define YYERRCODE 256
#endif

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
typedef union {
	char *c_str;
	yy_statements* statements;
} YYSTYPE;
extern YYSTYPE yy_compile_policylval;
