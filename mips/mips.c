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
	}
}

/* Print the view of the registers */
void print_register_view() {
	int i;
	if (!DEBUG_ON) return;
	printf("Register View\n---------------\n");
	for (i = 0; i < REG_COUNT; i++) {
		if (register_use_allowed(i) || is_argument_register(i)) {
			printf("[%s] \t- Contents: [%p - %s] - Accesses: %d - Assignment order: %d\n", register_name(i), regs[i]->contents, regs[i]->contents ? regs[i]->contents->identifier : "EMPTY", regs[i]->accesses, regs[i]->assignment_id);
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
		return optimal_reg;
	}
	return free_reg;
}

/* Is the value already in a register?? */
int already_in_reg(value *var) {
	int position = 0;
	for (position = 0; position < REG_COUNT; position++) {
		/* Point to same variable OR are equivalent constants */
		/* Must be sourced from a user accessible register, or the special zero register */
		if (register_use_allowed(position) || position == 0 || is_argument_register(position)) {
			if (regs[position]->contents == var || (is_constant(var) && regs[position]->contents && is_constant(regs[position]->contents) && to_int(NULL, var) == to_int(NULL, regs[position]->contents))) {
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
	int special_fields = 2; 
	int allocation_size = (word_size * local_size) + (word_size * special_fields);
	return allocation_size;
}

/* Outputs fn that is used to build a new activation record, local_size passed in $a0 */
void write_activation_record_fn() {
	/* Make label */
	operand *comment;
	comment = make_label_operand("# Make a new activation record\n# Precondition: $a0 contains total required heap size, $v0 contains static link\n# Returns: start of heap address in $v0, heap contains reference to static link and old $fp value");
	append_mips(mips_comment(comment, 0));
	append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("mk_ar"), NULL, NULL, "", 0));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($t8), make_constant_operand($v0), NULL, "Backup static link in $t8", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(9), NULL, "Allocate space systemcode", 1));
	append_mips(syscall("Allocate space on heap"));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($t9), make_constant_operand($fp), NULL, "Backup old $fp in $t9", 1));
	/* Point $fp to the correct place */
	append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand($fp), make_register_operand($v0), make_register_operand($a0), "$fp = heap start address + heap size", 1));
	/* Save static link */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($t8), make_offset_operand($fp, -4), NULL, "Save static link", 1));
	/* Save old $fp */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($t9), make_offset_operand($fp, -8), NULL, "Save old $fp", 1));	
	/* Jump back */
	append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "", 1));	
}

/* Load a variable in local scope */
void cg_load_local_var(value *var, int destination_register) {
	int num = var->variable_number;
	append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(destination_register), make_offset_operand($fp, -4 * (num + 1)), NULL, "Load local variable", 1));
}

/* Generate code for an operation */
void cg_operation(int operation, value *op1, value *op2, value *result, int current_depth, int frame_size) {
	/*int result_reg = which_register(result, 0, current_depth, frame_size);
	regs[result_reg]->contents = result;	
	int op1_reg = which_register(op1, 1, current_depth, frame_size);
	regs[op1_reg]->contents = op1;	
	int op2_reg = which_register(op2, 1, current_depth, frame_size);	
	regs[op2_reg]->contents = op2;
	switch(operation) {
		case '+':
			append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "", 1));
			break;
		case '-':
			append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand(result_reg), make_register_operand(op1_reg), make_register_operand(op2_reg), "", 1));
			break;
		case '*':
			append_mips(mips("mul", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(op1_reg), make_register_operand(op2_reg), NULL, "", 1));
			append_mips(mips("mflo", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(result_reg), NULL, NULL, "", 1));			
			break;	
		case '/':
			append_mips(mips("div", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(op1_reg), make_register_operand(op2_reg), NULL, "", 1));
			append_mips(mips("mflo", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(result_reg), NULL, NULL, "", 1));
			break;
		case '%':
			append_mips(mips("div", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(op1_reg), make_register_operand(op2_reg), NULL, "", 1));
			append_mips(mips("mfhi", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand(result_reg), NULL, NULL, "", 1));
			break;
	}*/
}

/* Code generate PUSHING a parameter */
void cg_push_param(value *operand, int param_number, int current_depth, int frame_size) {
	/*if (param_number > 4) {
		// TO DO: Store at end of heap
	}
	else {
		// TO DO: If value is not empty, save it in current frame
		int operand_reg = which_register(operand, 1, current_depth, frame_size);
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($a0 + param_number), make_register_operand(operand_reg), NULL, "Push operand", 1));
	}*/
}

/* Code generate POPPING a parameter */
void cg_pop_param(value *operand, int param_number) {
	append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("pop"), NULL, NULL, "", 0));
}

/* Code generate an assignment */
void cg_assign(value *result, value *operand1, int current_depth, int frame_size) {
	/*int result_reg = which_register(result, 0, current_depth, frame_size);
	regs[result_reg]->contents = result;*/
}

/* Code generate an IF statement */
void cg_if(value *condition, value *true_label, int current_depth, int frame_size) {
	/*int condition_register = which_register(condition, 1, current_depth, frame_size);
	append_mips(mips("bne", OT_REGISTER, OT_REGISTER, OT_LABEL, make_register_operand(condition_register), make_register_operand($zero), make_label_operand(correct_string_rep(true_label)), "", 1));*/
}

/* Code generate a fn call */
void cg_fn_call(value *result, value *fn_def, int current_depth, int frame_size) {
	/*int result_reg = which_register(result, 0, current_depth, frame_size);
	regs[result_reg]->contents = result;	
	append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("_%s", correct_string_rep(fn_def)), NULL, NULL, "", 1));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(result_reg), make_register_operand($v0), NULL, "", 1));*/
}

/* Generate code to traverse a static link */
void cg_load_static_link(int depth, int frame_size) {
	int i = 0;
	/*
	* If depth_difference:
	*	is 0 - pass in the same static link that we have in our current activation record (look up -8($s0))
	*	is 1 - pass in the current $fp
	*	otherwise - traverse this number of stack frames (lw $temp, -8($s0) depth difference amount of times, then move to $v0)
	*/
	switch(depth) {
		case 0:
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v0), make_offset_operand($s0, frame_size - 8), NULL, "Point callee to same static link as mine (caller)", 1));
			break;
		case 1:
			append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v0), make_register_operand($fp), NULL, "Set this current activation record as the static link", 1));			
			break;
		default:
			for (i = 0; i < depth; i++) {
				append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s3), make_offset_operand($s0, frame_size - 8), NULL, "Move up static link", 1));
			}
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v0), make_offset_operand($s0, frame_size - 8), NULL, "Final static link found", 1));
			break;
	}
}

/* Write out code */
void write_code(tac_quad *quad) {
	/* No parameters, 0 = 1 parameter (0 because $a0 is first arg register), so -1 is no args */
	static int param_number = -1;
	static int frame_size = 0;
	static int nesting_level = -1;
	static value *current_fn = NULL;
	int depth_difference = 0;
	int size = 0;
	int temporary;
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
		if (quad->type == TT_END_FN) {
			nesting_level--;		
		}
		write_code(quad->next);
		return;
	}
	switch(quad->type) {
		case TT_FN_DEF:
			nesting_level++;
			break;
		case TT_INIT_FRAME:
			/* Verified: HT */
			size = to_int(NULL, quad->operand1);
			size = activation_record_size(size);
			frame_size = size;
			/* Make activation record of required size */
			append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($a0), make_constant_operand(frame_size), NULL, "Store the frame size required for this AR", 1));
			append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("mk_ar"), NULL, NULL, "", 1));
			/* Store a reference to activation record address in $s0 */
			append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($s0), make_register_operand($v0), NULL, "Store heap end address in $s0", 1));
			break;
		case TT_BEGIN_FN:
			/* Verified: HT */
			if (strcmp(correct_string_rep(quad->operand1), "main")==0) entry_point = quad;
			current_fn = quad->operand1;
			/* Save return address in stack */
			append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("_%s", correct_string_rep(quad->operand1)), NULL, NULL, "", 0));
			append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "", 1));
			append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($ra), make_offset_operand($sp, 0), NULL, "Save return address in stack", 1));
			param_number = -1;
			break;
		case TT_GOTO:
			append_mips(mips("j", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand(correct_string_rep(quad->operand1)), NULL, NULL, "", 1));
			break;
		case TT_POP_PARAM:
			cg_pop_param(quad->operand1, ++param_number);
			break;
		case TT_LABEL:
			append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand(correct_string_rep(quad->operand1)), NULL, NULL, "", 0));
			break;
		case TT_ASSIGN:
			cg_assign(quad->result, quad->operand1, current_fn->stored_in_env->nested_level, frame_size);
			break;
		case TT_PUSH_PARAM:
			cg_push_param(quad->operand1, ++param_number, current_fn->stored_in_env->nested_level, frame_size);
			break;
		case TT_PREPARE:
			param_number = -1;
			break;
		case TT_IF:
			cg_if(quad->operand1, quad->result, current_fn->stored_in_env->nested_level, frame_size);
			break;
		case TT_OP:
			cg_operation(quad->subtype, quad->operand1, quad->operand2, quad->result, current_fn->stored_in_env->nested_level, frame_size);
			break;
		case TT_FN_CALL:
			/* Reset param count */
			param_number = -1;
			/* Work out what static link to pass */
			depth_difference = current_fn->stored_in_env->nested_level - quad->operand1->stored_in_env->nested_level;
			cg_load_static_link(depth_difference, frame_size);
			cg_fn_call(quad->result, quad->operand1, current_fn->stored_in_env->nested_level, frame_size);			
			break;
		case TT_END_FN:
			nesting_level--;		
			/* Load return address from stack */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($ra), make_offset_operand($sp, 0), NULL, "Get return address", 1));
			append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "Pop return address from stack", 1));
			append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v0), make_register_operand($zero), NULL, "Null return value", 1));
			/* Load previous frame pointer */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($fp), make_offset_operand($s0, frame_size - 4), NULL, "Load previous frame ptr", 1));
			append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "Jump to $ra", 1));
			break;
		case TT_RETURN:
			/* Save the return value */
			/* Restore the activation record */
			if (quad->operand1) {
				//cg_assign_to_reg($v0, quad->operand1, current_fn->stored_in_env->nested_level, frame_size);
			}
			/* Load return address from stack */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($ra), make_offset_operand($sp, 0), NULL, "Get return address", 1));
			append_mips(mips("sub", OT_REGISTER, OT_REGISTER, OT_CONSTANT, make_register_operand($sp), make_register_operand($sp), make_constant_operand(4), "Pop return address from stack", 1));
			/* Load previous frame pointer */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($fp), make_offset_operand($s0, frame_size - 4), NULL, "Load previous frame ptr", 1));
			/* Load previous heap pointer */
			append_mips(mips("lw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s0), make_offset_operand($s0, frame_size - 8), NULL, "Load static link", 1));
			append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "Jump to $ra", 1));
			break;
		default:
			fatal("Unrecognised TAC quad");
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
	/* Write out inner fns separately */
	write_code(pending_code);
	write_activation_record_fn();
	write_epilogue();	
	print_mips(instructions);
}