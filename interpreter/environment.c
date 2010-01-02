#include "environment.h"

/**
*	environment.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	Environment structure utility functions for the --C language
*
*/

/* Create a new environment to store values in */
environment *create_environment(environment *static_link) {
	/* Assign space for environment */
	environment *new_environment = (environment *) calloc(1, sizeof(environment));
	/* Assign space to store values */
	new_environment->values = (value **) calloc(HASH_VALUE_SIZE, sizeof(value *));
	/* Assign static link ptr */
	new_environment->static_link = static_link;
	new_environment->env_size = 0;
	/* Record how nested this environment is */
	if (!static_link) {
		new_environment->nested_level = 0;
	}
	else {
		new_environment->nested_level = static_link->nested_level + 1;
	}
	return new_environment;
}

/* ==== SEARCH Helper Fns ==== */

int matching_names(value *val, char *name) {
	if (!val || !name) return 0;
	return strcmp(val->identifier, name)==0;
}

int matching_types(value *val, int type) {
	if (!val) return 0;
	return type==VT_ANY || val->value_type==type;
}

int matching_return_type(value *val, int type) {
	if (!val) return 0;
	return type==VT_ANY || val->value_type!=VT_FUNCTN || val->data.func->return_type==type;
}

/* ==== End of SEARCH Helper Fns ==== */

/* Retrieve a value of a specific type from the environment */
/* Values which are in the nearest scope will be returned */
value *search(environment *env, char *identifier, int value_type, int return_type, int recursive) {
	/* Find out what position in the hashtable the value should be stored in */
	int hash_position = environment_hash(identifier);
	if (env == NULL) return NULL;
	value *a_value = env->values[hash_position];
	/* Try and find the matching value */
	while (a_value) {
		if (matching_names(a_value, identifier) && matching_types(a_value, value_type) && matching_return_type(a_value, return_type)) {
			return a_value;
		}
		a_value = a_value->next;
	}
	/* Move up to the next environment */
	if (recursive) {
		return get(env->static_link, identifier);	
	}
	return NULL;
}

value *last_if_evaluation(environment *env) {
	return search(env, IF_EVAL_SYMBOL, VT_INTEGR, VT_ANY, 0);
}

/* Retrieve a value of a ANY type from the environment */
/* Values which are in the nearest scope will be returned */
value *get(environment *env, char *identifier) {
	return search(env, identifier, VT_ANY, VT_ANY, 1);
}

char *return_type_as_string(int type) {
	switch(type) {
		case INT:
			return "integer";
		case VOID:
			return "void";
		case FUNCTION:
			return "function";
	}
	return "unknown";
}

/* Recursively print values */
void debug_print_value(value *val) {
	if (DEBUG_ON && val) {
		if (val->temporary) {
			printf("\ttemporary - identifier: %s - var_number: %d\n", val->identifier, val->variable_number);
		}
		else {
			switch(val->value_type) {
				case VT_VOID:
					printf("\tvoid - identifier: %s - var_number: %d\n", val->identifier, val->variable_number);
					break;
				case VT_INTEGR:
					printf("\tint - identifier: %s, value: %d - var_number: %d\n", val->identifier, val->data.int_value, val->variable_number);
					break;
				case VT_STRING:	
					/* A string should never be stored in the hashtable - this language doesn't have strings */
					break;			
				case VT_FUNCTN:			
					printf("\tfunc - identifier: %s, entry-point: %p, returns: %s, definition-env: %p, params: %d\n - var_number: %d\n", val->identifier, val->data.func->node_value, return_type_as_string(val->data.func->return_type), val->data.func->definition_env, param_count(val), val->variable_number);			
					break;			
				case VT_LINKED:		
					break;
			}
		}
		debug_print_value(val->next);
	}
}

/* Register actual parameters in environment */
void define_parameters(environment *env, value *formal, value *actual, environment *search_env) {
	if (param_count(formal) != param_count(actual)) return;
	value *formal_param = formal->data.func->params;
	value *actual_param = actual;	
	value *tmp;
	int dealtWith = 0;
	while(formal_param && actual_param) {
		/* Type check assignment */
		if (actual_param->value_type == VT_STRING) {
			tmp = get(search_env, actual_param->data.string_value);
			if (!tmp) fatal("Could not look-up parameter value");
			type_check_assignment(string_value(formal_param->identifier), tmp, to_int(NULL, formal_param));
			store(env, tmp->value_type, formal_param->identifier, tmp, 1, 1, 0, 0);
		}
		else {
			type_check_assignment(string_value(formal_param->identifier), actual_param, to_int(NULL, formal_param));
			store(env, actual_param->value_type, formal_param->identifier, actual_param, 1, 1, 0, 0);
		}
		formal_param = formal_param->next;
		actual_param = actual_param->next;
	}
}

/* Print environment debugging info */
void debug_environment(environment *env) {
	if (DEBUG_ON && env) {
		int i = 0;
		printf("---------\n");
		printf("Environment: %p\n", env);
		printf("Static link via: %p [Nested %d levels deep]\n", env->static_link, env->nested_level);
		for (i = 0; i < HASH_VALUE_SIZE; i++) {
			if (env->values[i]) {
				printf("[%d]:\n", i);
				debug_print_value(env->values[i]);
			}
		}
	}
}

/* Return the size of the environment */
int env_size(environment *env) {
	int i = 0;
	int temp_size = 0;
	for (i = 0; i < HASH_VALUE_SIZE; i++) {
		if (env->values[i]) {
			value *current = env->values[i];
			while (current) {
				temp_size++;
				current = current->next;
			}
		}
	}
	return temp_size;
}

/* Wrapper to store a function in the environment */
value *store_function(environment *env, value *func, environment *local_env) {
	/* Check we were passed valid data */
	if (!env || !func) return;
	if (func->value_type!=VT_FUNCTN) return;
	func->data.func->local_env = local_env;
	return store(env, VT_FUNCTN, func->identifier, func, 0, 1, 1, 0);
}

/* Store variable in environment */
value *store(environment *env, int value_type, char *identifier, value *val, int is_param, int is_declarator, int is_fn_dec, int is_temporary) {
	value *new_value;
	/* Check entry will be valid */
	if (!identifier || (!val && value_type!=VT_VOID)) return NULL;
	if (val && val->value_type==VT_STRING) return NULL;
	/* Find out what position in the hashtable the value should be stored in */
	int hash_position = environment_hash(identifier);
	/* The environment must be valid */
	if (!env) return NULL;
	/* Check for redefinition */
	if (!is_fn_dec && !is_declarator && value_type!=VT_FUNCTN && !is_param) {
		if (value_type==VT_UNTYPED) value_type = VT_ANY;
		/* Value already exists - overwrite */
		new_value = search(env, identifier, value_type, VT_ANY, 1);
		if (!new_value) {
			fatal("Could not find identifier '%s' with type %d", identifier);
		}
	}
	else if (is_fn_dec && value_type==VT_FUNCTN && search(env, identifier, value_type, VT_ANY, 1) && !is_param) {
		/* Functions may not be redefined if they exist anywhere in the local / global scope */
		/* Only exception is if definition is EXACTLY the same, this is the case if the entry point */
		/* of the two functions is the same. This can happen when we do an initial pre-scan and then scan */
		/* over the whole AST again, for instance in the TAC generator. */
		value * existing_fn = search(env, identifier, value_type, VT_ANY, 1);
		if (existing_fn->data.func->node_value == val->data.func->node_value) {
			/* Return ptr to existing entry */
			return existing_fn;
		}
		fatal("Function '%s' redefines another function. Function redefinition is not allowed!", identifier);
		return NULL;
	}
	else {
		/* If is a declarator, we must NOT redefine an existing variable in the SAME local scope */
		if (!is_param && is_declarator && search(env, identifier, VT_ANY, VT_ANY, 0)) {
			fatal("Variable '%s' redeclares an existing variable in local scope", identifier);
		}
		/* Build new value */
		new_value = (value *) calloc(1, sizeof(value));
		new_value->identifier = malloc((sizeof(char) * strlen(identifier)) + 1);
		strcpy(new_value->identifier, identifier);
		new_value->next = NULL;
		new_value->value_type = value_type;
		new_value->temporary = is_temporary;
		new_value->stored_in_env = env;
		if (is_declarator) {
			new_value->variable_number = env->env_size;
			env->env_size = env->env_size + 1;
		}
		/* Do we have any values in this position of the array? */
		if (!env->values[hash_position]) {
			/* Nothing exists here yet */
			env->values[hash_position] = new_value;
		}
		else {
			/* Value already in this hash position, append to the end */
			value *current_val = env->values[hash_position];
			if (!current_val) fatal("Could not access current value in hash position %d", hash_position);
			for (;;) {
				if (current_val->next==NULL) {
					break;
				}
				current_val = current_val->next;
			}
			current_val->next = new_value;
		}
	}
	/* Assign correct value */
	switch(value_type) {
		case VT_INTEGR:
			new_value->data.int_value = val->data.int_value;
			break;
		case VT_FUNCTN:
			new_value->data.func = val->data.func;
			break;
		case VT_VOID:
		default:
			/* No value */
			break;
	}
	debug_environment(env);
	return new_value;
}

/* ==== HASHTABLE UTILITIES ==== */

/* Find environment hashtable position to store the variable in */
int environment_hash(char *s) {
    int h = 0;
    while (*s != '\0') {
      h = (h<<4) ^ *s++;
    }
    return (0x7fffffff&h) % HASH_VALUE_SIZE;
}

/* Walk down a series of chained values to find the bottom one */
value *find_leaf_value(value *top) {
	if (top==NULL) return NULL;
	if (top->next==NULL) return top;
	return find_leaf_value(top->next);
}