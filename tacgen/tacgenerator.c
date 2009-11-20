#include "tacgenerator.h"

/* Generate a new temporary */
int generate_temporary() {
	static int temp_count = 0;
	return temp_count++;
}

/* Is the given node simple? */
int is_node_simple(NODE *node) {
	if (node==NULL) return 0;
	return (type_of(node) == CONSTANT || type_of(node) == IDENTIFIER);
}

/* Assertion with error text */
void assert(int assertion, char *error) {
	if (!assertion) {
		printf("TAC Error: %s\n", error);
		exit(-1);
	}
}

/* Make a TAC quad */
tac_quad *make_quad(char op, char *operand1, char *operand2, char *result) {
	tac_quad *tmp_quad = (tac_quad *) malloc(sizeof(tac_quad));
	tmp_quad->op = op;
	tmp_quad->operand1 = malloc(sizeof(char) * (strlen(operand1) + 1));
	tmp_quad->operand2 = malloc(sizeof(char) * (strlen(operand2) + 1));
	tmp_quad->result = malloc(sizeof(char) * (strlen(result) + 1));
	strcpy(tmp_quad->operand1, operand1);
	strcpy(tmp_quad->operand2, operand2);
	strcpy(tmp_quad->result, result);
	return tmp_quad;
}

/* Compile AST into TAC quad */
tac_quad *compile_tac(NODE *node) {
	if (is_node_simple(node)) return make_quad('S', );
	switch(type_of(node)) {
		case('='):
			/* LHS must be simple */
			assert(is_node_simple(node->left), "LHS of assignment must be simple expression");
			return NULL;
	}
}


/* Start the TAC generator process at the top of the AST */
void start_tac_gen(NODE *tree) {
	
}