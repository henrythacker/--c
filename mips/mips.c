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

/* Initialise our global view of register allocation */
void init_register_view() {
	int i;
	for (i = 0; i < REG_COUNT; i++) {
		char *reg_prefix = i == 0 ? "" : (i < 5 ? "a" : "t");
		int reg_count = i == 0 ? 0 : (i > 4 ? i - 5 : i - 1);
		regs[i] = (register_contents *) malloc(sizeof(register_contents));
		regs[i]->contents = NULL;
		regs[i]->accesses = 0;
		regs[i]->assignment_id = 0;
		regs[i]->name = malloc(sizeof(char) * 4);
		sprintf(regs[i]->name, "$%s%d", reg_prefix, reg_count);
	}
}

/* Print the view of the registers */
void print_register_view() {
	int i;
	printf("Register View\n---------------\n");
	for (i = 0; i < REG_COUNT; i++) {
		printf("[%s] \t- Contents: [%p - %s] - Accesses: %d - Assignment order: %d\n", regs[i]->name, regs[i]->contents, regs[i]->contents ? regs[i]->contents->identifier : "EMPTY", regs[i]->accesses, regs[i]->assignment_id);
	}
}

/* Find the first free register, if any */
int first_free_reg() {
	int position = 5;
	for (position = 5; position < REG_COUNT; position++) {
		if (!regs[position]->contents) {
			regs[position]->assignment_id = ++regs_assignments;			
			regs[position]->contents = NULL;
			regs[position]->accesses = 1;			
			return position;
		} 
	}
	return REG_NONE_FREE;
}

/* If no registers free, replace the oldest value */
int choose_best_reg() {
	int free_reg = first_free_reg();
	if (free_reg == REG_NONE_FREE) {
		int position = 5;
		int lowest_assignment_order = -1;
		int optimal_reg = -1;
		for (position = 5; position < REG_COUNT; position++) {
			if (lowest_assignment_order == -1 || regs[position]->assignment_id < lowest_assignment_order) {
				lowest_assignment_order = regs[position]->assignment_id;
				optimal_reg = position;
			}
		}
		/* TO DO: Move out existing value into stack */
		regs[optimal_reg]->assignment_id = ++regs_assignments;
		regs[optimal_reg]->contents = NULL;
		regs[optimal_reg]->accesses = 1;		
		return optimal_reg;
	}
	return free_reg;
}

/* Is the value already in a register?? */
int already_in_reg(value *var) {
	int position = 0;
	for (position = 0; position < REG_COUNT; position++) {
		/* Point to same variable OR are equivalent constants */
		if (regs[position]->contents == var || (is_constant(var) && regs[position]->contents && is_constant(regs[position]->contents) && to_int(NULL, var) == to_int(NULL, regs[position]->contents))) {
			/* Increment accesses to track how popular this register is */
			regs[position]->accesses = regs[position]->accesses + 1;
			regs[position]->assignment_id = ++regs_assignments;			
			return position;
		}
	}
	return REG_VALUE_NOT_AVAILABLE;
}

/* All encompassing register assignment fn */
int which_register(value *var, int must_already_exist) {
	int reg_id = already_in_reg(var);
	if (reg_id == REG_VALUE_NOT_AVAILABLE) {
		if (!is_constant(var) && must_already_exist) {
			fatal("Could not find value %s", correct_string_rep(var));
		}
		reg_id = first_free_reg();
		if (reg_id == REG_NONE_FREE) {
			reg_id = choose_best_reg();
			assert(reg_id!=-1, "Could not free up a register");
		}
	}
	/* Load constants into the freed register, if applicable */
	if (is_constant(var)) {
		/* Special zero register */
		if (to_int(NULL, var)==0) return 0;
		printf("\tli %s, %s # Load constant into freed register\n", regs[reg_id]->name, correct_string_rep(var));
	}
	return reg_id;
}

/* Generate code for an operation */
void cg_operation(int operation, value *op1, value *op2, value *result) {
	int result_reg = which_register(result, 0);
	regs[result_reg]->contents = result;	
	int op1_reg = which_register(op1, 1);
	regs[op1_reg]->contents = op1;	
	int op2_reg = which_register(op2, 1);	
	regs[op2_reg]->contents = op2;		
	switch(operation) {
		case '+':
			printf("\tadd %s, %s, %s\n", regs[result_reg]->name, regs[op1_reg]->name, regs[op2_reg]->name);
			break;
		case '-':
			printf("\tsub %s, %s, %s\n", regs[result_reg]->name, regs[op1_reg]->name, regs[op2_reg]->name);
			break;
		case '*':
			printf("\tmult %s, %s\n\tmflo %s\n", regs[op1_reg]->name, regs[op2_reg]->name, regs[result_reg]->name);
			break;	
		case '/':
			printf("\tdiv %s, %s\n\tmflo %s\n", regs[op1_reg]->name, regs[op2_reg]->name, regs[result_reg]->name);
			break;
		case '%':
			printf("\tdiv %s, %s\n\tmfhi %s\n", regs[op1_reg]->name, regs[op2_reg]->name, regs[result_reg]->name);
			break;
	}
}

/* Write out code */
void write_code(tac_quad *quad) {
	static int param_number = 0;
	static environment *current_env = NULL;
	if (!quad) return;
	switch(quad->type) {
		case TT_FN_DEF:
			break;
		case TT_BEGIN_FN:
			current_env = quad->operand1->data.func->local_env;
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

/* Generate MIPS code for given tree */
void code_gen(NODE *tree) {
	regs = (register_contents **) malloc(sizeof(register_contents *) * REG_COUNT);
	init_register_view();
	print_register_view();
	tac_quad *quad = start_tac_gen(tree);
	write_preamble();
	write_code(quad);
	write_epilogue();
}