%{

#include <string.h>
#include "y.tab.h"

%}

%option noyywrap

%%

\n  	{ return NEWLINE; }
[ \t] 	{ /* Discard spaces and tabs */ }	
">"		{ return GREAT; }
">>"	{ return GREATGREAT; }
">&"	{ return GREATAMPERSAND; }
">>&"	{ return GREATGREATAMPERSAND; }
"<"		{ return LESS; }
"|" 	{ return PIPE; }
"&" 	{ return AMPERSAND; }

\"[^\n\"]*\" {
		// handle arguments in quotes by stripping the quotes

		yytext = (char*)(yytext+1); // drop the front quotes
		yytext[strlen(yytext) - 1] = '\0'; // drop the end quotes
		yylval.string_val = strdup(yytext);
		return WORD;
	}

[^ \t\n][^ \t\n]*  {
		// Assume that file names have only alpha chars
		yylval.string_val = strdup(yytext);
		return WORD;
	}

%%