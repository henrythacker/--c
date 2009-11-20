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
			if (quad->op!=' ') {
				printf("%s %c %s\n", quad->result, quad->op, quad->operand1);
			}
			else {
				printf("%s %s\n", quad->result, quad->operand1);
			}
			break;
		case 2:
			printf("%s = %s %c %s\n", quad->result, quad->operand1, quad->op, quad->operand2);
			break;	
	}
	print_tac(quad->next);
}

/* Make a TAC quad */
tac_quad *make_quad_value(char op, char *operand1, char *operand2, char *result) {
	tac_quad *tmp_quad = (tac_quad *)malloc(sizeof(tac_quad));
	tmp_quad->op = op;
	tmp_quad->operand1 = (char *)malloc(sizeof(char) * (strlen(operand1) + 1));
	tmp_quad->operand2 = (char *)malloc(sizeof(char) * (strlen(operand2) + 1));
	tmp_quad->result = (char *)malloc(sizeof(char) * (strlen(result) + 1));
	strcpy(tmp_quad->operand1, operand1);
	strcpy(tmp_quad->operand2, operand2);
	strcpy(tmp_quad->result, result);
	return tmp_quad;
}

char *make_simple(NODE *node) {
	int i_value = 0;
	char *s_tmp, *val1, *val2, *temporary;
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
		case '=':
			val1 = make_simple(node->left);
			val2 = make_simple(node->right);
			append_code(make_quad_value('=', val2, "", val1));
			return NULL;
		case '*':
		case '/':
		case '>':
		case '<':										
		case '%':							
		case '-':									
		case '+':
			temporary = generate_temporary();
			val1 = make_simple(node->left);
			val2 = make_simple(node->right);
			append_code(make_quad_value(type_of(node), val1, val2, temporary));	
			return temporary;	
		case '~':
			make_simple(node->left);
			make_simple(node->right);			
			return NULL;
		case INT:
		case VOID:
			return NULL;
	}
	
}


/* Start the TAC generator process at the top of the AST */
void start_tac_gen(NODE *tree) {
	make_simple(tree);
	print_tac(tac_output);
}