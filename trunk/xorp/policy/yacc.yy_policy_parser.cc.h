#ifndef YYERRCODE
#define YYERRCODE 256
#endif

#define YY_INT 257
#define YY_UINT 258
#define YY_UINTRANGE 259
#define YY_STR 260
#define YY_ID 261
#define YY_IPV4 262
#define YY_IPV4RANGE 263
#define YY_IPV4NET 264
#define YY_IPV6 265
#define YY_IPV6RANGE 266
#define YY_IPV6NET 267
#define YY_NOT 268
#define YY_AND 269
#define YY_XOR 270
#define YY_OR 271
#define YY_HEAD 272
#define YY_CTR 273
#define YY_NE_INT 274
#define YY_EQ 275
#define YY_NE 276
#define YY_LE 277
#define YY_GT 278
#define YY_LT 279
#define YY_GE 280
#define YY_ADD 281
#define YY_SUB 282
#define YY_MUL 283
#define YY_SEMICOLON 284
#define YY_LPAR 285
#define YY_RPAR 286
#define YY_ASSIGN 287
#define YY_SET 288
#define YY_REGEX 289
#define YY_ACCEPT 290
#define YY_REJECT 291
#define YY_PROTOCOL 292
typedef union {
	char *c_str;
	Node *node;
} YYSTYPE;
extern YYSTYPE yy_policy_parserlval;
