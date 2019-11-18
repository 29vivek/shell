#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include <fcntl.h>

#include "command.h"

SimpleCommand::SimpleCommand()
{
	// just enough space for 5 args now
	_numberOfAvailableArguments = 5;
	_numberOfArguments = 0;
	_arguments = (char **) malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char * argument)
{
	if(_numberOfAvailableArguments == _numberOfArguments  + 1) {
		// double the available space
		_numberOfAvailableArguments *= 2;
		_arguments = (char **) realloc(_arguments,
				  _numberOfAvailableArguments * sizeof(char *));
	}
	
	_arguments[_numberOfArguments] = argument;

	// null terminate the args
	_arguments[_numberOfArguments + 1] = NULL;
	
	_numberOfArguments++;
}

Command::Command()
{
	// space for one simple command
	_numberOfAvailableSimpleCommands = 1;
	_simpleCommands = (SimpleCommand **)
		malloc(_numberOfSimpleCommands * sizeof(SimpleCommand *));

	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
	_openOptions = 0;
}

void Command::insertSimpleCommand(SimpleCommand * simpleCommand)
{
	if(_numberOfAvailableSimpleCommands == _numberOfSimpleCommands) {
		_numberOfAvailableSimpleCommands *= 2;
		_simpleCommands = (SimpleCommand **) realloc(_simpleCommands,
			 _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
	}
	
	_simpleCommands[_numberOfSimpleCommands] = simpleCommand;
	_numberOfSimpleCommands++;
}

void Command::clear()
{
	for(int i = 0; i < _numberOfSimpleCommands; i++) {
		for(int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
			free(_simpleCommands[i]->_arguments[j]);
		}
		
		free(_simpleCommands[i]->_arguments);
		free(_simpleCommands[i]);
	}

	if(_outFile) {
		free(_outFile);
	}

	if(_inputFile) {
		free(_inputFile);
	}
	
	_numberOfSimpleCommands = 0;
	_outFile = 0;
	_inputFile = 0;
	_errFile = 0;
	_background = 0;
}

void Command::print()
{
	printf("\n---------------------------------------------------------\n");
	printf("COMMAND TABLE\n");
	printf("Simple Commands: \n");
	for(int i = 0; i < _numberOfSimpleCommands; i++) {
		printf("%d- ", i);
		for(int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++) {
			printf("\"%s\" ", _simpleCommands[i]->_arguments[j]);
		}
		printf("\n");
	}

	printf("\n");
	printf("Output: %s\n", _outFile ? _outFile : "Default");
	printf("Input: %s\n", _inputFile ? _inputFile : "Default");
	printf("Error: %s\n", _errFile? _errFile: "Default");
	printf("Background: %s\n", _background? "YES" : "NO");
	printf("---------------------------------------------------------\n");
	printf("\n");
}

void Command::execute()
{
	// nothing if no simple commands
	if(_numberOfSimpleCommands == 0) {
		prompt();
		return;
	}

	// print command data structure
	print();
	
	// save stdin / stdout / stderr
	int tempIn = dup(0);
	int tempOut = dup(1);
	int tempErr = dup(2);

	// set input
	int fdIn;
	if(_inputFile) { 	// open file for reading
		fdIn = open(_inputFile, O_RDONLY); 
	} else { 			// use default input
		fdIn = dup(tempIn);
	}

	int i;
	int fdOut;
	pid_t child;
	for(i = 0; i < _numberOfSimpleCommands; i++) {
		// redirect input and close fdIn since we're done with it
		dup2(fdIn, 0);
		close(fdIn);
		
		// setup output
		if(i == _numberOfSimpleCommands - 1) { // last simple command
			if(_outFile) { //redirect output
				mode_t openMode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // -rw-r-----
				fdOut = open(_outFile, _openOptions, openMode);
			} else { //use default output
				fdOut = dup(tempOut);
			}

	 	} else { // not last simple command, so create a pipe
			int fdPipe[2];
			pipe(fdPipe);
			fdOut = fdPipe[1];
			fdIn = fdPipe[0];
		}

		
		dup2(fdOut, 1);  // redirect output (stdout to fdOut)
		if(_errFile) {  // redirect error at same location as output if necessary
			dup2(fdOut, 2); 
		}
		close(fdOut); //close fdOut since we're done with it
		
		// check for special commands
		if (!strcmp(_simpleCommands[i]->_arguments[0], "bye") || 
			!strcmp(_simpleCommands[i]->_arguments[0], "exit") ||
			!strcmp(_simpleCommands[i]->_arguments[0], "quit")) { // exit
				exit(1);
		} else if(!strcmp(_simpleCommands[i]->_arguments[0], "cd")) {
			if(_simpleCommands[i]->_numberOfArguments != 2) {
				fprintf(stderr, "%s", "Invalid number of arguments\n");
			} else {
				int ret = chdir(_simpleCommands[i]->_arguments[1]);
				if(ret) {
					// chdir failed, perror it
					perror("cd");
				}
			}
			child = 1;
			continue;
		} else if(!strcmp(_simpleCommands[i]->_arguments[0], "help")) {
			// ignore args
			fprintf(stdout, "%s", "Builtin commands:\n  cd: change directory\n"
				"  exit/bye/quit: to quit shell\n  isleap: check if year is leap year\n"
				"  op: op followed by 2 operands followed by +, -, * or /\n"
				"  hello: say hello"
				"  help: to view this message\n");
			child = 1;
			continue;
		} else if(!strcmp(_simpleCommands[i]->_arguments[0], "hello")) {
			// ignore args
			char* user = getenv("USER");
			printf("hello %s. having a great day?\n", user);
			child = 1;
			continue;
		} else if(!strcmp(_simpleCommands[i]->_arguments[0], "op")) {
			// op 6 7 + of this type
			if(_simpleCommands[i]->_numberOfArguments != 4) {
				fprintf(stderr, "%s", "Invalid number of arguments\nUse help to know usage\n");
			} else {
				if(!isdigit(_simpleCommands[i]->_arguments[1][0]) ||
					!isdigit(_simpleCommands[i]->_arguments[2][0])) {
						fprintf(stderr, "%s", "Invalid operands!\n");
						break;
					}
				double a = atoi(_simpleCommands[i]->_arguments[1]);
				double b = atoi(_simpleCommands[i]->_arguments[2]);
				switch(_simpleCommands[i]->_arguments[3][0]) {
					case '+': printf("addition gives: %.2f\n", a+b); break;
					case '-': printf("subtraction gives: %.2f\n", a-b); break;
					case '*': printf("multiplication gives: %.2f\n", a*b); break;
					case '/': printf("division gives: %.2f\n", a/b); break;
					default: printf("Invalid operation!\n");
				}
			}
			child = 1;
			continue;
		} else if(!strcmp(_simpleCommands[i]->_arguments[0], "isleap")) {
			// convert args into int and check
			if(_simpleCommands[i]->_numberOfArguments < 2) {
				fprintf(stderr, "%s", "Invalid number of arguments\n");
			} else {
				for(int j=1; j<_simpleCommands[i]->_numberOfArguments; j++) {
					int year = atoi(_simpleCommands[i]->_arguments[j]);
					if(!year) {
						// invalid input - a string
						printf("%s is not a year!\n", _simpleCommands[i]->_arguments[j]);
						continue;
					}
					if((year%4 == 0) && (year%100 != 0 || year%400 == 0)) {
						printf("%d is a leap year.\n", year);
					} else {
						printf("%d is not a leap year.\n", year);
					}
				}
			}
			child = 1;
			continue;
		} else { 
			// else we fork!
			child = fork();
		}
		
		if(child == 0) { // child process
			signal(SIGINT, SIG_DFL); // reset signal
			execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);

			// if the child process reaches this point, then execvp must have failed
			perror("execvp");
			exit(1);
		} else if(child < 0) {
			fprintf(stderr, "Fork failed\n");
			exit(1);
		}

	} // endfor

	// restore in/out/err defaults
	dup2(tempIn, 0);
	dup2(tempOut, 1);
	dup2(tempErr, 2);
	close(tempIn);
	close(tempOut);
	close(tempErr);

	if(!_background) {
		waitpid(child, NULL, 0);
	}

	// clear to prepare for next command
	clear();
	
	// Print new prompt
	prompt();
}

void Command::prompt()
{
	printf("> ");
	fflush(stdout);
}

Command Command::_currentCommand;
SimpleCommand * Command::_currentSimpleCommand;

int yyparse(void);

void sigint_handler(int p) {
	printf("\n");
	Command::_currentCommand.clear();
	Command::_currentCommand.prompt();
}


int main()
{
	struct sigaction sa;
	sa.sa_handler = sigint_handler;
	sa.sa_flags = SA_RESTART; // restart any interrupted system calls
	sigemptyset(&sa.sa_mask);

	// set the SIGINT handler
	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		exit(1);
	}

	Command::_currentCommand.prompt();
	yyparse();
	return 0;
}

