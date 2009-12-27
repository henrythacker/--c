#include "mips.h"

void write_preamble() {
	struct tm *local_time;
	time_t gen_time;
	gen_time = time(NULL);
	local_time = localtime(&gen_time);
	printf("# Compiled from --C\n# %s\n\n.data\n\tEOL:\t.asciiz \"\\n\"\n.text\n", asctime(local_time));
}

/* Find size required for a given fn */
int local_size(value *fn_def) {
	if (!fn_def) return 0;
	if (fn_def->value_type!=VT_FUNCTN) return 0;
	environment *env = fn_def->data.func->local_env;
	if (!env) fatal("Size of local environment can not be determined");
	return env->env_size;
}

/* Write out code */
void write_code(tac_quad *quad) {
	static int param_number = 0;
	if (!quad) return;
	switch(quad->type) {
		case TT_FN_DEF:
			printf("_%s:\n", correct_string_rep(quad->operand1));
			break;
		case TT_BEGIN_FN:
			if (strcmp(correct_string_rep(quad->operand1), "main") == 0) entry_point = quad;
			/* Set up activation record */
			printf("\tsw $ra, 4($sp) # Save previous $ra on stack (offset 4)\n");
			printf("\tsw $fp, 8($sp) # Save previous $fp on stack (offset 8)\n");
			printf("\tmove $fp, $sp # $fp = $sp, so we can index arguments as offsets from $fp\n");
			break;
		case TT_LABEL:
			printf("%s:\n", correct_string_rep(quad->operand1));
			break;
		case TT_ASSIGN:
			printf("\t ###### Result - variable number: %d\n", quad->result->variable_number);
			break;
		case TT_PUSH_PARAM:
			printf("\tsw $a%d, -%d($sp) # Push param\n", param_number, (param_number + 1) * 4);
			param_number++;
			break;
		case TT_PREPARE:
			/* Assign enough space for the activation record */
			printf("\tsub $sp, %d\n", ACTIVATION_RECORD_SIZE);
			break;
		case TT_FN_CALL:
			// Reset param count
			param_number = 0;
			printf("\tjal _%s\n\tsw $v0, 0($fp)\n", correct_string_rep(quad->operand1));			
			printf("\tadd $sp, %d # Move $sp back\n", ACTIVATION_RECORD_SIZE);
			break;
		case TT_RETURN:
			/* Save the return value */
			printf("\tli $v0, %s\n", correct_string_rep(quad->operand1));
			/* Restore the activation record */
			printf("\tmove $sp, $fp # Restore $sp, so we can index previous $ra, $fp\n");
			printf("\tlw $ra, 4($sp) # Restore previous $ra\n");
			printf("\tlw $fp, 8($sp) # Restore previous $fp\n");			
			printf("\tjr $ra\n");
			break;
		default:
			printf("", quad->type);
			break;
	}
	write_code(quad->next);
}

void write_epilogue() {
	tac_quad *quad = entry_point;
	if (!entry_point) return;
	printf("main:\n");
	printf("\tsub $sp, %d\n", ACTIVATION_RECORD_SIZE);
	printf("\tjal _main\n");
	printf("\tadd $sp, %d\n", ACTIVATION_RECORD_SIZE);
	printf("\tmove $a0, $v0\n");
	/* Print result */
	printf("\tli $v0, 1\n\tsyscall\n");
	/* Print newline */
	printf("\tli $v0, 4\n\tla $a0, EOL\n\tsyscall\n");	
	/* Exit */
	printf("\tli $v0, 10\n\tsyscall\n");
}

/* Generate MIPS code for given tree */
void code_gen(NODE *tree) {
	tac_quad *quad = start_tac_gen(tree);
	write_preamble();
	write_code(quad);
	write_epilogue();
}