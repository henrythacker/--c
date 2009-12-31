#include "mips.h"

void write_preamble() {
	struct tm *local_time;
	time_t gen_time;
	gen_time = time(NULL);
	local_time = localtime(&gen_time);
	printf("# Compiled from --C\n# %s\n\n.data\n\tEOL:\t.asciiz \"\\n\"\n.text\n", asctime(local_time));
}

/* Is the value a constant? */
int is_constant(value *var) {
	return var && !var->temporary && var->value_type == VT_INTEGR && var->identifier[0]=='_';
}

void write_epilogue() {
	tac_quad *quad = entry_point;
	if (!entry_point) return;
	printf("main:\n");
	printf("\tjal _main\n");
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
		/* TO DO: Move out existing value into heap */
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

int activation_record_size(int local_size) {
	int word_size = 4;
	/* Special fields are: */
	/* - Previous $fp */
	/* - Static link */		
	int special_fields = 2; 
	int allocation_size = (word_size * local_size) + (word_size * special_fields);
	return allocation_size;
}

/* Make the activation record structure, store the address of the created structure into the destination_reg */
int generate_activation_record(int local_size) {
	int allocation_size = activation_record_size(local_size);
	/* TO DO: Move aside whatever is in $a0, $v0 */
	printf("\tli $a0, %d # Allocation size for activation record\n", allocation_size);
	printf("\tli $v0, 9 # Allocate space systemcode\n");
	printf("\tsyscall # Allocate space on heap\n");
	/* Move result */
	printf("\tmove $s0, $v0 # Save activation record address\n");
	/* TO DO: Restore whatever was in $a0, $v0 */
	return allocation_size;
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
		regs[reg_id]->contents = var;	
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

/* Code generate PUSHING a parameter */
void cg_push_param(value *operand, int param_number) {
	if (param_number > 4) {
		/* TO DO: Store at end of heap */
	}
	else {
		/* TO DO: If value is not empty, save it in current frame */
		int operand_reg = which_register(operand, 1);
		printf("\tmove $a%d, %s # Push operand %d\n", param_number, regs[operand_reg]->name, param_number + 1);
	}
}

/* Code generate POPPING a parameter */
void cg_pop_param(value *operand, int param_number) {
	if (param_number > 4) {
		/* TO DO: Load from end of heap */
	}
	else {
		int index = param_number + 1;
		if (regs[index]->contents) {
			/* TO DO: Backup existing value */
		}
		regs[index]->contents = operand;
		regs[index]->assignment_id = ++regs_assignments;
		regs[index]->accesses = 1;
	}
}

/* Code generate an assignment */
void cg_assign(value *result, value *operand1) {
	int result_reg = which_register(result, 0);
	regs[result_reg]->contents = result;	
	int op1_reg = which_register(operand1, 1);
	regs[op1_reg]->contents = operand1;
	printf("\tmove %s, %s\n", regs[result_reg]->name, regs[op1_reg]->name);
}

/* Code generate an IF statement */
void cg_if(value *condition, value *true_label) {
	
}

/* Code generate a fn call */
void cg_fn_call(value *result, value *fn_def) {
	int result_reg = which_register(result, 0);
	regs[result_reg]->contents = result;	
	printf("\tjal _%s\n\tmove %s, $v0\n", correct_string_rep(fn_def), regs[result_reg]->name);
}

/* Write out code */
void write_code(tac_quad *quad) {
	/* No parameters, 0 = 1 parameter (0 because $a0 is first arg register), so -1 is no args */
	static int param_number = -1;
	static int frame_size = 0;
	int size = 0;
	int temporary;
	if (!quad) return;
	switch(quad->type) {
		case TT_FN_DEF:
			break;
		case TT_INIT_FRAME:
			/* Get a place to store the old $s0 - i.e. static link */
			temporary = choose_best_reg();
			size = to_int(NULL, quad->operand1);
			printf("\tmove %s, $s0\n", regs[temporary]->name);
			size = generate_activation_record(size);
			frame_size = size;
			/* Store frame pointer */
			printf("\tsw $fp, %d($s0) # Save previous frame ptr\n", size - 4);
			/* Store static link AFTER frame pointer */
			printf("\tsw %s, %d($s0) # Save static link\n", regs[temporary]->name, size - 8);
			break;
		case TT_BEGIN_FN:
			if (strcmp(correct_string_rep(quad->operand1), "main")==0) entry_point = quad;
			printf("_%s:\n", correct_string_rep(quad->operand1));
			printf("\tadd $sp, $sp, 4\n");
			printf("\tsw $ra, 0($sp)\n");
			param_number = -1;
			break;
		case TT_POP_PARAM:
			cg_pop_param(quad->operand1, ++param_number);
			break;
		case TT_LABEL:
			printf("%s:\n", correct_string_rep(quad->operand1));		
			break;
		case TT_ASSIGN:
			cg_assign(quad->result, quad->operand1);
			break;
		case TT_PUSH_PARAM:
			cg_push_param(quad->operand1, ++param_number);
			break;
		case TT_PREPARE:
			param_number = -1;
			break;
		case TT_IF:
			cg_if(quad->operand1, quad->result);
			break;
		case TT_OP:
			cg_operation(quad->subtype, quad->operand1, quad->operand2, quad->result);
			break;
		case TT_FN_CALL:
			// Reset param count
			param_number = -1;
			cg_fn_call(quad->result, quad->operand1);			
			break;
		case TT_END_FN:
			/* Load return address from stack */
			printf("\tlw $ra, 0($sp) # Get return address\n");
			printf("\tsub $sp, $sp, 4 # Pop return address from stack\n");
			printf("\tmove $v0, $0 # Null return value\n");
			/* Load previous frame pointer */
			printf("\tlw $fp, %d($s0) # Load previous frame ptr\n", frame_size - 4);
			/* Load previous heap pointer */
			printf("\tlw $s0, %d($s0) # Load static link\n", frame_size - 8);
			printf("\tjr $ra # Jump to $ra\n");
			break;
		case TT_RETURN:
			/* Save the return value */
			/* Restore the activation record */
			if (quad->operand1) {
				printf("\tmove $v0, %s # Set return value\n", regs[which_register(quad->operand1, 1)]->name);
			}
			/* Load previous return address from stack */
			printf("\tlw $ra, 0($sp) # Get return address\n");
			printf("\tsub $sp, $sp, 4 # Pop return address from stack\n");
			/* Load previous frame pointer */
			printf("\tlw $fp, %d($s0) # Load previous frame ptr\n", frame_size - 4);
			/* Load previous "heap" pointer */
			printf("\tlw $s0, %d($s0) # Load static link\n", frame_size - 8);
			printf("\tjr $ra # Jump to $ra\n");
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