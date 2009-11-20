#ifndef __TACGEN_H
#define __TACGEN_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "token.h"

/* Fn prototypes */
void start_tac_gen(NODE *);

/* TAC structure */
typedef struct tac_quad {
	char *op;
	char *operand1;
	char *operand2;	
	char *result;
	struct tac_quad *next;		
}tac_quad;

typedef struct simple {
	char *label;
	struct tac_quad *code;
}simple;

tac_quad *tac_output;

#endif