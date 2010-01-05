#ifndef __MOPT_H
#define __MOPT_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "conversion.h"
#include "token.h"
#include "registers.h"
#include "codegen_utils.h"

mips_instruction *do_optimise(mips_instruction *);

#endif