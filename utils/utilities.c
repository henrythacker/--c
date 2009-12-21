#include "utilities.h"

/* Assertion with error text */
void assert(int assertion, char *error) {
	if (!assertion) {
		printf("TAC Error: %s\n", error);
		exit(-1);
	}
}

/* Initialise a void placeholder value */
value *void_value(void) {
	static int t_count = 0;
	value *tmp_value = calloc(1, sizeof(value));
	char temporary_name[10];
	sprintf(temporary_name, "void%d", t_count);
	tmp_value->identifier = temporary_name;
	tmp_value->value_type = VT_VOID;
	t_count++;
	return tmp_value;
}

/* Check that the value can be returned by the function with a given declared return type */
void type_check_return(value *returned_value, int declared_return_type) {
	if (!returned_value && declared_return_type!=VOID) fatal("Expected a return value!");
	if (declared_return_type == INT) {
		if (returned_value->value_type != VT_INTEGR) {
			fatal("Expected integer return value");	
		}
	}
	if (declared_return_type == FUNCTION) {
		if (returned_value->value_type != VT_FUNCTN) {
			fatal("Expected function return value");	
		}
	}
}


/* Check that a variable with value: variable_value matches up with the declared type */
void type_check_assignment(value *variable_name, value *variable_value, int declared_type) {
	if (!variable_name || !variable_value) fatal("Not enough information to typecheck statement!");
	/* VOID variables can never be assigned to! */
	if (declared_type == VOID) {
		fatal("Variable '%s' of type \"void\" can not be assigned a value", to_string(variable_name));	
	}
	if (declared_type == INT) {
		if (variable_value->value_type != VT_INTEGR) {
			fatal("Variable '%s' of type \"int\" can not be assigned a non-integer value", to_string(variable_name));	
		}
	}
	if (declared_type == FUNCTION) {
		if (variable_value->value_type != VT_FUNCTN) {
			fatal("Variable '%s' of type \"function\" can only point to functions", to_string(variable_name));
		}
	}
}