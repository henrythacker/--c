#include "interpreter.h"

/**
*	interpreter.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	Interpreter for the --C language
*
*/

int t_count = 1;

/* Assign variable */
/* Assign data to identifier in env */
void assign(environment *env, value *identifier, value *data) {
	if (env==NULL || identifier==NULL) return;
	store(env, VT_INTEGR, to_string(identifier), data);
}

/* Check whether main entry point exists */
value *main_entry_point(environment *env) {
	return search(env, "main", VT_FUNCTN, INT, 0);
}

/* Execute a function */
value *execute_fn(environment *env, value *fn_reference, int flag) {
	NODE *node;
	environment *definition_env;
	node = fn_reference->data.func->node_value;
	definition_env = fn_reference->data.func->definition_env;
	evaluate(definition_env, node, flag);
}

/* Build a new function definition - don't store in environment yet though */
/* This data structure is further embellished within the recursive evaluate fn */
value *build_function(environment *env, value *fn_name, value *param_list) {
	value *tmp_value = calloc(1, sizeof(value));
	char *identifier = to_string(fn_name);
	function_declaration *fn_value = calloc(1, sizeof(function_declaration));	
	tmp_value->value_type = VT_FUNCTN;
	tmp_value->identifier = malloc((sizeof(char) * strlen(identifier)) + 1);
	strcpy(tmp_value->identifier, identifier);
	fn_value->params = param_list;
	fn_value->definition_env = env;
	tmp_value->data.func = fn_value;
	return tmp_value;
}

/* Store a function in the environment */
void store_function(environment *env, value *func) {
	/* Check we were passed valid data */
	if (!env || !func) return;
	if (func->value_type!=VT_FUNCTN) return;
	store(env, VT_FUNCTN, func->identifier, func);
}

/* Recursive evaluation of AST */
value *evaluate(environment *env, NODE *node, int flag) {
	value *lhs, *rhs, *temp;
	environment *new_env;
	/* Check if we were passed an invalid node */
	if (!node) return NULL;
	switch(type_of(node)) {
		case '=':
			lhs = evaluate(env, node->left, flag);
			rhs = evaluate(env, node->right, flag);
			assign(env, lhs, rhs);
			return NULL;
		case IDENTIFIER:
			return string_temporary(cast_from_node(node)->lexeme);
		case CONSTANT:
			return int_temporary(cast_from_node(node)->value);
		case VOID:
		case FUNCTION:
		case INT:
			return int_temporary(type_of(node));
		case LEAF:
			return evaluate(env, node->left, flag);			
		case 'D':
			new_env = create_environment(env);
			/* LHS is FN definition */
			/* LHS is executed in current environment */
			lhs = evaluate(env, node->left, flag);
			if (flag==INTERPRET_FULL) {
				/* RHS becomes evaluated fn body */
				/* Executed in new environment */
				rhs = evaluate(new_env, node->right, flag);	
			}
			else {
				rhs = NULL;
			}
			if (lhs!=NULL) {
				/* Point function to the correct fn body */
 				lhs->data.func->node_value = node->right;
				/* Store function definition in environment */
				store_function(env, lhs);
			}
			return rhs;
		case 'd':
			/* LHS is the type */
			lhs = evaluate(env, node->left, flag);
			/* RHS is fn name & params */
			rhs = evaluate(env, node->right, flag);
			/* Store return type */
			rhs->data.func->return_type = to_int(lhs);
			return rhs;
		case 'F':
			/* FN name in LHS */
			lhs = evaluate(env, node->left, flag);
			/* Pull our parameters */
			rhs = evaluate(env, node->right, INTERPRET_PARAMS);
			return build_function(env, lhs, rhs);
		case RETURN:
			lhs = evaluate(env, node->left, flag);
			if (lhs!=NULL && lhs->value_type!=VT_INTEGR) {
				return get(env, to_string(lhs));
			}
			return lhs;
		case '~':
			lhs = evaluate(env, node->left, flag);
			rhs = evaluate(env, node->right, flag);
			return rhs;
		case ';':
			lhs = NULL;
			rhs = NULL;
			if (node->left!=NULL) lhs = evaluate(env, node->left, flag);
			if (lhs==NULL) {
				if (node->right!=NULL) rhs = evaluate(env, node->right, flag);			
				if (rhs==NULL) {
					return NULL;
				}
				else {
					return rhs;
				}
			}
			else {
				return lhs;
			}
		default:
			printf("Unrecognised token type: %d\n", type_of(node));
			return NULL;
	}
}

/* Make a string temporary */
value *string_temporary(char *val) {
	value *tmp_value = calloc(1, sizeof(value));
	char temporary_name[10];
	sprintf(temporary_name, "t%d", t_count);
	tmp_value->identifier = temporary_name;
	tmp_value->value_type = VT_STRING;
	tmp_value->data.string_value = (char *) malloc((sizeof(char) * strlen(val)) + 1);
	strcpy(tmp_value->data.string_value, val);
	t_count++;
	return tmp_value;
}

value *int_temporary(int val) {
	value *tmp_value = calloc(1, sizeof(value));
	char temporary_name[10];
	sprintf(temporary_name, "t%d", t_count);
	tmp_value->identifier = temporary_name;
	tmp_value->value_type = VT_INTEGR;
	tmp_value->data.int_value = val;
	t_count++;
	return tmp_value;
}

/* Start the interpretation process at the top of the AST */
void start_interpret(NODE *start) {
	initial_environment = create_environment(NULL);
	evaluate(initial_environment, start, INTERPRET_FN_SCAN);
	debug("Function scan complete.");
	if (main_entry_point(initial_environment)) {
		value *return_value;
		debug("Entry point - int main() found");		
		debug("Starting full interpretation...");
		return_value = execute_fn(initial_environment, main_entry_point(initial_environment), INTERPRET_FULL);
		print_return_value(return_value);
	}
	else {
		fatal("Entry point - int main() NOT found!");
	}
}