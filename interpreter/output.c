#include "output.h"

/**
*	output.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	System out functions for the --C language
*
*/

/* Fatal error */
void fatal(char *str) {
	printf("FATAL: %s\n", str);
	exit(-1);
}

/* Print out debug info */
void debug(char *str) {
	if (DEBUG_ON) {
		printf("%s\n", str);
	}
}

/* Print return value */
void print_return_value(environment *env, value *val) {
	if (!val) return;
	switch(val->value_type) {
		case VT_INTEGR:
			if (to_int(env, val) != UNDEFINED) printf("Result: %d\n", to_int(env, val));
			return;
		case VT_FUNCTN:
			printf("Result: %p\n", val->data.func->node_value);
			return;
		default:
			return;
	}
}