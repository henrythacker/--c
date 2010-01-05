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
#define VT_ANY INT_MIN + 6

/* Temporary that cannot be EASILY type-checked at runtime */
#define VT_UNTYPED INT_MIN + 7

/* Special value which stores last if evaluation in environment */
#define IF_EVAL_SYMBOL "$IF"
#define CONTINUE_EVAL_SYMBOL "$CONTINUE"
#define BREAK_EVAL_SYMBOL "$BREAK"

/* Value structure */
typedef struct value {
	char *identifier;
	struct value *next;
	int value_type;
	int temporary;
	int variable_number;
	struct environment *stored_in_env;
	union {
		char *string_value;
		int int_value;
		struct value *linked_value;
		struct function_declaration *func;
	}data;
} value;

/* Environment structure */
typedef struct environment {
	int env_size;
	struct value **values;
	struct environment *static_link;
	int nested_level;
} environment;

/* Function declaration struct */
typedef struct function_declaration {
	int return_type;
	struct value *params;
	struct environment *definition_env;
	struct environment *local_env;	
	NODE *node_value; /* Function entry point */
} function_declaration;

/* Fn prototypes */
int env_size(environment *env);
environment *create_environment(environment *);
value *find_leaf_value(value *);
value *get(environment *, char *);
extern value *string_value(char *);
void debug_print_value(value *);
value *last_if_evaluation(environment *);
void define_parameters(environment *, value *, value *, environment *);
value *store(environment *, int, char *, value *, int, int, int, int);
value *search(environment *, char *, int, int, int);
value *store_function(environment *, value *, environment *);
int environment_hash(char *);

#endif