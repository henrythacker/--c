#ifndef __MIPS_H
#define __MIPS_H

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

#endif