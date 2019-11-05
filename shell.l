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

[^ \t\n][^ \t\n]*  {
		/* Assume that file names have only alpha chars */
		yylval.string_val = strdup(yytext);
		return WORD;
	}

%%

