%token	<string_val> WORD

%token 	NOTOKEN NEWLINE GREAT LESS PIPE AMPERSAND GREATGREAT GREATGREATAMPERSAND GREATAMPERSAND

%union	{
	char *string_val;
}

%{

#include <stdio.h>
#include <string.h>
#include <fcntl.h> // for open() arguments
#include "command.h"
void yyerror(const char * s);
int yylex();

%}

%%

goal:	
	commands
	;

commands: 
	command
	| commands command 
	;

command: 
	simple_command
    ;

simple_command:	
	pipe_list iomodifier_list background_opt NEWLINE {
		printf("YACC: Execute command\n");
		Command::_currentCommand.execute();
	}
	| NEWLINE { 
		Command::_currentCommand.clear();
		Command::_currentCommand.prompt();
	}
	| error NEWLINE { yyerrok; }
	;

pipe_list:
	pipe_list PIPE command_and_args
	| command_and_args 
	;


command_and_args:
	command_word arg_list {
		Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
	}
	;

arg_list:
	arg_list argument
	|
	;

argument:
	WORD {
		printf("YACC: Insert argument \"%s\"\n", $1);
	    Command::_currentSimpleCommand->insertArgument($1);
	}
	;

command_word:
	WORD {
		printf("YACC: Insert command \"%s\"\n", $1);

	    Command::_currentSimpleCommand = new SimpleCommand();
	    Command::_currentSimpleCommand->insertArgument($1);
	}
	;

iomodifier_list:
	iomodifier_list iomodifier
	|
	;

iomodifier:
	GREATGREAT WORD {
		printf("YACC: Insert output \"%s\"\n", $2);
		// append stdout to file
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT;
		Command::_currentCommand._outFile = $2;
	}
	| GREAT WORD {
		printf("YACC: Insert output \"%s\"\n", $2);
		// rewrite the file if it already exists
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT | O_TRUNC;
		Command::_currentCommand._outFile = $2;
	}
	| GREATGREATAMPERSAND WORD {
		// redirect stdout and stderr to file and append
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;

	}
	| GREATAMPERSAND WORD {
		// redirect stdout and stderr to file and truncate
		Command::_currentCommand._openOptions = O_WRONLY | O_CREAT | O_TRUNC;
		Command::_currentCommand._outFile = $2;
		Command::_currentCommand._errFile = $2;

	}
	| LESS WORD {
		printf("YACC: Insert input \"%s\"\n", $2);
		Command::_currentCommand._inputFile = $2;
	} 
	;

background_opt:
	AMPERSAND {
		Command::_currentCommand._background = 1;
	}
	|
	;

%%

void yyerror(const char * s)
{
	fprintf(stderr,"%s", s);
}
