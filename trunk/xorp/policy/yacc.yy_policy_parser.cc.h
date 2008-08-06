#ifndef YYERRCODE
#define YYERRCODE 256
#endif

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
#define YY_NOT 282
#define YY_AND 283
#define YY_XOR 284
#define YY_OR 285
#define YY_HEAD 286
#define YY_CTR 287
#define YY_NE_INT 288
#define YY_EQ 289
#define YY_NE 290
#define YY_LE 291
#define YY_GT 292
#define YY_LT 293
#define YY_GE 294
#define YY_IPNET_EQ 295
#define YY_IPNET_LE 296
#define YY_IPNET_GT 297
#define YY_IPNET_LT 298
#define YY_IPNET_GE 299
#define YY_ADD 300
#define YY_SUB 301
#define YY_MUL 302
typedef union {
	char*		c_str;
	Node*		node;
	BinOper*	op;
} YYSTYPE;
extern YYSTYPE yy_policy_parserlval;
