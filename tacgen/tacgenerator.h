#ifndef __TACGEN_H
#define __TACGEN_H

#define TT_IF 150
#define TT_GOTO 151
#define TT_LABEL 152
#define TT_OP 153
#define TT_ASSIGN 154
#define TT_RETURN 155
#define TT_BEGIN_FN 156
#define TT_END_FN 157
#define TT_FN_DEF 158
#define TT_INIT_FRAME 159
#define TT_PUSH_PARAM 160
#define TT_POP_PARAM 161
#define TT_FN_CALL 162
#define TT_PREPARE 163
#define TT_FN_BODY 164

#define EMBEDDED_FNS 300

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "interpreter.h"
#include "token.h"

/* Fn prototypes */
struct tac_quad *make_quad_value(char *, value *, value *, value *, int, int);
struct tac_quad *start_tac_gen(NODE *);
value *make_simple(environment *, NODE *, int, int);
void print_tac(struct tac_quad *);

/* TAC structure */
typedef struct tac_quad {
	char *op;
	value *operand1;
	value *operand2;	
	value *result;
	int type;
	int subtype;
	struct tac_quad *next;		
}tac_quad;

typedef struct simple {
	char *label;
	struct tac_quad *code;
}simple;

value *null_fn;

tac_quad *tac_output;

#endif