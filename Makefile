cc = gcc -g
CC = g++ -g

LEX = lex
YACC = yacc

all: shell

lex.yy.c: shell.l
	$(LEX) shell.l

y.tab.c: shell.y
	$(YACC) -d shell.y
	
shell: lex.yy.c y.tab.c command.cpp
	$(CC) -o shell lex.yy.c y.tab.c command.cpp

clean:
	rm -f lex.yy.c y.tab.c y.tab.h shell *.o *.out
