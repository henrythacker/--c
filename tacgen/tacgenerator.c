#include "tacgenerator.h"

/* Generate a new temporary */
value *generate_temporary(environment *env) {
	char *tmp;
	static int temp_count = 0;
	tmp = malloc(sizeof(char) * 15);
	sprintf(tmp, "t%d", ++temp_count);
	return register_temporary(env, tmp);
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
			printf("%s\n", to_string(quad->operand1));
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


/* Build necessary code for an if statement */
void build_if_stmt(environment *env, NODE *node, int if_count, tac_quad *end_jump, int flag, int return_type) {
	char *s_tmp;
	value *val1, *val2, *temporary;
	if (node==NULL || (type_of(node)!=IF && type_of(node)!=WHILE)) return;
	/* LHS is condition */
	val1 = make_simple(env, node->left, flag, return_type);
	
	/* Generate if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dtrue", if_count);
	append_code(make_if(val1, s_tmp));

	/* Output false branch (i.e. else part) */	
	if (type_of(node->right)==ELSE) {
		/* Build code for false part */
		build_else_part(env, node->right, 0, flag, return_type);
	}
	
	/* Generate goto end of if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dend", if_count);
	append_code(make_goto(s_tmp));
	
	/* Generate label for start of true branch */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dtrue:", if_count);
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
	sprintf(s_tmp, "if%dend:", if_count);
	append_code(make_label(s_tmp));
}

/* Build necessary code for a while statement */
void build_while_stmt(environment *env, NODE *node, int while_count, int if_count, int flag, int return_type) {
	char *s_tmp, *val1, *val2, *temporary;
	tac_quad *loop_jmp;
	if (node==NULL || type_of(node)!=WHILE) return;
	
	/* Generate label for start of while loop */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "while%d:", while_count);
	append_code(make_label(s_tmp));
	
	/* Generate loop jump */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "while%d", while_count);
	loop_jmp = make_goto(s_tmp);
	
	/* Build IF stmt for condition */
	build_if_stmt(env, node, if_count, loop_jmp, flag, return_type);
	
	/* End while loop stmt */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "while%dend:", while_count);
	append_code(make_label(s_tmp));
	
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
			sprintf(s_tmp, "while%dend", while_count);
			append_code(make_label(s_tmp));
			return NULL;			
		case CONTINUE:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "while%d", while_count);
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
					printf("2\n");					
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
			temporary = generate_temporary(env);
			val1 = make_simple(env, node->left, flag, return_type);
			val2 = make_simple(env, node->right, flag, return_type);
			append_code(make_quad_value(type_to_string(type_of(node)), val1, val2, temporary, TT_OP));
			return temporary;
		case '~':
			register_variable_subtree(env, node, VT_ANY);
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
			if (val1!=NULL) {
				/* Point function to the correct fn body */
				val1->data.func->node_value = node->right;
				/* Store function definition in environment */
				store_function(env, val1);
			}
			/* Look inside body, but in new environment */
			new_env = create_environment(env);
			val2 = make_simple(new_env, node->right, flag, return_type);
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
		case ',':
			val1 = make_simple(env, node->left, flag, return_type);
			val2 = make_simple(env, node->right, flag, return_type);
			if (val1 && val2) {
				return join(val1, val2);
			}
			return NULL;			
		case INT:
		case VOID:
			return NULL;
		default:
			make_simple(env, node->left, flag, return_type);
			make_simple(env, node->right, flag, return_type);			
			return NULL;
	}
	
}


/* Start the TAC generator process at the top of the AST */
void start_tac_gen(NODE *tree) {
	environment *default_env = create_environment(NULL);
	make_simple(default_env, tree, 0, 0);
	print_tac(tac_output);
}