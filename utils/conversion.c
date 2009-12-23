#include "conversion.h"

/**
*	conversion.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	Conversion functions for the --C language
*
*/

/* NODE -> TOKEN CAST */
TOKEN * cast_from_node(NODE *node) {
	return (node==NULL ? NULL : (TOKEN *)node);
}

/* Return type of node */
int type_of(NODE *node) {
	if (node==NULL) {
		return -1;
	}
	return node->type;
}

/* Join the values together - as a parameter list */
value *join(value *val1, value *val2) {
	val1->next = val2;
	return val1;
}


/* Convert from the VT types used in the environment into the parser generated token types */
int vt_type_convert(int type) {
	switch(type) {
		case VT_INTEGR:
			return INT;
		case VT_FUNCTN:
			return FUNCTION;
		case VT_VOID:
			return VOID;
		default:
			return -1;
	}
}

/* Convert int to string */
char *cmm_itoa(int int_val) {
	char *str;
	str = malloc(15 * sizeof(char));
	sprintf(str, "%d", int_val);
	return str;
}


/* ==== C TYPE -> VALUE UTILITIES ==== */

/* Make a string value out of a string */
value *string_value(char *val) {
	static int t_count = 0;
	value *tmp_value = calloc(1, sizeof(value));
	char *temporary_name;
	temporary_name = malloc(sizeof(char) * 15);
	sprintf(temporary_name, "_str%d", t_count);
	tmp_value->identifier = temporary_name;
	tmp_value->value_type = VT_STRING;
	tmp_value->data.string_value = (char *) malloc((sizeof(char) * strlen(val)) + 1);
	strcpy(tmp_value->data.string_value, val);
	t_count++;
	return tmp_value;
}

/* Make an int value out of an int */
value *int_value(int val) {
	static int t_count = 0;
	value *tmp_value = calloc(1, sizeof(value));
	char *temporary_name;
	temporary_name = malloc(sizeof(char) * 15);
	sprintf(temporary_name, "_int%d", t_count);
	tmp_value->identifier = temporary_name;
	tmp_value->value_type = VT_INTEGR;
	tmp_value->data.int_value = val;
	t_count++;
	return tmp_value;
}


/* Make an int value out of an int */
value *untyped_value() {
	static int t_count = 0;
	value *tmp_value = calloc(1, sizeof(value));
	char *temporary_name;
	temporary_name = malloc(sizeof(char) * 15);
	sprintf(temporary_name, "_ut%d", t_count);
	tmp_value->identifier = temporary_name;
	tmp_value->value_type = VT_UNTYPED;
	t_count++;
	return tmp_value;
}

/* Make an int param out of an int and an identifier */
value *int_param(char *identifier, int val) {
	value *tmp_value = calloc(1, sizeof(value));
	tmp_value->identifier = identifier;
	tmp_value->value_type = VT_INTEGR;
	tmp_value->data.int_value = val;
	return tmp_value;
}

/* ==== VALUE -> C TYPE UTILITIES ==== */
char *to_string(value *val) {
	if (val==NULL) return NULL;
	if (val->value_type!=VT_STRING) return NULL;
	return val->data.string_value;
}

int to_int(environment *env, value *val) {
	if (val==NULL) return UNDEFINED;
	if (val->value_type==VT_STRING && env) {
		/* It might be a variable that we should resolve */
		val = search(env, val->data.string_value, VT_INTEGR, VT_ANY, 1);
		if (!val) fatal("Integer value expected");
		return val->data.int_value;
	}
	else if (val->value_type==VT_INTEGR) {
		return val->data.int_value;		
	}
	fatal("Integer value expected");
	return UNDEFINED;
}

/* Return the correct string representation for a variable / string */
char *correct_string_rep(value *val) {
	if (val==NULL) return NULL;
	if (val->temporary) return val->identifier;
	if (val->value_type==VT_STRING) return to_string(val);
	if (val->value_type==VT_INTEGR && val->identifier[0]=='_') return cmm_itoa(to_int(NULL, val));
	return val->identifier;
}
