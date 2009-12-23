#ifndef __INTERP_H
#define __INTERP_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include "C.tab.h"
#include "nodes.h"
#include "token.h"
#include "arithmetic.h"
#include "output.h"
#include "conversion.h"
#include "environment.h"
#include "utilities.h"

/* Interpretation flags */
#define INTERPRET_FULL INT_MIN + 1
#define INTERPRET_FN_SCAN INT_MIN + 2
#define INTERPRET_PARAMS INT_MIN + 3

/* Vars */
environment *initial_environment;
value *null_function;

/* Fn prototypes */
void start_interpret(NODE *);
value *string_temporary(char *);
void register_variable_subtree(environment *, NODE *, int);
value *int_temporary(int);
int param_count(value *);
char *to_string(value *);
value *evaluate(environment *, NODE *, int, int);
value *build_function(environment *, value *, value *);

#endif
