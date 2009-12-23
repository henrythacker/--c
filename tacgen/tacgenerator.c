#include "tacgenerator.h"

/* Generate a new temporary */
value *generate_temporary(environment *env, value *null_value) {
	char *tmp;
	static int temp_count = 0;
	tmp = malloc(sizeof(char) * 15);
	sprintf(tmp, "t%d", ++temp_count);
	return register_temporary(env, tmp, null_value);
}

/* Add TAC quad onto end of generated code */
void append_code(tac_quad *quad) {
	if (tac_output == NULL) {
		tac_output = quad;
	}
	else {
		tac_quad *tmp = tac_output;
		while(1) {
			if (tmp->next==NULL) break;
			tmp = tmp->next;
		}
		tmp->next = quad;
	}
}

/* TAC to stdout */
void print_tac(tac_quad *quad) {
	if (quad==NULL) return;
	switch(quad->type) {
		case TT_LABEL:
			printf("%s:\n", to_string(quad->operand1));
			break;
		case TT_FN_DEF:
			printf("_%s:\nBeginFn %d\n", to_string(quad->operand1), param_count(quad->operand1));
			break;	
		case TT_FN_CALL:
			printf("%s = CallFn _%s\n", correct_string_rep(quad->result), to_string(quad->operand1));
			break;			
		case TT_POP_PARAM:
			printf("PopParam %s\n", correct_string_rep(quad->operand1));
			break;
		case TT_PUSH_PARAM:
			printf("PushParam %s\n", correct_string_rep(quad->operand1));
			break;
		case TT_IF:
			printf("if %s goto %s\n", correct_string_rep(quad->operand1), correct_string_rep(quad->result));
			break;			
		case TT_ASSIGN:
			printf("%s %s %s\n", correct_string_rep(quad->result), quad->op, correct_string_rep(quad->operand1));
			break;			
		case TT_GOTO:
			printf("goto %s\n", correct_string_rep(quad->operand1));
			break;
		case TT_OP:
			printf("%s = %s %s %s\n", correct_string_rep(quad->result), correct_string_rep(quad->operand1), quad->op, correct_string_rep(quad->operand2));
			break;
		case TT_RETURN:
			if (quad->operand1) {
				printf("Return %s\n", correct_string_rep(quad->operand1));
			}
			else {
				printf("Return");
			}
			break;
		case TT_BEGIN_FN:
			printf("BeginFn %d\n", param_count(quad->operand1));
			break;
		case TT_END_FN:
			printf("EndFn\n");
			break;			
		default:
			fatal("Unknown TAC Quad type '%d'", quad->type);
	}
	print_tac(quad->next);
}

/* Make a TAC quad */
tac_quad *make_quad_value(char *op, value *operand1, value *operand2, value *result, int type) {
	tac_quad *tmp_quad = (tac_quad *)malloc(sizeof(tac_quad));
	tmp_quad->op = (char *)malloc(sizeof(char) * (strlen(op) + 1));
	tmp_quad->operand1 = operand1;
	tmp_quad->operand2 = operand2;
	tmp_quad->result = result;
	tmp_quad->type = type;
	strcpy(tmp_quad->op, op);
	return tmp_quad;
}

/* Convert operator tokens into TAC operator string equivalents */
char *type_to_string(int type) {
	char *tmp_type;
	switch(type) {
		case NE_OP:
			return "!=";
		case EQ_OP:
			return "==";
		case LE_OP:
			return "<=";		
		case GE_OP:
			return ">=";
		default:
			tmp_type = malloc(sizeof(char) * 3);
			sprintf(tmp_type, "%c", type);
			return tmp_type;
	}
}

/* Build the correct code in the correct place for the else part */
void build_else_part(environment *env, NODE *node, int true_part, int flag, int return_type) {
	if (node==NULL || type_of(node)!=ELSE) return;
	if (true_part) {
		make_simple(env, node->left, flag, return_type);
	}
	else {
		make_simple(env, node->right, flag, return_type);		
	}
}

/* Generate jump label with given name */
tac_quad *make_label(char *label_name) {
	return make_quad_value("", string_value(label_name), NULL, NULL, TT_LABEL);
}

/* Generate IF statement from given condition and true jump label */
tac_quad *make_if(value *condition, char *true_label) {
	return make_quad_value("", condition, NULL, string_value(true_label), TT_IF);
}

/* Generate IF statement from given condition and true jump label */
tac_quad *make_goto(char *label_name) {
	return make_quad_value("", string_value(label_name), NULL, NULL, TT_GOTO);
}

/* Generate RETURN statement with given return value */
tac_quad *make_return(value *return_value) {
	return make_quad_value("", return_value, NULL, NULL, TT_RETURN);
}

/* Generate an END_FN statement */
tac_quad *make_end_fn() {
	return make_quad_value("", NULL, NULL, NULL, TT_END_FN);
}

/* Generate an BEGIN_FN statement */
tac_quad *make_begin_fn(value *fn_def) {
	return make_quad_value("", fn_def, NULL, NULL, TT_BEGIN_FN);
}

/* Generate FN Definition label */
tac_quad *make_fn_def(value *fn_def) {
	return make_quad_value("", string_value(fn_def->identifier), NULL, NULL, TT_FN_DEF);
}

/* Generate FN call */
tac_quad *make_fn_call(value *result, value *fn_def) {
	return make_quad_value("", fn_def, NULL, result, TT_FN_CALL);
}

/* Build necessary code for an if statement */
void build_if_stmt(environment *env, NODE *node, int if_count, tac_quad *end_jump, int flag, int return_type) {
	char *s_tmp;
	value *val1, *val2, *temporary;
	if (node==NULL || (type_of(node)!=IF && type_of(node)!=WHILE)) return;
	/* LHS is condition */
	val1 = make_simple(env, node->left, flag, return_type);
	
	/* Generate if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$if%dtrue", if_count);
	append_code(make_if(val1, s_tmp));

	/* Output false branch (i.e. else part) */	
	if (type_of(node->right)==ELSE) {
		/* Build code for false part */
		build_else_part(env, node->right, 0, flag, return_type);
	}
	
	/* Generate goto end of if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$if%dend", if_count);
	append_code(make_goto(s_tmp));
	
	/* Generate label for start of true branch */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$if%dtrue", if_count);
	append_code(make_label(s_tmp));
	
	/* Output true branch */
	if (type_of(node->right)==ELSE) {
		/* Build code for true part */
		build_else_part(env, node->right, 1, flag, return_type);
	}
	else {
		/* True part is whole right branch */
		make_simple(env, node->right, flag, return_type);
	}
	
	/* Check if extra loop jump has been specified (for WHILE loops etc) */
	if (end_jump) {
		append_code(end_jump);
	}
	
	/* Generate end of IF stmt label */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$if%dend", if_count);
	append_code(make_label(s_tmp));
}

/* Build necessary code for a while statement */
void build_while_stmt(environment *env, NODE *node, int while_count, int if_count, int flag, int return_type) {
	char *s_tmp, *val1, *val2, *temporary;
	tac_quad *loop_jmp;
	if (node==NULL || type_of(node)!=WHILE) return;
	
	/* Generate label for start of while loop */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$while%d", while_count);
	append_code(make_label(s_tmp));
	
	/* Generate loop jump */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$while%d", while_count);
	loop_jmp = make_goto(s_tmp);
	
	/* Build IF stmt for condition */
	build_if_stmt(env, node, if_count, loop_jmp, flag, return_type);
	
	/* End while loop stmt */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "$while%dend", while_count);
	append_code(make_label(s_tmp));
	
}

/* Register params in environment */
void register_params(environment *env, value *param_list) {
	if (!param_list) return;
	value *current_param = param_list;
	while (current_param!=NULL) {
		value *param = NULL;
		value *param_name = string_value(current_param->identifier);
		switch(current_param->value_type) {	
			case VT_INTEGR:
				param = assign(env, param_name, int_value(0), 1);
				break;
			case VT_VOID:	
				param = assign(env, param_name, void_value(), 1);						
				break;
			case VT_FUNCTN:
				param = assign(env, param_name, null_fn, 1);
				break;
			default:
				fatal("Could not determine parameter type!");
		}
		append_code(make_quad_value("", param, NULL, NULL, TT_POP_PARAM));
		current_param = current_param->next;
	}
}

/* Push params on param stack in reverse order (recursively) */
tac_quad *push_params(value *params_head) {
	if (!params_head) return NULL;
	if (params_head->next) {
		append_code(push_params(params_head->next));
	}
	return make_quad_value("", params_head, NULL, NULL, TT_PUSH_PARAM);		
}

/* 
 * Make the given NODE simple - i.e. return a temporary for complex subtrees 
 * The appropriate code is also generated and pushed onto the code stack
*/
value *make_simple(environment *env, NODE *node, int flag, int return_type) {
	int i_value = 0;
	char *s_tmp;
	value *val1, *val2, *temporary, *temp;
	static int if_count = 0;
	static int while_count = 0;	
	environment *new_env;
	if (node==NULL) return NULL;
	switch(type_of(node)) {
		case LEAF: 
			return make_simple(env, node->left, flag, return_type);
		case CONSTANT:
			i_value = cast_from_node(node)->value;
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "%d", i_value);
			return int_value(i_value);
		case IDENTIFIER:
			return string_value(cast_from_node(node)->lexeme);
		case IF:
			build_if_stmt(env, node, ++if_count, NULL, flag, return_type);
			return NULL;
		case BREAK:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "$while%dend", while_count);
			append_code(make_label(s_tmp));
			return NULL;			
		case CONTINUE:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "$while%d", while_count);
			append_code(make_label(s_tmp));
			return NULL;
		case WHILE:
			new_env = create_environment(env);
			build_while_stmt(new_env, node, ++while_count, ++if_count, flag, return_type);
			return NULL;	
		case '=':
			val1 = make_simple(env, node->left, flag, return_type);
			val2 = make_simple(env, node->right, flag, return_type);
			if (val2 && val2->value_type!=VT_INTEGR && val2->value_type!=VT_FUNCTN) {
				if (val2->value_type == VT_STRING) {
					val2 = get(env, val2->data.string_value);
				}
				else {
					val2 = get(env, val2->identifier);
				}
				if (!val2) fatal("Undeclared identifier");					
			}
			/* Check the LHS variable has already been defined */
			temp = get(env, to_string(val1));
			assert(temp!=NULL, "Variable not defined");
			/* Type check the assignment */
			type_check_assignment(val1, val2, vt_type_convert(temp->value_type));
			assign(env, val1, val2, 0);
			append_code(make_quad_value("=", val2, NULL, val1, TT_ASSIGN));
			return NULL;
		case '*':
		case '/':
		case '>':
		case '<':										
		case '%':							
		case '-':									
		case '+':
		case NE_OP:
		case LE_OP:
		case GE_OP:				
		case EQ_OP:
			temporary = generate_temporary(env, int_value(0));
			val1 = make_simple(env, node->left, flag, return_type);
			val2 = make_simple(env, node->right, flag, return_type);
			append_code(make_quad_value(type_to_string(type_of(node)), val1, val2, temporary, TT_OP));
			return temporary;
		case '~':
			if (flag != INTERPRET_PARAMS) {
				/* Params should not be registered, because at this point we're not in the correct environment */
				register_variable_subtree(env, node, VT_ANY);
			}
			val1 = make_simple(env, node->left, flag, return_type);
			val2 = make_simple(env, node->right, flag, return_type);
			if (flag == INTERPRET_PARAMS) {
				return int_param(to_string(val2), to_int(env, val1));
			}			
			return NULL;
		case 'D':

			/* val1 is FN definition */
			/* val1 is executed in current environment */
			val1 = make_simple(env, node->left, flag, return_type);
			
			/* If this is an embedded function, generate a goto to the end of the fn def */
			/* Otherwise, we will inadvertendly attempt to execute the inner fn */
			s_tmp = malloc(sizeof(char) * (strlen(val1->identifier) + 2));
			sprintf(s_tmp, "~%s", val1->identifier);
			if (flag==EMBEDDED_FNS) {
				append_code(make_goto(s_tmp));
			}
			
			if (val1!=NULL) {
				/* Point function to the correct fn body */
				val1->data.func->node_value = node->right;
				/* Store function definition in environment */
				val2 = store_function(env, val1);
			}
			/* Write out FN Name label */
			append_code(make_fn_def(val2));
			/* Look inside body, but in new environment */
			new_env = create_environment(env);
			/* Define parameters with default empty values */
			register_params(new_env, val2->data.func->params);
			/* Look inside fn body */
			val2 = make_simple(new_env, node->right, EMBEDDED_FNS, val1->data.func->return_type);
			/* Write end of function marker */
			append_code(make_end_fn());
			/* Write end of function label if embedded fn */
			if (flag==EMBEDDED_FNS) {
				append_code(make_label(s_tmp));
			}
			return NULL;
		case 'd':
			/* val1 is the type */
			val1 = make_simple(env, node->left, flag, return_type);
			/* val2 is fn name & params */
			val2 = make_simple(env, node->right, flag, return_type);
			/* Store return type */
			val2->data.func->return_type = to_int(env, val1);
			return val2;
		case 'F':
			/* FN name in val1 */
			val1 = make_simple(env, node->left, flag, return_type);
			/* Pull our parameters */
			val2 = make_simple(env, node->right, INTERPRET_PARAMS, return_type);
			return build_function(env, val1, val2);
		case RETURN:
			val1 = make_simple(env, node->left, flag, return_type);
			/* Provide lookup for non-constants */
			if (val1 && val1->value_type!=VT_INTEGR) {
				if (val1->value_type == VT_STRING) {
					val1 = get(env, val1->data.string_value);
				}
				else {
					val1 = get(env, val1->identifier);
				}
				if (!val1) fatal("Undeclared identifier");
			}
			type_check_return(val1, return_type);
			append_code(make_return(val1));
			return NULL;
		case ',':
			val1 = make_simple(env, node->left, flag, return_type);
			val2 = make_simple(env, node->right, flag, return_type);
			if (val1 && val2) {
				return join(val1, val2);
			}
			return NULL;	
		case APPLY:
			/* FN Name */
			val1 = make_simple(env, node->left, flag, return_type);
			/* Params */
			val2 = make_simple(env, node->right, flag, return_type);
			append_code(push_params(val2));
			/* Lookup function */
			temp = search(env, to_string(val1), VT_FUNCTN, VT_ANY, 1);
			if (temp) {
				int fn_return_type = temp->data.func->return_type;
				/* Temporary for result (if any) */
				switch(fn_return_type) {
					case INT:
						temporary = generate_temporary(env, int_value(0));
						break;
					case VOID:
						temporary = generate_temporary(env, NULL);
						break;						
					case FUNCTION:
						temporary = generate_temporary(env, null_fn);
						break;
					default:
						fatal("Unknown Return Type %d", fn_return_type);
						return NULL;
				}
				append_code(make_fn_call(temporary, val1));
			}
			return temporary;
		case FUNCTION:
		case INT:
		case VOID:
			return int_value(type_of(node));
		case ';':
			make_simple(env, node->left, flag, return_type);
			make_simple(env, node->right, flag, return_type);			
			return NULL;
		default:
			fatal("Unrecognised node type");
			return NULL;
	}
	
}


/* Start the TAC generator process at the top of the AST */
void start_tac_gen(NODE *tree) {
	null_fn = build_null_function();
	environment *default_env = create_environment(NULL);
	make_simple(default_env, tree, 0, 0);
	print_tac(tac_output);
}