#include "environment2.h"

value *register_temporary(environment *env, char *temp_name) {
	value *reference = NULL;
	reference = store(env, VT_TEMPORARY, temp_name, int_value(0), 0, 1, 0);
	assert(reference!=NULL, "Could not register temporary");
	return reference;
}