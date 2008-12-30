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
typedef union {
	char*		c_str;
	Node*		node;
	BinOper*	op;
} YYSTYPE;
extern YYSTYPE yy_policy_parserlval;
