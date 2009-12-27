#ifndef __MIPS_H
#define __MIPS_H

#define ACTIVATION_RECORD_SIZE  (local_size(quad->operand1) + 2) * 4

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "token.h"
#include "tacgenerator.h"

/* Fn prototypes */
void code_gen(NODE *);

/* Store reference to entry point */
tac_quad *entry_point;

#endif