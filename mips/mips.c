#include "mips.h"

/* Append MIPS code to generated code stack */
void append_mips(struct mips_instruction *ins) {
	/* Are there any existing instructions? */
	if (!instructions) {
		instructions = ins;
	}
	else {
		struct mips_instruction *tmp_instruction = instructions;
		while (tmp_instruction->next != NULL) {
			tmp_instruction = tmp_instruction->next;
		}
		/* Append instruction onto the end */
		tmp_instruction->next = ins;
	}
}

/* Find the depth difference between two different environments */
int depth_difference(environment *caller_env, environment *callee_env) {
	int depth = 0;
	environment *env = caller_env;		
	if (callee_env == caller_env) return 0;
	while(env) {
		env = env->static_link;
		depth++;
		if (env == callee_env) {
			return depth;
		}
	}
	return -1;
}

/* Valid register - can the USER utilise the given register? */
int register_use_allowed(int reg_id) {
	/* can use all $t0-$t7 registers */
	return (reg_id >= $t0 && reg_id <= $t7);
}

int is_argument_register(int reg_id) {
	return reg_id >= $a0 && reg_id <= $a3;
}

/* Write header into source */
void write_preamble() {
	operand *comment;
	mips_instruction *ins;
	struct tm *local_time;
	time_t gen_time;
	gen_time = time(NULL);
	local_time = localtime(&gen_time);
	comment = make_label_operand("# Compiled from --C\n# %s\n.data\n\tEOL:\t.asciiz \"\\n\"\n.text\n", asctime(local_time));
	append_mips(mips_comment(comment, 0));
}

/* Is the value a constant? */
int is_constant(value *var) {
	return var && !var->temporary && var->value_type == VT_INTEGR && var->identifier[0]=='_';
}

/* Write stub fn to call into the user code and return the value */
void write_epilogue() {
	tac_quad *quad = entry_point;
	if (!entry_point) return;
	append_mips(mips("", OT_ZERO_ADDRESS, OT_UNSET, OT_UNSET, make_label_operand(".text"), NULL, NULL, "", 1));
	append_mips(mips(".globl", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("main"), NULL, NULL, "", 1));	
	append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("main"), NULL, NULL, "", 0));
	/* Pass zero dynamic and static links */
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($a1), make_register_operand($zero), NULL, "Zero dynamic link", 1));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v0), make_register_operand($zero), NULL, "Zero static link", 1));		
	/* Call the _main fn */
	append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("_main"), NULL, NULL, "", 1));
	/* Get hold of the return value */
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($a0), make_register_operand($v0), NULL, "Retrieve the return value of the main function", 1));
	/* Print the int */
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(1), NULL, "Print integer", 1));
	append_mips(syscall(""));	
	/* Print an EOL character */
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(4), NULL, "Print string", 1));
	append_mips(mips("la", OT_REGISTER, OT_LABEL, OT_UNSET, make_register_operand($a0), make_label_operand("EOL"), NULL, "Printing EOL character", 1));
	append_mips(syscall(""));
	/* Sys exit */
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(10), NULL, "System exit", 1));
	append_mips(syscall(""));
}

/* Initialise our global view of register allocation */
void init_register_view() {
	int i;
	for (i = 0; i < REG_COUNT; i++) {
		regs[i] = (register_contents *) malloc(sizeof(register_contents));
		regs[i]->contents = NULL;
		regs[i]->accesses = 0;
		regs[i]->assignment_id = 0;
		regs[i]->modified = 0;
	}
}

/* Print the view of the registers */
void print_register_view() {
	int i;
	if (!DEBUG_ON) return;
	printf("Register View\n---------------\n");
	for (i = 0; i < REG_COUNT; i++) {
		if (register_use_allowed(i) || is_argument_register(i)) {
			printf("[%s] \t- Contents: [%p - %s] - Modified: %d - Accesses: %d - Assignment order: %d\n", register_name(i), regs[i]->contents, regs[i]->contents ? regs[i]->contents->identifier : "EMPTY", regs[i]->modified, regs[i]->accesses, regs[i]->assignment_id);
		}
	}
}

/* Find the first free register, if any */
int first_free_reg() {
	int position = 0;
	for (position = 0; position < REG_COUNT; position++) {
		if (register_use_allowed(position) && !regs[position]->contents) {
			regs[position]->assignment_id = ++regs_assignments;			
			regs[position]->contents = NULL;
			regs[position]->accesses = 1;		
			regs[position]->modified = 0;	
			return position;
		} 
	}
	return REG_NONE_FREE;
}

/* If no registers free, replace the oldest value */
int choose_best_reg() {
	int free_reg = first_free_reg();
	if (free_reg == REG_NONE_FREE) {
		int position = 0;
		int lowest_assignment_order = -1;
		int optimal_reg = -1;
		for (position = 0; position < REG_COUNT; position++) {
			if (register_use_allowed(position)) {
				if (lowest_assignment_order == -1 || regs[position]->assignment_id < lowest_assignment_order) {
					lowest_assignment_order = regs[position]->assignment_id;
					optimal_reg = position;
				}
			}
		}
		/* TO DO: Move out existing value into heap */
		regs[optimal_reg]->assignment_id = ++regs_assignments;
		regs[optimal_reg]->contents = NULL;
		regs[optimal_reg]->accesses = 1;	
		regs[optimal_reg]->modified = 0;			
		return optimal_reg;
	}
	return free_reg;
}

/* Is the value already in a register?? */
int already_in_reg(value *var) {
	int position = 0;
	/* Check if this is a well known function, if so, pass back the address of the label in a register */
	if (var->value_type == VT_FUNCTN && var->data.func && var->data.func->node_value) {
		position = choose_best_reg();
		/* Return address to function */
		append_mips(mips("la", OT_REGISTER, OT_LABEL, OT_UNSET, make_register_operand($v0), make_label_operand("_%s", correct_string_rep(var)), NULL, "Store address of function", 1));
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v1), make_register_operand($s0), NULL, "Store static link to call with", 1));
		append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("rfunc"), NULL, NULL, "Register fn variable", 1));
		/* $v0 now contains fn descriptor, can be used to execute the function in the right way */
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(position), make_register_operand($v0), NULL, "Return fn descriptor address", 1));
		has_used_fn_variable = 1;
		return position;
	}
	/* Machine dependent optimization - use the zero register where possible */
	if (is_constant(var) && to_int(NULL, var) == 0) return $zero;
	
	for (position = 0; position < REG_COUNT; position++) {
		/* Point to same variable OR are equivalent constants */
		/* Must be sourced from a user accessible register, or the special zero register */
		if (register_use_allowed(position) || position == 0 || is_argument_register(position)) {
			if (regs[position]->contents == var) {
				/* Increment accesses to track how popular this register is */
				regs[position]->accesses = regs[position]->accesses + 1;
				regs[position]->assignment_id = ++regs_assignments;		
				return position;
			}
		}
	}
	return REG_VALUE_NOT_AVAILABLE;
}

int activation_record_size(int local_size) {
	int word_size = 4;
	/* Special fields are: */
	/* - Previous $fp */
	/* - Static link */	
	/* - Dynamic link */
	/* - Frame size */	
	int special_fields = 4; 
	int allocation_size = (word_size * local_size) + (word_size * special_fields);
	return allocation_size;
}

/* Outputs fn that is used to build a new activation record, local_size passed in $a0 */
void write_activation_record_fn() {
	/* Make label */
	operand *comment;
	comment = make_label_operand("# Make a new activation record\n# Precondition: $a0 contains total required heap size, $a1 contains dynamic link, $v0 contains static link\n# Returns: start of heap address in $v0, heap contains reference to static link and old $fp value");
	append_mips(mips_comment(comment, 0));
	append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("mk_ar"), NULL, NULL, "", 0));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($t8), make_constant_operand($v0), NULL, "Backup static link in $t8", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(9), NULL, "Allocate space systemcode", 1));
	append_mips(syscall("Allocate space on heap"));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($t9), make_constant_operand($fp), NULL, "Backup old $fp in $t9", 1));
	/* Point $fp to the correct place */
	append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand($fp), make_register_operand($v0), make_register_operand($a0), "$fp = heap start address + heap size", 1));
	/* Save static link */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($t8), make_offset_operand($v0, 0), NULL, "Save static link", 1));
	/* Save old $fp */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($t9), make_offset_operand($v0, 4), NULL, "Save old $fp", 1));	
	/* Save dynamic link */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($a1), make_offset_operand($v0, 8), NULL, "Save dynamic link", 1));	
	/* Save frame size */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($a0), make_offset_operand($v0, 12), NULL, "Save framesize", 1));	
	/* Jump back */
	append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "", 1));	
}

/* Outputs fn that is used to register new function variables */
void write_register_fn_variable() {
	operand *comment;
	comment = make_label_operand("# Make a new function variable entry\n# Precondition: $v0 contains function address, $v1 contains static link\n# Returns: address of allocated fn entry descriptor in $v0");
	append_mips(mips_comment(comment, 0));
	append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("rfunc"), NULL, NULL, "", 0));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($t8), make_constant_operand($v0), NULL, "Backup fn address in $t8", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($a0), make_constant_operand(8), NULL, "Space required for descriptor", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(9), NULL, "Allocate space systemcode", 1));
	append_mips(syscall("Allocate space on heap"));
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($t8), make_offset_operand($v0, 0), NULL, "Store fn address", 1));
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v1), make_offset_operand($v0, 4), NULL, "Store static link", 1));
	append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "", 1));	
}

/* Load a variable in local scope */
void cg_load_local_var(value *var, int destination_register) {
	int num = var->variable_number;
	append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(destination_register), make_offset_operand($fp, -4 * (num + 1)), NULL, "Load local variable", 1));
}

/*
* VARIABLE FNS
*/

/* Find a variable, load it into a register if not already in one, return the register ID */
int cg_find_variable(value *variable, environment *current_env, int frame_size, int should_attempt_load) {
	if (!variable || (!variable->stored_in_env && !is_constant(variable))) {
		fatal("Could not find variable %s!", correct_string_rep(variable));
	}
	int reg_id = already_in_reg(variable);
	if (reg_id == REG_VALUE_NOT_AVAILABLE) {
		reg_id = choose_best_reg();
		regs[reg_id]->contents = variable;
		if (!should_attempt_load) return reg_id;
		if (is_constant(variable)) {
			append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand(reg_id), make_constant_operand(to_int(NULL, variable)), NULL, "", 1));
			return reg_id;
		}
		/* Support loading local variables */
		if (variable->stored_in_env->static_link == current_env) {
			cg_load_local_var(variable, reg_id);
		}
		else {
			int depth = (current_env->nested_level - variable->stored_in_env->nested_level) + 1;
			int i = 0;
			int num = variable->variable_number;
			int offset_reg = choose_best_reg();
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(reg_id), make_offset_operand($s0, 0), NULL, "Move up a static link", 1));
			for (i = 1; i < depth; i++) {
				append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(reg_id), make_offset_operand(reg_id, 0), NULL, "Move up a static link", 1));
			}
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(offset_reg), make_offset_operand(reg_id, 12), NULL, "Load framesize for static link", 1));
			append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(offset_reg), make_register_operand(offset_reg), make_register_operand(reg_id), "Seek to $fp [end of AR]", 1));
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(reg_id), make_offset_operand(offset_reg, -4 * (num + 1)), NULL, correct_string_rep(variable), 1));
		}
	}
	return reg_id;
}

void cg_store_in_reg(int reg, value *operand, environment *current_env, int frame_size) {
	if (is_constant(operand)) {
		append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand(reg), make_constant_operand(to_int(NULL, operand)), NULL, "", 1));
	}
	else {
		int value_reg = cg_find_variable(operand, current_env, frame_size, 1);
		/* Make the assignment */
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(reg), make_register_operand(value_reg), NULL, "Assign values", 1));	
	}
}

/* Either return the existing register that has been used to store a variable in, or insert into new register */
int get_register(value *variable, environment *current_env, int frame_size, int must_already_exist) {
	int reg_id = cg_find_variable(variable, current_env, frame_size, must_already_exist);
	if (reg_id == REG_VALUE_NOT_AVAILABLE) {
		if (must_already_exist) {
			fatal("Could not find variable %s", correct_string_rep(variable));
		}
		reg_id = choose_best_reg();
		regs[reg_id]->contents = variable;
	}
	return reg_id;
}

/*
* END VARIABLE FNS
*/

/* Generate code for an operation */
void cg_operation(int operation, value *op1, value *op2, value *result, environment *current_env, int frame_size) {
	int result_reg = get_register(result, current_env, frame_size, 0);
	int op1_reg = -1;
	int op2_reg = -1;
	switch(operation) {
		case '+':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			if (is_constant(op2)) {
				append_mips(mips("addi", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand(result_reg), make_register_operand(op1_reg), make_constant_operand(to_int(NULL, op2)), "", 1));
			}
			else {
				op2_reg = get_register(op2, current_env, frame_size, 1);
				append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "", 1));
			}
			break;
		case '-':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			if (is_constant(op2)) {
				append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand(result_reg), make_register_operand(op1_reg), make_constant_operand(to_int(NULL, op2)), "", 1));
			}
			else {
				op2_reg = get_register(op2, current_env, frame_size, 1);
				append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "", 1));
			}
			break;
		case '*':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("mult", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(op1_reg), make_register_operand(op2_reg), NULL, "", 1));
			append_mips(mips("mflo", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(result_reg), NULL, NULL, "", 1));			
			break;	
		case '/':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("div", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(op1_reg), make_register_operand(op2_reg), NULL, "", 1));
			append_mips(mips("mflo", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(result_reg), NULL, NULL, "", 1));
			break;
		case '%':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("div", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(op1_reg), make_register_operand(op2_reg), NULL, "", 1));
			append_mips(mips("mfhi", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(result_reg), NULL, NULL, "", 1));
			break;
		case '<':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			if (is_constant(op2)) {
				append_mips(mips("slti", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand(result_reg), make_register_operand(op1_reg), make_constant_operand(to_int(NULL, op2)), "$c = $a < b", 1));				
			}
			else {
				op2_reg = get_register(op2, current_env, frame_size, 1);
				append_mips(mips("slti", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "$c = $a < $b", 1));											
			}
			break;
		case '>':
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("sgt", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "$c = $a > $b", 1));
			break;
		case LE_OP:
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("sle", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "$c = $a <= $b", 1));
			break;
		case GE_OP:
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("sge", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "$c = $a >= $b", 1));
			break;	
		case EQ_OP:
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("seq", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "$c = $a == $b", 1));
			break;
		case NE_OP:
			op1_reg = get_register(op1, current_env, frame_size, 1);
			op2_reg = get_register(op2, current_env, frame_size, 1);
			append_mips(mips("sne", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "$c = $a != $b", 1));
			break;
		default:
			fatal("Unrecognised operator!");
			break;
	}	
	/* Set modified flag */
	regs[result_reg]->modified = 1;
}

/* Code generate PUSHING a parameter */
void cg_push_param(value *operand, environment *current_env, int frame_size) {
	int reg_id = already_in_reg(operand);
	if (reg_id == REG_VALUE_NOT_AVAILABLE) {
		reg_id = $a0;
		cg_store_in_reg($a0, operand, current_env, frame_size);
	}
	append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "Move stack pointer", 1));	
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(reg_id), make_offset_operand($sp, 0), NULL, "Write param into stack", 1));
}

/* Code generate POPPING a parameter */
void cg_pop_param(value *operand) {
	int num = operand->variable_number;
	append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($a0), make_offset_operand($sp, 0), NULL, "Pop the parameter", 1));
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($a0), make_offset_operand($fp, -4 * (num + 1)), NULL, "Write param into heap", 1));
	append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "Move stack pointer", 1));	
}

/* Code generate an assignment */
void cg_assign(value *result, value *operand1, environment *current_env, int frame_size) {
	int result_reg = get_register(result, current_env, frame_size, 1);
	cg_store_in_reg(result_reg, operand1, current_env, frame_size);
	regs[result_reg]->modified = 1;
}

/* Code generate an IF statement */
void cg_if(value *condition, value *true_label, environment *current_env, int frame_size) {
	int condition_register = get_register(condition, current_env, frame_size, 1);
	append_mips(mips("bne", OT_REGISTER, OT_REGISTER, OT_LABEL, make_register_operand(condition_register), make_register_operand($zero), make_label_operand(correct_string_rep(true_label)), "", 1));
}

/* Code generate a fn call */
void cg_fn_call(value *result, value *fn_def, environment *current_env, int frame_size) {
	int result_reg = get_register(result, current_env, frame_size, 0);
	regs[result_reg]->contents = result;	
	regs[result_reg]->modified = 1;	
	/* Pass dynamic link in $a1 */
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($a1), make_register_operand($s0), NULL, "Pass dynamic link", 1));
	append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("_%s", correct_string_rep(fn_def)), NULL, NULL, "", 1));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(result_reg), make_register_operand($v0), NULL, "", 1));
}

/* Generate code to traverse a static link */
void cg_load_static_link(value *caller, value *callee) {
	int i = 0;
	if (!caller || !callee) return;
	if (caller->stored_in_env == callee->stored_in_env) {
		/* Same level */
		append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v0), make_offset_operand($s0, 0), NULL, "Point callee to same static link as mine (caller)", 1));
	}
	else if (caller->stored_in_env == callee->stored_in_env->static_link) {
		/* Directly nested */
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v0), make_register_operand($s0), NULL, "Set this current activation record as the static link", 1));			
	}
	else {
		int diff = depth_difference(caller->stored_in_env, callee->stored_in_env);
		append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v0), make_offset_operand($s0, 0), NULL, "Move up one static link", 1));
		for (i=0; i < diff; i++) {
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v0), make_offset_operand($v0, 0), NULL, "Point callee to same static link as mine (caller)", 1));
		}
	}
}

/* Clear registers */
void clear_regs() {
	int i = 0;
	for (i = 0; i < REG_COUNT; i++) {
		/* Do not save constants and only save back modified values */
		if (regs[i]->contents) {
			regs[i]->contents = NULL;
			regs[i]->accesses = 0;
			regs[i]->assignment_id = 0;
			/* Modified value saved */
			regs[i]->modified = 0;
		}
	}
}

/* Save pertinent $t regs before a fn call in case they get overwritten */
void save_t_regs(environment *current_env) {
	int i = 0;
	for (i = 0; i < REG_COUNT; i++) {
		/* Do not save constants and only save back modified values */
		if (regs[i]->contents && regs[i]->modified && regs[i]->contents->stored_in_env) {
			if (regs[i]->contents->stored_in_env->static_link == current_fn->stored_in_env) {
				append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(i), make_offset_operand($fp, -4 * (regs[i]->contents->variable_number + 1)), NULL, "Write out used local variable", 1));
			}
			else {
				value *variable = regs[i]->contents;
				int reg_id = choose_best_reg();
				int depth = (current_env->nested_level - variable->stored_in_env->nested_level) + 1;
				int x = 0;
				int num = variable->variable_number;
				append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(reg_id), make_offset_operand($s0, 0), NULL, "Move up a static link", 1));
				for (x = 1; x < depth; x++) {
					append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(reg_id), make_offset_operand(reg_id, 0), NULL, "Move up a static link", 1));
				}
				append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s6), make_offset_operand(reg_id, 12), NULL, "Load framesize for static link", 1));
				append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand($s6), make_register_operand($s6), make_register_operand(reg_id), "Seek to $fp [end of AR]", 1));			
				append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(i), make_offset_operand($s6, -4 * (num + 1)), NULL, "Save distant modified variable", 1));
			}
			/* Modified value saved */
			regs[i]->modified = 0;
		}
	}
}

/* Write out code */
void write_code(tac_quad *quad) {
	/* No parameters, 0 = 1 parameter (0 because $a0 is first arg register), so -1 is no args */
	int depth_difference = 0;
	int size = 0;
	int temporary;
	static int begins_seen = 0;
	static int ends_seen = 0;	
	if (!quad) return;
	/* Reorder the TAC so that inner fns are moved out */
	if (nesting_level > 0) {
		/* Copy the TAC quad, in order to discard the next ptr */
		tac_quad *new_quad = make_quad_value(quad->op, quad->operand1, quad->operand2, quad->result, quad->type, quad->subtype);
		if (!pending_code) {
			pending_code = new_quad;
		}
		else {
			/* Append the TAC */
			tac_quad *tmp_quad = pending_code;
			while (tmp_quad->next != NULL) {
				tmp_quad = tmp_quad->next;
			}
			tmp_quad->next = new_quad;
		}
		if (quad->type == TT_BEGIN_FN) {;
			begins_seen++;
		}
		if (quad->type == TT_END_FN) {
			ends_seen++;			
		}
		if (begins_seen == ends_seen) {
			/* Decrement nesting_level */
			--nesting_level;
		}
		write_code(quad->next);
		return;
	}
	begins_seen = 0;
	ends_seen = 0;
	switch(quad->type) {
		case TT_FN_DEF:
			break;
		case TT_INIT_FRAME:
			size = to_int(NULL, quad->operand1);
			size = activation_record_size(size);
			frame_size = size;
			/* Save return address in $s7 */
			append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($s7), make_register_operand($ra), NULL, "Store Return address in $s7", 1));
			/* Make activation record of required size */
			append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($a0), make_constant_operand(frame_size), NULL, "Store the frame size required for this AR", 1));
			append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("mk_ar"), NULL, NULL, "", 1));
			/* Store a reference to activation record address in $s0 */
			append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($s0), make_register_operand($v0), NULL, "Store heap start address in $s0", 1));
			break;
		case TT_FN_BODY:
			/* Save return address in stack */
			append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "", 1));
			append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s7), make_offset_operand($sp, 0), NULL, "Save return address in stack", 1));
			break;
		case TT_BEGIN_FN:
			nesting_level++;
			if (nesting_level > 0) {
				/* Store this node for later processing, if this is a nested fn */
				write_code(quad);
				return;
			}
			if (strcmp(correct_string_rep(quad->operand1), "main")==0) entry_point = quad; 
			current_fn = quad->operand1;
			append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("_%s", correct_string_rep(quad->operand1)), NULL, NULL, "", 0));
			param_number = -1;
			break;
		case TT_GOTO:
			append_mips(mips("j", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand(correct_string_rep(quad->operand1)), NULL, NULL, "", 1));
			break;
		case TT_POP_PARAM:
			++param_number;
			cg_pop_param(quad->operand1);
			break;
		case TT_LABEL:
			append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand(correct_string_rep(quad->operand1)), NULL, NULL, "", 0));
			break;
		case TT_ASSIGN:
			cg_assign(quad->result, quad->operand1, current_fn->stored_in_env, frame_size);
			break;
		case TT_PUSH_PARAM:
			++param_number;
			cg_push_param(quad->operand1, current_fn->stored_in_env, frame_size);
			break;
		case TT_PREPARE:
			param_number = -1;
			break;
		case TT_IF:
			cg_if(quad->operand1, quad->result, current_fn->stored_in_env, frame_size);
			break;
		case TT_OP:
			cg_operation(quad->subtype, quad->operand1, quad->operand2, quad->result, current_fn->stored_in_env, frame_size);
			break;
		case TT_FN_CALL:
			/* Reset param count */
			param_number = -1;			
			/* Wire out live registers into memory, in-case they're overwritten */
			save_t_regs(current_fn->stored_in_env);
			clear_regs();
			if (!quad->operand1->data.func || !quad->operand1->data.func->node_value) {
				/* Dealing with fn variable - we can deduce its entry point & static link from */
				/* runtime stored information */
				int fn_variable = get_register(quad->operand1, current_fn->stored_in_env, frame_size, 1);
				int result_reg = get_register(quad->result, current_fn->stored_in_env, frame_size, 0);
				int address_reg = choose_best_reg();
				append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(address_reg), make_offset_operand(fn_variable, 0), NULL, "Get Fn address", 1));
				append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v0), make_offset_operand(fn_variable, 4), NULL, "Get static link", 1));
				regs[result_reg]->contents = quad->result;	
				regs[result_reg]->modified = 1;	
				/* Pass dynamic link in $a1 */
				append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($a1), make_register_operand($s0), NULL, "Pass dynamic link", 1));
				append_mips(mips("jalr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(address_reg), NULL, NULL, "", 1));
				append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(result_reg), make_register_operand($v0), NULL, "", 1));
			}
			else {
				/* Work out what static link to pass */
				cg_load_static_link(current_fn, quad->operand1);
				cg_fn_call(quad->result, quad->operand1, current_fn->stored_in_env, frame_size);		
			}
			break;
		case TT_END_FN:
			nesting_level--;		
			/* Save regs */
			save_t_regs(current_fn->stored_in_env);
			clear_regs();
			/* Load return address from stack */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($ra), make_offset_operand($sp, 0), NULL, "Get return address", 1));
			append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "Pop return address from stack", 1));
			append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v0), make_register_operand($zero), NULL, "Null return value", 1));
			/* Load previous frame pointer */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($fp), make_offset_operand($s0, 4), NULL, "Load previous frame ptr", 1));
			/* Load previous static link */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s0), make_offset_operand($s0, 8), NULL, "Load dynamic link", 1));
			append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "Jump to $ra", 1));
			break;
		case TT_RETURN:
			/* Save the return value */
			/* Restore the activation record */
			if (quad->operand1) {
				if (quad->operand1->value_type==VT_FUNCTN) {
					/* Return address to function */
					append_mips(mips("la", OT_REGISTER, OT_LABEL, OT_UNSET, make_register_operand($v0), make_label_operand("_%s", correct_string_rep(quad->operand1)), NULL, "Store address of function", 1));
					append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v1), make_register_operand($s0), NULL, "Store static link to call with", 1));
					append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("rfunc"), NULL, NULL, "Register fn variable", 1));
					/* $v0 now contains fn descriptor, can be used to execute the function in the right way */
					has_used_fn_variable = 1;
				}
				else {
					cg_store_in_reg($v0, quad->operand1, current_fn->stored_in_env, frame_size);
				}
			}
			/* Save regs */
			save_t_regs(current_fn->stored_in_env);
			clear_regs();
			/* Load return address from stack */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($ra), make_offset_operand($sp, 0), NULL, "Get return address", 1));
			append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "Pop return address from stack", 1));
			/* Load previous frame pointer */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($fp), make_offset_operand($s0, 4), NULL, "Load previous frame ptr", 1));
			/* Load previous static link */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s0), make_offset_operand($s0, 8), NULL, "Load dynamic link", 1));
			append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "Jump to $ra", 1));
			break;
		default:
			fatal("Unrecognised TAC quad");
			break;
	}
	write_code(quad->next);
}

/* Reset global variables to their defaults before running write_code each time */
void reset_globals() {
	param_number = -1;
	frame_size = 0;
	nesting_level = -1;
	current_fn = NULL;
}

/* Generate MIPS code for given tree */
void code_gen(NODE *tree) {
	if (!tree) {
		fatal("Invalid input");
	}
	has_used_fn_variable = 0;
	regs = (register_contents **) malloc(sizeof(register_contents *) * REG_COUNT);
	init_register_view();
	print_register_view();
	tac_quad *quad = start_tac_gen(tree);
	write_preamble();
	reset_globals();
	write_code(quad);
	reset_globals();
	/* Write out inner fns separately */
	write_code(pending_code);
	write_activation_record_fn();
	if (has_used_fn_variable) write_register_fn_variable();
	write_epilogue();	
	print_mips(instructions);
}