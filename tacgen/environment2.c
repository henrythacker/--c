#include "environment2.h"

value *register_temporary(environment *env, char *temp_name, value *null_value) {
	value *reference = NULL;
	if (!null_value) {
		reference = store(env, VT_VOID, temp_name, NULL, 0, 1, 0, 1);
	}
	else {
		reference = store(env, null_value->value_type, temp_name, null_value, 0, 1, 0, 1);
	}
	assert(reference!=NULL, "Could not register temporary");
	return reference;
}