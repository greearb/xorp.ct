	int level=0;
	int newstatement=1;
	int i;
	void tab(int level) { 
	    for(i=0;i<4*level;i++) printf(" ");
	}
%%
"{"	{
	level++;
	newstatement=1;
	printf("({)\n");
	}
"}"	{
	level--;
	newstatement=1;
	tab(level);
	printf("(})\n");
	}
"text"	{
	printf("TYPE(text) ");
	newstatement=0;
	}
"uint"	{
	printf("TYPE(uint) ");
	newstatement=0;
	}
"bool"	{
	printf("TYPE(bool) ");
	newstatement=0;
	}
"toggle"	{
	printf("TYPE(toggle) ");
	newstatement=0;
	}
"ipvx"	{
	printf("TYPE(ipvx) ");
	newstatement=0;
	}
"ipvx_prefix"	{
	printf("TYPE(ipvx_prefix) ");
	newstatement=0;
	}
"="	{
	printf("ASSIGN_DEFAULT ");
	newstatement=0;
	}
";"	{
	printf("(;)\n");
	newstatement=1;
	}
"."	{
	printf("(.) ");
	newstatement=0;
	}
":"	{
	printf("ASSIGN_VALUE ");
	newstatement=0;
	}
","	{
	printf("LIST_NEXT ");
	newstatement=0;
	}
"->"	{
	printf("RETURN ");
	newstatement=0;
	}
"true"	{
	printf("TRUE ");
	newstatement=0;
	}
"false"	{
	printf("FALSE ");
	newstatement=0;
	}
[ \t\n]+	/*whitespace*/
"\%"[a-z][a-z0-9"\-""_"]*	{
	if (newstatement == 1) tab(level);
	printf("COMMAND(%s) ", yytext);
	newstatement=0;
	}
[A-Z][A-Z0-9"\-""_"]*	{
	printf("VARIABLE(%s) ", yytext);
	newstatement=0;
	}
[a-z][a-z0-9"\-""_"]*	{
	if (newstatement == 1) tab(level);
	printf("TOKEN(%s) ", yytext);
	newstatement=0;
	}
\"[a-zA-Z0-9"\-""_""\[""\]"":""\/"]*\"	{
	printf("STRING(%s) ", yytext);
	newstatement=0;
	}
[0-9]+	{
	printf("INTEGER(%s) ", yytext);
	newstatement=0;
	}

%%

main() 
{
	yylex();
}