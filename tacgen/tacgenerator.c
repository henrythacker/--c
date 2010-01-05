#include "tacgenerator.h"

/* Register temporary in the specified environment */
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

/* Generate a new temporary */
value *generate_temporary(environment *env, value *null_value) {
	char *tmp;
	static int temp_count = 0;
	tmp = malloc(sizeof(char) * 15);
	sprintf(tmp, "_t%d", ++temp_count);
	return register_temporary(env, tmp, null_value);
}

value *generate_untypechecked_temporary(environment *env) {
	return generate_temporary(env, untyped_value());
}

/* TAC to stdout */
void print_tac(tac_quad *quad) {
	if (quad==NULL) return;
	switch(quad->type) {
		case TT_LABEL:
			printf("%s:\n", to_string(quad->operand1));
			break;
		case TT_FN_DEF:
			printf("_%s:\n", to_string(quad->operand1));
			break;	
		case TT_FN_CALL:
			printf("%s = CallFn _%s\n", correct_string_rep(quad->result), correct_string_rep(quad->operand1));
			break;			
		case TT_INIT_FRAME:
			printf("InitFrame %s\n", correct_string_rep(quad->operand1));
			break;
		case TT_POP_PARAM:
     		printf("PopParam %s\n", quad->operand1->identifier);
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
		case TT_PREPARE:
			printf("PrepareToCall %d\n", param_count(quad->operand1));
			break;	
		case TT_BEGIN_FN:
			printf("BeginFn %s\n", correct_string_rep(quad->operand1));
			break;
		case TT_FN_BODY:
			printf("FnBody\n");	
			break;
		case TT_END_FN:
			printf("EndFn\n");
			break;			
		default:
			fatal("Unknown TAC Quad type '%d'", quad->type);
	}
	print_tac(quad->next);
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

/* Make a TAC quad */
tac_quad *make_quad_value(char *op, value *operand1, value *operand2, value *result, int type, int subtype) {
	tac_quad *tmp_quad = (tac_quad *)malloc(sizeof(tac_quad));
	tmp_quad->op = (char *)malloc(sizeof(char) * (strlen(op) + 1));
	tmp_quad->operand1 = operand1;
	tmp_quad->operand2 = operand2;
	tmp_quad->result = result;
	tmp_quad->type = type;
	tmp_quad->subtype = subtype;
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
tac_quad *prepare_fn(value *func) {
	return make_quad_value("", func, NULL, NULL, TT_PREPARE, 0);
}

/* Generate jump label with given name */
tac_quad *make_label(char *label_name) {
	return make_quad_value("", string_value(label_name), NULL, NULL, TT_LABEL, 0);
}

/* Generate IF statement from given condition and true jump label */
tac_quad *make_if(value *condition, char *true_label) {
	return make_quad_value("", condition, NULL, string_value(true_label), TT_IF, 0);
}

/* Generate IF statement from given condition and true jump label */
tac_quad *make_goto(char *label_name) {
	return make_quad_value("", string_value(label_name), NULL, NULL, TT_GOTO, 0);
}

/* Generate RETURN statement with given return value */
tac_quad *make_return(value *return_value) {
	return make_quad_value("", return_value, NULL, NULL, TT_RETURN, 0);
}

/* Generate an END_FN statement */
tac_quad *make_end_fn(value *fn_def) {
	return make_quad_value("", fn_def, NULL, NULL, TT_END_FN, 0);
}

/* Generate an INIT_FRAME statement */
tac_quad *make_init_frame() {
	return make_quad_value("", int_value(0), NULL, NULL, TT_INIT_FRAME, 0);
}

/* Generate an BEGIN_FN statement */
tac_quad *make_begin_fn(value *fn_def) {
	return make_quad_value("", fn_def, NULL, NULL, TT_BEGIN_FN, 0);
}

/* Generate FN Definition label */
tac_quad *make_fn_def(value *fn_def) {
	return make_quad_value("", string_value(fn_def->identifier), NULL, NULL, TT_FN_DEF, 0);
}

/* Generate FN call */
tac_quad *make_fn_call(value *result, value *fn_def) {
	return make_quad_value("", fn_def, NULL, result, TT_FN_CALL, 0);
}

/* Generate FN body */
tac_quad *make_fn_body(value *fn_def) {
	return make_quad_value("", fn_def, NULL, NULL, TT_FN_BODY, 0);
}

/* Build necessary code for an if statement */
void build_if_stmt(environment *env, NODE *node, int if_count, tac_quad *false_jump, tac_quad *loop_jump, int flag, int return_type) {
	char *s_tmp;
	value *val1, *val2, *temporary;
	if (node==NULL || (type_of(node)!=IF && type_of(node)!=WHILE)) return;
	/* LHS is condition */
	val1 = make_simple(env, node->left, flag, return_type);
	
	/* Generate if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "__if%dtrue", if_count);
	append_code(make_if(val1, s_tmp));

	/* Output false branch (i.e. else part) */	
	if (type_of(node->right)==ELSE) {
		/* Build code for false part */
		build_else_part(env, node->right, 0, flag, return_type);
	}
	
	/* Generate goto end of if statement */
	if (false_jump != NULL) {
		append_code(false_jump);
	}
	else {
		s_tmp = malloc(sizeof(char) * 25);
		sprintf(s_tmp, "__if%dend", if_count);
		append_code(make_goto(s_tmp));
	}
	
	/* Generate label for start of true branch */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "__if%dtrue", if_count);
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
	if (loop_jump) {
		append_code(loop_jump);
	}
	
	/* Generate end of IF stmt label */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "__if%dend", if_count);
	append_code(make_label(s_tmp));
}

/* Build necessary code for a while statement */
void build_while_stmt(environment *env, NODE *node, int while_count, int if_count, int flag, int return_type) {
	char *s_tmp, *val1, *val2, *temporary;
	tac_quad *loop_jmp;
	if (node==NULL || type_of(node)!=WHILE) return;
	
	/* Generate label for start of while loop */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "__while%d", while_count);
	append_code(make_label(s_tmp));
	
	/* Generate loop jump */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "__while%d", while_count);
	loop_jmp = make_goto(s_tmp);

	/* End while loop stmt */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "__while%dend", while_count);
	
	/* Build IF stmt for condition */
	build_if_stmt(env, node, if_count, make_goto(s_tmp), loop_jmp, flag, return_type);
	
	append_code(make_label(s_tmp));
	
}

/* Register params in environment */
void register_params(environment *env, value *param_list) {
	value *current_param;
	if (!param_list) return;
	current_param = param_list;
	while (current_param!=NULL) {
		value *param = NULL;
		value *param_name;
		param_name = string_value(current_param->identifier);
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
		append_code(make_quad_value("", param, NULL, NULL, TT_POP_PARAM, 0));
		current_param = current_param->next;
	}
}

/* Push params on param stack in reverse order (recursively) */
tac_quad *push_params(environment *env, value *params_head) {
	if (!params_head) return NULL;
	if (params_head->next) {
		append_code(push_params(env, params_head->next));
	}
	if (params_head->value_type == VT_STRING) {
		value *tmp = get(env, correct_string_rep(params_head));
		return make_quad_value("", tmp, NULL, NULL, TT_PUSH_PARAM, 0);		
	}
	return make_quad_value("", params_head, NULL, NULL, TT_PUSH_PARAM, 0);		
}

/* Declare variables underneath a declarator tree */
void declare_variables_tac(environment *env, NODE *node, int variable_type, int return_type) {
	value *variable_name = NULL;
	if (env == NULL || node == NULL) {
		return;
	}
	else if (type_of(node) == ',') {
		declare_variables_tac(env, node->left, variable_type, return_type);
		declare_variables_tac(env, node->right, variable_type, return_type);
		return;		
	}
	else if (type_of(node) == '=') { /* Specific assignment */
		variable_name = make_simple(env, node->left, 0, return_type);
	}
	else if (type_of(node) == LEAF) { /* Undefined assignment */
		variable_name = make_simple(env, node->left, 0, return_type);		
	}
	/* Assign variable */
	if (variable_name) {		
		/* Assign a default initialization value for this type */
		switch(variable_type) {	
			case INT:
				assign(env, variable_name, int_value(0), 1);
				break;
			case VOID:	
				assign(env, variable_name, void_value(), 1);						
				break;
			case FUNCTION:
				assign(env, variable_name, null_fn, 1);
				break;
		}
	}
	else {
		fatal("Could not ascertain variable name!");
	}
}

/* Go down the declarator tree initialising the variables, at this stage */
void register_variable_subtree_tac(environment *env, NODE *node, int return_type) {
	NODE *original_node = node;
	/* Ensure we have all required params */
	if (!env || !node || type_of(node) != '~') return;
	/* Skip over LEAF nodes */
	if (node->left != NULL && type_of(node->left) == LEAF) {
		node = node->left;
	}
	if (node->left != NULL && (type_of(node->left) == VOID || type_of(node->left) == FUNCTION || type_of(node->left) == INT)) {
		/* Find variable type */
		int variable_type = to_int(NULL, make_simple(env, node->left, 0, return_type));
		declare_variables_tac(env, original_node->right, variable_type, return_type);
	}
}


/* 
 * Make the given NODE simple - i.e. return a temporary for complex subtrees 
 * The appropriate code is also generated and pushed onto the code stack
*/
value *make_simple(environment *env, NODE *node, int flag, int return_type) {
	int i_value = 0;
	char *s_tmp = NULL;
	value *val1 = NULL, *val2 = NULL, *temporary = NULL, *temp = NULL;
	static int if_count = 0;
	static int while_count = 0;	
	tac_quad *temp_quad = NULL;
	environment *new_env = NULL;
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
			build_if_stmt(env, node, ++if_count, NULL, NULL, flag, return_type);
			return NULL;
		case BREAK:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "__while%dend", while_count);
			append_code(make_goto(s_tmp));
			return NULL;			
		case CONTINUE:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "__while%d", while_count);
			append_code(make_goto(s_tmp));
			return NULL;
		case WHILE:
			build_while_stmt(env, node, ++while_count, ++if_count, flag, return_type);
			return NULL;	
		case '=':
			if (flag == INTERPRET_FN_SCAN) return NULL;
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
			temporary = assign(env, val1, val2, 0);
			if (flag != INTERPRET_FN_SCAN) append_code(make_quad_value("=", val2, NULL, temporary, TT_ASSIGN, 0));
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
			if (val1->value_type==VT_STRING) val1 = get(env, correct_string_rep(val1));
			if (val2->value_type==VT_STRING) val2 = get(env, correct_string_rep(val2));
			assert(val1 != NULL, "Operand value 1 must not be null");	
			assert(val2 != NULL, "Operand value 2 must not be null");				
			if (flag != INTERPRET_FN_SCAN) append_code(make_quad_value(type_to_string(type_of(node)), val1, val2, temporary, TT_OP, type_of(node)));
			return temporary;
		case '~':
			if (flag != INTERPRET_PARAMS && flag!=INTERPRET_FN_SCAN) {
				/* Params should not be registered, because at this point we're not in the correct environment */
				register_variable_subtree_tac(env, node, VT_ANY);
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
			
			/* New FN body environment */
			new_env = create_environment(env);
			if (val1!=NULL) {
				/* Point function to the correct fn body */
				val1->data.func->node_value = node->right;			
				/* Store function definition in environment */
				val2 = store_function(env, val1, new_env);
			}
			if (flag != INTERPRET_FN_SCAN) {
				/* Write out FN Name label */
				append_code(make_begin_fn(val2));				
				append_code(make_fn_def(val2));
				/* Make init frame */
				temp_quad = make_init_frame();
				append_code(temp_quad);
				/* Define parameters with default empty values */
				register_params(new_env, val2->data.func->params);
				append_code(make_fn_body(val2));
				/* Look inside fn body */
				val2 = make_simple(new_env, node->right, EMBEDDED_FNS, val1->data.func->return_type);
				/* Update prepare frame with environment size */
				temp_quad->operand1 = int_value(env_size(new_env));
				/* Write end of function marker */
				append_code(make_end_fn(val2));
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
			/* Lookup function */
			temp = search(env, to_string(val1), VT_FUNCTN, VT_ANY, 1);
			if (temp) {
				int fn_return_type;
				append_code(prepare_fn(val2));
				append_code(push_params(env, val2));
				/* If we can't typecheck, set a special UNDEFINED flag to say we can't */
				/* typecheck. This can happen with function variables, we do not EASILY know the */
				/* return type of the functions they are bound to until runtime. */
				fn_return_type = UNDEFINED;
				if (temp->data.func) {
					fn_return_type = temp->data.func->return_type;	
				} 
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
						temporary = generate_untypechecked_temporary(env);
						break;
				}
				append_code(make_fn_call(temporary, temp));
				return temporary;
			}
			else {
				fatal("Cannot find function '%s'", to_string(val1));
			}
			return NULL;
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
tac_quad *start_tac_gen(NODE *tree) {
	/* Do a scan for function definitions first */
	environment *initial_env;
	initial_env = create_environment(NULL);
	null_fn = build_null_function();
	make_simple(initial_env, tree, INTERPRET_FN_SCAN, INT);
	/* Actually generate the TAC */
	make_simple(initial_env, tree, 0, 0);
	return tac_output;
}