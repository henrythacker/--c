#include "conversion.h"

/**
*	conversion.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	Conversion functions for the --C language
*
*/

/* ==== VALUE -> C TYPE UTILITIES ==== */
char *to_string(value *val) {
	if (val==NULL) return NULL;
	if (val->value_type!=VT_STRING) return NULL;
	return val->data.string_value;
}

int to_int(value *val) {
	if (val==NULL) return UNDEFINED;
	if (val->value_type!=VT_INTEGR) return UNDEFINED;
	return val->data.int_value;
}

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