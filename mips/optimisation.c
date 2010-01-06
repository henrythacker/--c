#include "optimisation.h"

/**
*	optimisation.c by Henry Thacker
*
*	Basic code optimiser for MIPS code generation
*
*/

/* Should I continue removing nodes after a return statement, given the contents of the current node? */
int continue_removing_nodes(mips_instruction *current) {
	return !(current && current->operand1 && (current->operand1_type == OT_COMMENT || current->operand1_type == OT_FN_LABEL || current->operand1_type == OT_LABEL));
}

/* Remove all statements after a return (JR) statement up until a label, function label or comment */
/* Rationale: Statements after jumps will never be executed */
mips_instruction *remove_after_return(mips_instruction *input) {
	mips_instruction *instruction;
	mips_instruction *previous_instruction;
	int deleting = 0;
	previous_instruction = NULL;
	instruction = input;
	while (instruction) {
		if (previous_instruction && deleting && continue_removing_nodes(instruction)) {
			previous_instruction->next = instruction->next;
			/* We don't need this instruction any more */
			instruction = NULL;
			free(instruction);
			instruction = previous_instruction->next;
		}
		else {
			/* Do we need to delete after this node? i.e. jump return, or jump */
			deleting = strcmp(instruction->operation, "jr")==0 || strcmp(instruction->operation, "j")==0;
			previous_instruction = instruction;
			instruction = instruction->next;
		}
	}
	return input;
}

/* Due to the way the code generation happens, often one can find the pattern move $X, $v0 followed directly by move $v0, $X */
/* Remove the redundant second line, the first is enough */
/* Rationale: remove redundant code */
mips_instruction *remove_redundant_move(mips_instruction *input) {
	mips_instruction *c_ins;
	mips_instruction *p_ins;
	p_ins = NULL;
	c_ins = input;
	while (c_ins) {
		if (c_ins && p_ins) {
			int prev;
			int curr;
			prev = strcmp(p_ins->operation, "move")==0 && p_ins->operand2 && p_ins->operand2_type == OT_REGISTER && p_ins->operand2->reg == $v0;
			curr = strcmp(c_ins->operation, "move")==0 && c_ins->operand1 && c_ins->operand1_type == OT_REGISTER && c_ins->operand1->reg == $v0;
			if (prev && curr && p_ins->operand2->reg == c_ins->operand1->reg) {
				/* Target couple of instructions found */
				/* Circumvent the current instruction */
				p_ins->next = c_ins->next;
				free(c_ins);
				c_ins = p_ins->next;
				p_ins = c_ins->next;
				continue;
			}
		}
		p_ins = c_ins;
		c_ins = c_ins->next;
	}
	return input;
}

/* Due to the way the code generation happens, often one can find the pattern add $sp, $sp, 4 followed directly by sub $sp, $sp, 4 */
/* Remove both lines, the effect of both operations in tandem cancel each other out */
/* Rationale: remove redundant code */
mips_instruction *remove_redundant_sp_move(mips_instruction *input) {
	mips_instruction *c_ins, *p_ins, *n_ins;	
	p_ins = NULL;
	c_ins = input;
	n_ins = c_ins->next;
	while (c_ins) {
		if (c_ins && p_ins && n_ins) {
			int condition1, condition2;
			condition1 = strcmp(c_ins->operation, "add")==0 && c_ins->operand1 && c_ins->operand1_type == OT_REGISTER && c_ins->operand1->reg == $sp
							&& c_ins->operand2 && c_ins->operand2_type == OT_REGISTER && c_ins->operand1->reg == $sp && c_ins->operand3
							&& c_ins->operand3_type == OT_CONSTANT && c_ins->operand3->constant == 4;
			condition2 = strcmp(n_ins->operation, "sub")==0 && n_ins->operand1 && n_ins->operand1_type == OT_REGISTER && n_ins->operand1->reg == $sp
							&& n_ins->operand2 && n_ins->operand2_type == OT_REGISTER && n_ins->operand1->reg == $sp && n_ins->operand3
							&& n_ins->operand3_type == OT_CONSTANT && n_ins->operand3->constant == 4;
			if (condition1 && condition2) {
				/* Target couple of instructions found */
				/* Circumvent the current instructions */
				free(c_ins);
				p_ins->next = n_ins->next;
				free(n_ins);
				p_ins = p_ins->next;
				c_ins = p_ins->next;
				continue;
			}
		}
		p_ins = c_ins;
		c_ins = c_ins->next;
		if (c_ins) n_ins = c_ins->next;
	}
	return input;	
}

/* Perform optimisation functions */
mips_instruction *do_optimise(mips_instruction *input) {
	input = remove_after_return(input);
	input = remove_redundant_move(input);
	input = remove_redundant_sp_move(input);
	return input;
}