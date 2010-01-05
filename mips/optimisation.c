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

/* Perform optimisation functions */
mips_instruction *do_optimise(mips_instruction *input) {
	input = remove_after_return(input);
	return input;
}