#include "output.h"

/**
*	output.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	System out functions for the --C language
*
*/

/* Fatal error */
void fatal(char *str, ...) {
	va_list arglist;
	char *str_arg;
	char *current_char;
	int i;
	va_start(arglist, str);
	printf("Fatal exception: ");
	/* Iterate through our format string */
	for (current_char = str; *current_char!='\0'; current_char++) {
		/* Check if a format specifier is coming up? */
		if (*current_char != '%') {
			printf("%c", *current_char);
			/* Move to the next char */
			continue;
		}
		current_char++;
		switch(*current_char) {
			/* What type of format specifier do we have? */
			case 'c':
				/* Pull out char as int */
				i = va_arg(arglist, int);
				printf("%c", i);
				break;
			case 'd':
				/* Pull out an int */
				i = va_arg(arglist, int);
				printf("%d", i);
				break;
			case 's':
				/* Pull out a string */
				str_arg = va_arg(arglist, char *);
				printf("%s", str_arg);
				break;
			case '%':
				/* Allow percentage sign to be escaped */
				printf("%%");
				break;
		}
		va_end(arglist);
	}
	printf("\n");
	/* Fatal error, so exit with error code */
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