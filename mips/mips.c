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

/* Is the value a constant? */
int is_constant(value *var) {
	return var && !var->temporary && var->value_type == VT_INTEGR && var->identifier[0]=='_';
}

/* Get hold of variable */
char *load_variable(environment *local_env, value *var) {
	int static_link = 0;
	int found = -1;
	int links = 0;
	environment *current_env = local_env;
	while (current_env) {
		if (var->stored_in_env == current_env) {
			found = static_link;
			break;
		}
		current_env = current_env->static_link;
		static_link++;
	}
	if (found==-1) fatal("Could not load variable");
	if (found==0) {
		/* Already in current env, should be available more easily */
		
	}
	else {
		/* Follow $fp, found number of times */
		for (links=0; links<found; links++) {
		
		}
	}
}

void cg_operation(int operation, value *op1, value *op2, value *result) {
	
}

/* Write out code */
void write_code(tac_quad *quad) {
	static int param_number = 0;
	if (!quad) return;
	switch(quad->type) {
		case TT_FN_DEF:
			break;
		case TT_BEGIN_FN:
			break;
		case TT_LABEL:
			break;
		case TT_ASSIGN:
			break;
		case TT_PUSH_PARAM:
			printf("\tsub $sp, $sp, 4 # Reserve space for pushed parameter\n");
			printf("\tsw $t, 0($sp) # Reserve space for pushed parameter\n");			
			break;
		case TT_PREPARE:
			break;
		case TT_OP:
			cg_operation(quad->subtype, quad->operand1, quad->operand2, quad->result);
			break;
		case TT_FN_CALL:
			// Reset param count
			param_number = 0;
			break;
		case TT_RETURN:
			/* Save the return value */
			/* Restore the activation record */
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