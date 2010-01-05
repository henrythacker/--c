#include "registers.h"

/**
*	registers.c by Henry Thacker
*
*	Utilities to work with state of MIPS registers while code generation takes place
*
*/

/* Initialise our global view of register allocation */
void init_register_view(register_contents **regs) {
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
void print_register_view(register_contents **regs) {
	int i;
	if (!DEBUG_ON) return;
	printf("Register View\n---------------\n");
	for (i = 0; i < REG_COUNT; i++) {
		if (register_use_allowed(i) || is_argument_register(i)) {
			printf("[%s] \t- Contents: [%p - %s] - Modified: %d - Accesses: %d - Assignment order: %d\n", register_name(i), regs[i]->contents, regs[i]->contents ? regs[i]->contents->identifier : "EMPTY", regs[i]->modified, regs[i]->accesses, regs[i]->assignment_id);
		}
	}
}

/* Clear registers */
void clear_regs(register_contents **regs) {
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

/* Save a specific $t reg, if applicable */
void save_t_reg(register_contents **regs, int i, environment *current_env) {
	/* Do not save constants and only save back modified values */
	if (regs[i]->contents && regs[i]->modified && regs[i]->contents->stored_in_env) {
		if (regs[i]->contents->stored_in_env->static_link == current_env) {
			append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand(i), make_offset_operand($fp, -4 * (regs[i]->contents->variable_number + 1)), NULL, "Write out used local variable", 1));
		}
		else {
			value *variable = regs[i]->contents;
			int reg_id = choose_best_reg(regs, current_env);
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

/* Save pertinent $t regs before a fn call in case they get overwritten */
void save_t_regs(register_contents **regs, environment *current_env) {
	int i = 0;
	for (i = 0; i < REG_COUNT; i++) {
		save_t_reg(regs, i, current_env);
	}
}

/* Find the first free register, if any */
int first_free_reg(register_contents **regs) {
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
int choose_best_reg(register_contents **regs, environment *env) {
	
	int free_reg = first_free_reg(regs);
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
		/* Move out existing value that is being spilled */
		if (regs[optimal_reg]->contents->stored_in_env) {
			save_t_reg(regs, optimal_reg, env);
		}
		regs[optimal_reg]->assignment_id = ++regs_assignments;
		regs[optimal_reg]->contents = NULL;
		regs[optimal_reg]->accesses = 1;	
		regs[optimal_reg]->modified = 0;			
		return optimal_reg;
	}
	return free_reg;
}

/* Is the value already in a register?? */
int already_in_reg(register_contents **regs, value *var, environment *env, int *has_used_fn_variable) {
	int position = 0;
	/* Check if this is a well known function, if so, pass back the address of the label in a register */
	if (var->value_type == VT_FUNCTN && var->data.func && var->data.func->node_value) {
		position = choose_best_reg(regs, env);
		/* Return address to function */
		append_mips(mips("la", OT_REGISTER, OT_LABEL, OT_UNSET, make_register_operand($v0), make_label_operand("_%s", correct_string_rep(var)), NULL, "Store address of function", 1));
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($v1), make_register_operand($s0), NULL, "Store static link to call with", 1));
		append_mips(mips("jal", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("rfunc"), NULL, NULL, "Register fn variable", 1));
		/* $v0 now contains fn descriptor, can be used to execute the function in the right way */
		append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand(position), make_register_operand($v0), NULL, "Return fn descriptor address", 1));
		*has_used_fn_variable = 1;
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