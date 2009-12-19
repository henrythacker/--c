#include "environment2.h"

void register_temporary(environment *env, char *temp_name) {
	value *reference = NULL;
	reference = store(env, VT_INTEGR, temp_name, int_value(0), 0);
	assert(reference!=NULL, "Could not register temporary");
}