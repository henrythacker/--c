#ifndef __MIPS_H
#define __MIPS_H

#define ACTIVATION_RECORD_SIZE  (local_size(quad->operand1) + 2) * 4

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "token.h"
#include "registers.h"
#include "tacgenerator.h"
#include "interpreter.h"
#include "codegen_utils.h"

/* Fn prototypes */
void code_gen(NODE *);

/* Store reference to entry point */
tac_quad *entry_point;

/* Location that code is written to */
mips_instruction *instructions;

register_contents** regs;

int has_used_fn_variable;
value *current_fn;

#endif