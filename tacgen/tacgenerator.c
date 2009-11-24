#include "tacgenerator.h"

/* Generate a new temporary */
char *generate_temporary() {
	char *tmp;
	static int temp_count = 0;
	tmp = malloc(sizeof(char) * 15);
	sprintf(tmp, "t%d", ++temp_count);
	return tmp;
}

/* Assertion with error text */
void assert(int assertion, char *error) {
	if (!assertion) {
		printf("TAC Error: %s\n", error);
		exit(-1);
	}
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

/* Operand Count */
int operand_count(tac_quad *quad) {
	int count = 0;
	if (quad!=NULL && quad->operand1!=NULL && strlen(quad->operand1)>0) {
		count++;
	}
	if (quad!=NULL && quad->operand2!=NULL && strlen(quad->operand2)>0) {
		count++;
	}
	return count;
}

/* TAC to stdout */
void print_tac(tac_quad *quad) {
	if (quad==NULL) return;
	int operands = operand_count(quad);
	switch(operands) {
		case 0:
			printf("%s\n", quad->result);
			break;
		case 1:
			if (strlen(quad->op)>0) {
				if (strcmp(quad->op, "?")==0) {
					/* if statement */
					printf("if %s goto %s\n", quad->operand1, quad->result);
				}
				else {
					printf("%s %s %s\n", quad->result, quad->op, quad->operand1);
				}
			}
			else {
				printf("%s %s\n", quad->result, quad->operand1);
			}
			break;
		case 2:
			printf("%s = %s %s %s\n", quad->result, quad->operand1, quad->op, quad->operand2);
			break;	
	}
	print_tac(quad->next);
}

/* Make a TAC quad */
tac_quad *make_quad_value(char *op, char *operand1, char *operand2, char *result) {
	tac_quad *tmp_quad = (tac_quad *)malloc(sizeof(tac_quad));
	tmp_quad->op = (char *)malloc(sizeof(char) * (strlen(op) + 1));
	tmp_quad->operand1 = (char *)malloc(sizeof(char) * (strlen(operand1) + 1));
	tmp_quad->operand2 = (char *)malloc(sizeof(char) * (strlen(operand2) + 1));
	tmp_quad->result = (char *)malloc(sizeof(char) * (strlen(result) + 1));
	strcpy(tmp_quad->op, op);
	strcpy(tmp_quad->operand1, operand1);
	strcpy(tmp_quad->operand2, operand2);
	strcpy(tmp_quad->result, result);
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
void build_else_part(NODE *node, int true_part) {
	if (node==NULL || type_of(node)!=ELSE) return;
	if (true_part) {
		make_simple(node->left);
	}
	else {
		make_simple(node->right);		
	}
}

/* Build necessary code for an if statement */
void build_if_stmt(NODE *node, int if_count, tac_quad *end_jump) {
	char *s_tmp, *val1, *val2, *temporary;
	if (node==NULL || (type_of(node)!=IF && type_of(node)!=WHILE)) return;
	/* LHS is condition */
	val1 = make_simple(node->left);
	
	/* Generate if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dtrue", if_count);
	append_code(make_quad_value("?", val1, "", s_tmp));

	/* Output false branch (i.e. else part) */	
	if (type_of(node->right)==ELSE) {
		/* Build code for false part */
		build_else_part(node->right, 0);
	}
	
	/* Generate goto end of if statement */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dend", if_count);
	append_code(make_quad_value("", s_tmp, "", "goto"));
	
	/* Generate label for start of true branch */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dtrue:", if_count);
	append_code(make_quad_value("", "", "", s_tmp));
	
	/* Output true branch */
	if (type_of(node->right)==ELSE) {
		/* Build code for true part */
		build_else_part(node->right, 1);
	}
	else {
		/* True part is whole right branch */
		make_simple(node->right);
	}
	
	/* Check if extra loop jump has been specified (for WHILE loops etc) */
	if (end_jump) {
		append_code(end_jump);
	}
	
	/* Generate end of IF stmt label */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "if%dend:", if_count);
	append_code(make_quad_value("", "", "", s_tmp));
}

/* Build necessary code for a while statement */
void build_while_stmt(NODE *node, int while_count, int if_count) {
	char *s_tmp, *val1, *val2, *temporary;
	tac_quad *loop_jmp;
	if (node==NULL || type_of(node)!=WHILE) return;
	
	/* Generate label for start of while loop */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "while%d:", while_count);
	append_code(make_quad_value("", "", "", s_tmp));
	
	/* Generate loop jump */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "while%d", while_count);
	loop_jmp = make_quad_value("", s_tmp, "", "goto");
	
	/* Build IF stmt for condition */
	build_if_stmt(node, if_count, loop_jmp);
	
	/* End while loop stmt */
	s_tmp = malloc(sizeof(char) * 25);
	sprintf(s_tmp, "while%dend:", while_count);
	append_code(make_quad_value("", "", "", s_tmp));
	
}

/* 
 * Make the given NODE simple - i.e. return a temporary for complex subtrees 
 * The appropriate code is also generated and pushed onto the code stack
*/
char *make_simple(NODE *node) {
	int i_value = 0;
	char *s_tmp, *val1, *val2, *temporary;
	static int if_count = 0;
	static int while_count = 0;	
	if (node==NULL) return NULL;
	switch(type_of(node)) {
		case LEAF: 
			return make_simple(node->left);
		case CONSTANT:
			/* Convert int to string */
			s_tmp = malloc(sizeof(char) * 15);
			i_value = cast_from_node(node)->value;
			sprintf(s_tmp, "%d", i_value);
			return s_tmp;
		case IDENTIFIER:
			return cast_from_node(node)->lexeme;
		case IF:
			build_if_stmt(node, ++if_count, NULL);
			return NULL;
		case BREAK:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "while%dend", while_count);
			append_code(make_quad_value("", s_tmp, "", "goto"));
			return NULL;			
		case CONTINUE:
			s_tmp = malloc(sizeof(char) * 25);
			sprintf(s_tmp, "while%d", while_count);
			append_code(make_quad_value("", s_tmp, "", "goto"));
			return NULL;
		case WHILE:
			build_while_stmt(node, ++while_count, ++if_count);
			return NULL;	
		case '=':
			val1 = make_simple(node->left);
			val2 = make_simple(node->right);
			append_code(make_quad_value("=", val2, "", val1));
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
			temporary = generate_temporary();
			val1 = make_simple(node->left);
			val2 = make_simple(node->right);
			append_code(make_quad_value(type_to_string(type_of(node)), val1, val2, temporary));
			return temporary;
		case '~':
			make_simple(node->left);
			make_simple(node->right);			
			return NULL;
		case INT:
		case VOID:
			return NULL;
		default:
			make_simple(node->left);
			make_simple(node->right);			
			return NULL;
	}
	
}


/* Start the TAC generator process at the top of the AST */
void start_tac_gen(NODE *tree) {
	make_simple(tree);
	print_tac(tac_output);
}