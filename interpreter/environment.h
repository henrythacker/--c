#ifndef __ENV_H
#define __ENV_H

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include "nodes.h"
#include "token.h"
#include "C.tab.h"

#define HASH_VALUE_SIZE 1000

#define DEBUG_ON 1

/* Value type - Integer */
#define VT_INTEGR INT_MIN + 1
/* Value type - String (for identifiers) */
#define VT_STRING INT_MIN + 2
/* Value type - Function ptr */
#define VT_FUNCTN INT_MIN + 3
/* Value type - linked values - for param lists, etc */
#define VT_LINKED INT_MIN + 4
/* Value type - void */
#define VT_VOID INT_MIN + 5

/* Any type - special type used in searches */
#define VT_ANY INT_MIN + 5

/* Special value which stores last if evaluation in environment */
#define IF_EVAL_SYMBOL "$IF"
#define CONTINUE_EVAL_SYMBOL "$CONTINUE"
#define BREAK_EVAL_SYMBOL "$BREAK"

/* Value structure */
typedef struct value {
	char *identifier;
	struct value *next;
	int value_type;
	union {
		char *string_value;
		int int_value;
		struct value *linked_value;
		struct function_declaration *func;
	}data;
} value;

/* Environment structure */
typedef struct environment {
	struct value **values;
	struct environment *static_link;
} environment;

/* Function declaration struct */
typedef struct function_declaration {
	int return_type;
	struct value *params;
	struct environment *definition_env;
	NODE *node_value; /* Function entry point */
} function_declaration;

/* Fn prototypes */
environment *create_environment(environment *);
value *find_leaf_value(value *);
value *get(environment *, char *);
value *last_if_evaluation(environment *);
void store(environment *, int, char *, value *);
value *search(environment *, char *, int, int, int);
void store_function(environment *, value *);
int environment_hash(char *);

#endif