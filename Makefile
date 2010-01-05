# makefile modified by Henry Thacker
VPATH = frontend interpreter tacgen utils mips
OBJS = lex.yy.o C.tab.o symbol_table.o nodes.o main.o interpreter.o environment.o conversion.o output.o arithmetic.o tacgenerator.o utilities.o mips.o codegen_utils.o registers.o optimisation.o
SRCS = lex.yy.c C.tab.c symbol_table.c nodes.c main.c interpreter.c environment.c conversion.c output.c arithmetic.c tacgenerator.c utilities.c mips.c codegen_utils.c registers.c optimisation.c
CPPFLAGS = -I interpreter -I frontend -I tacgen -I utils -I mips
CC = gcc

all:	mycc

clean:
	rm ${OBJS}

mycc:	${OBJS}
	${CC} -std=c89 -g $(CPPFLAGS) -o mycc ${OBJS} 

lex.yy.c: C.flex
	flex C.flex

C.tab.c:	C.y
	bison -d -t -v C.y

.c.o:
	${CC} -std=c89 -g $(CPPFLAGS) -c $^

depend:	
	${CC} -M $(SRCS) > .deps
	cat Makefile .deps > makefile

dist:	symbol_table.c nodes.c main.c Makefile C.flex C.y nodes.h token.h
	tar cvfz mycc.tgz symbol_table.c nodes.c main.c Makefile C.flex C.y \
		nodes.h token.h
