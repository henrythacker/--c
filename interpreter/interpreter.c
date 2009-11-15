#include "interpreter.h"

/**
*	interpreter.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	Interpreter for the --C language
*
*/

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
	if (!fn_reference) {
		fatal("Could not find function!");
	}
	else {
		environment *new_env;
		node = fn_reference->data.func->node_value;
		definition_env = fn_reference->data.func->definition_env;
		new_env = create_environment(definition_env);
		return evaluate(new_env, node, flag);
	}
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

/* Recursive evaluation of AST */
value *evaluate(environment *env, NODE *node, int flag) {
	value *lhs, *rhs, *temp;
	environment *new_env;
	/* Check if we were passed an invalid node */
	if (!node) return NULL;
	switch(type_of(node)) {
		case '+':
		case '-':
		case '*':
		case '/':
		case '%':
		case '>':
		case '<':
		case NE_OP:
		case EQ_OP:
		case LE_OP:
		case GE_OP:
		case '!':
			lhs = evaluate(env, node->left, flag);
			rhs = evaluate(env, node->right, flag);
			return arithmetic(env, type_of(node), lhs, rhs);
		case '=':
			lhs = evaluate(env, node->left, flag);
			rhs = evaluate(env, node->right, flag);
			if (rhs && rhs->value_type!=VT_INTEGR) {
				if (rhs->value_type == VT_STRING) 
					rhs = get(env, rhs->data.string_value);
				else
					rhs = get(env, rhs->identifier);
			}
			assign(env, lhs, rhs);
			return NULL;
		case APPLY:
			/* FN Name */
			lhs = evaluate(env, node->left, flag);
			/* Params */
			rhs = evaluate(env, node->right, flag);
			/* Lookup function */
			temp = search(env, to_string(lhs), VT_FUNCTN, VT_ANY, 1);
			return execute_fn(env, temp, flag);		
		case IDENTIFIER:
			return string_value(cast_from_node(node)->lexeme);
		case CONSTANT:
			return int_value(cast_from_node(node)->value);
		case VOID:
		case FUNCTION:
		case INT:
			return int_value(type_of(node));
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
			rhs->data.func->return_type = to_int(env, lhs);
			return rhs;
		case 'F':
			/* FN name in LHS */
			lhs = evaluate(env, node->left, flag);
			/* Pull our parameters */
			rhs = evaluate(env, node->right, INTERPRET_PARAMS);
			return build_function(env, lhs, rhs);
		case RETURN:
			lhs = evaluate(env, node->left, flag);
			/* Provide lookup for non-constants */
			if (lhs && lhs->value_type!=VT_INTEGR) {
				if (lhs->value_type == VT_STRING) 
					lhs = get(env, lhs->data.string_value);
				else
					lhs = get(env, lhs->identifier);
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
		print_return_value(initial_environment, return_value);
	}
	else {
		fatal("Entry point - int main() NOT found!");
	}
}