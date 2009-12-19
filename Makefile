# makefile modified by Henry Thacker
VPATH = frontend interpreter tacgen utils
OBJS = lex.yy.o C.tab.o symbol_table.o nodes.o main.o interpreter.o environment.o conversion.o output.o arithmetic.o tacgenerator.o environment2.o utilities.o
SRCS = lex.yy.c C.tab.c symbol_table.c nodes.c main.c interpreter.c environment.c conversion.c output.c arithmetic.c tacgenerator.c environment.c utilities.o
CPPFLAGS = -I interpreter -I frontend -I tacgen -I utils
CC = gcc

all:	mycc

clean:
	rm ${OBJS}

mycc:	${OBJS}
	${CC} -g $(CPPFLAGS) -o mycc ${OBJS} 

lex.yy.c: C.flex
	flex C.flex

C.tab.c:	C.y
	bison -d -t -v C.y

.c.o:
	${CC} -g $(CPPFLAGS) -c $^

depend:	
	${CC} -M $(SRCS) > .deps
	cat Makefile .deps > makefile

dist:	symbol_table.c nodes.c main.c Makefile C.flex C.y nodes.h token.h
	tar cvfz mycc.tgz symbol_table.c nodes.c main.c Makefile C.flex C.y \
		nodes.h token.h
