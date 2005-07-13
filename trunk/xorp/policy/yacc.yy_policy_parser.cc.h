#ifndef YYERRCODE
#define YYERRCODE 256
#endif

#define YY_INT 257
#define YY_UINT 258
#define YY_STR 259
#define YY_ID 260
#define YY_IPV4 261
#define YY_IPV4NET 262
#define YY_IPV6 263
#define YY_IPV6NET 264
#define YY_NOT 265
#define YY_AND 266
#define YY_XOR 267
#define YY_OR 268
#define YY_HEAD 269
#define YY_CTR 270
#define YY_NE_INT 271
#define YY_EQ 272
#define YY_NE 273
#define YY_LE 274
#define YY_GT 275
#define YY_LT 276
#define YY_GE 277
#define YY_ADD 278
#define YY_SUB 279
#define YY_MUL 280
#define YY_SEMICOLON 281
#define YY_LPAR 282
#define YY_RPAR 283
#define YY_ASSIGN 284
#define YY_SET 285
#define YY_REGEX 286
#define YY_ACCEPT 287
#define YY_REJECT 288
#define YY_PROTOCOL 289
typedef union {
	char *c_str;
	Node *node;
} YYSTYPE;
extern YYSTYPE yy_policy_parserlval;
