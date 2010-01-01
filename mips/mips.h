#ifndef __MIPS_H
#define __MIPS_H

#define ACTIVATION_RECORD_SIZE  (local_size(quad->operand1) + 2) * 4
#define REG_COUNT 15

#define REG_NONE_FREE -500
#define REG_VALUE_NOT_AVAILABLE -501

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "token.h"
#include "tacgenerator.h"
#include "interpreter.h"
#include "codegen_utils.h"

/* Fn prototypes */
void code_gen(NODE *);

/* Store reference to entry point */
tac_quad *entry_point;

/* Store pending code that can't be processed immediately */
tac_quad *pending_code;

typedef struct register_contents {
	value *contents; /* Value stored in the register */
	int accesses; /* How many times the value has been referenced */
	int assignment_id; /* What order this assignment was made */
	char* name;
}register_contents;

/*
* POSITIONS
* [0] $0 - special zero register
* [1 - 5] $a0, $a1, $a2, $a3
* [6 - 15] $t0, $t1, $t2, $t3, $t4, $t5, $t6, $t7, $t8, $t9
*
*/
register_contents** regs;
int regs_assignments;

#endif