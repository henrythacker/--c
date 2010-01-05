#include "codegen_utils.h"

/* Print out the operand value */
void print_mips_operand(mips_instruction *instruction, int operand) {
	int type;
	union operand *operand_value;
	int more_operands = 0;
	switch(operand) {
		case 1:
			type = instruction->operand1_type;
			operand_value = instruction->operand1;
			more_operands = instruction->operand2 && instruction->operand2_type != OT_UNSET;
			break;
		case 2:
			type = instruction->operand2_type;
			operand_value = instruction->operand2;		
			more_operands = instruction->operand3 && instruction->operand3_type != OT_UNSET;
			break;		
		case 3:
			type = instruction->operand3_type;
			operand_value = instruction->operand3;
			more_operands = 0;		
			break;
	}
	if (!operand_value || type==OT_UNSET) return;
	switch(type) {
		case OT_REGISTER:
			printf("%s%s", register_name(operand_value->reg), more_operands ? ", " : "");
			break;
		case OT_OFFSET:
			if (operand_value->reg_offset->offset == 0) {
				printf("(%s)%s", register_name(operand_value->reg_offset->reg), more_operands ? ", " : "");
			}
			else {
				printf("%d(%s)%s", operand_value->reg_offset->offset, register_name(operand_value->reg_offset->reg), more_operands ? ", " : "");				
			}
			break;
		case OT_COMMENT:
			if (DEBUG_ON) printf("%s%s", operand_value->label, more_operands ? ", " : "");				
			break;
		case OT_CONSTANT:
			printf("%d%s", operand_value->constant, more_operands ? ", " : "");				
			break;
		case OT_FN_LABEL:
			printf("_%s%s", operand_value->label, more_operands ? ", " : "");				
			break;
		case OT_ZERO_ADDRESS:
		case OT_LABEL:						
			printf("%s%s", operand_value->label, more_operands ? ", " : "");				
			break;	
	}
}

/* Write MIPS code */
void print_mips(mips_instruction *instructions) {
	int i = 0;
	if (!instructions) return;
	for (i = 0; i < instructions->indent_count; i++) {
		/* Indent as requested */
		printf("\t");
	}
	/* Check to see if we have a single operand by itself (with no operation specified) - needs printing in a different way*/
	if (instructions->operation && strlen(instructions->operation) == 0 && instructions->operand1_type != OT_UNSET && instructions->operand2_type == OT_UNSET && instructions->operand3_type == OT_UNSET) {
		print_mips_operand(instructions, 1);
		if (instructions->operand1_type != OT_ZERO_ADDRESS && instructions->operand1_type != OT_COMMENT) printf(":");
	}
	else {
		printf("%s ", instructions->operation);
		print_mips_operand(instructions, 1);
		print_mips_operand(instructions, 2);	
		print_mips_operand(instructions, 3);		
	}
	if (DEBUG_ON && instructions->comment && strlen(instructions->comment) > 0) {
		printf("\t# %s\n", instructions->comment);
	}
	else {
		printf("\n");
	}
	print_mips(instructions->next);
}

/* Create a new mips instruction */
mips_instruction *mips(char *operation, int op1type, int op2type, int op3type, operand *op1, operand *op2, operand *op3, char *comments, int indent_count) {
	mips_instruction *instruction = (mips_instruction *)malloc(sizeof(mips_instruction));
	/* Copy operation */
	instruction->operation = malloc(sizeof(char) * (strlen(operation) + 1));
	strcpy(instruction->operation, operation);
	/* Copy comments */
	instruction->comment = malloc(sizeof(char) * (strlen(comments) + 1));
	strcpy(instruction->comment, comments);
	/* Assign operands */
	instruction->operand1_type = op1type;
	instruction->operand2_type = op2type;
	instruction->operand3_type = op3type;
	instruction->operand1 = op1;
	instruction->operand2 = op2;
	instruction->operand3 = op3;
	/* Set indentation for printing */
	instruction->indent_count = indent_count;
	instruction->next = NULL;
	return instruction;
}

mips_instruction *syscall(char *comment) {
 return mips("", OT_ZERO_ADDRESS, OT_UNSET, OT_UNSET, make_label_operand("syscall"), NULL, NULL, comment, 1);
}

/* Generate a pseudo-instruction to hold a comment */
mips_instruction *mips_comment(operand *comment, int indent_count) {
	mips_instruction *instruction = (mips_instruction *)malloc(sizeof(mips_instruction));
	instruction->operation = "";
	/* Comment moved into a special operand */
	instruction->comment = "";
	instruction->operand1_type = OT_COMMENT;
	instruction->operand2_type = OT_UNSET;
	instruction->operand3_type = OT_UNSET;		
	instruction->operand1 = comment;
	/* Set indentation for printing */
	instruction->indent_count = indent_count;	
	instruction->next = NULL;	
	return instruction;
}

operand *new_operand() {
	return (operand *)malloc(sizeof(operand));
}

operand *make_constant_operand(int constant) {
	operand *tmp = new_operand();
	tmp->constant = constant;
	return tmp;
}

/* Register specific operand creators */

operand *make_register_operand(int reg_identifier) {
	operand *tmp = new_operand();
	tmp->reg = reg_identifier;
	return tmp;
}

operand *make_offset_operand(int reg_identifier, int offset_amount) {
	operand *tmp = new_operand();
	register_offset *offset = (register_offset *)malloc(sizeof(register_offset));
	offset->reg = reg_identifier;
	offset->offset = offset_amount;
	tmp->reg_offset = offset;
	return tmp;
}

/* End Register specific operand creators */

operand *make_label_operand(char *label, ...) {
	va_list arglist;
	operand *tmp = new_operand();
	char *bigstring = malloc(sizeof(char) * 300);
	int i;
	va_start(arglist, label);
	vsprintf(bigstring, label, arglist);
  	va_end(arglist);
	/* Copy label */
	tmp->label = malloc(sizeof(char) * (strlen(bigstring) + 1));
	strcpy(tmp->label, bigstring);
	free(bigstring);
	return tmp;
}

/* RUNTIME SUPPORT FNs */

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

/* Write stub fn to call into the user code and return the value */
void write_epilogue() {
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

/* Outputs fn that is used to build a new activation record, local_size passed in $a0 */
void write_activation_record_fn() {
	/* Make label */
	operand *comment;
	comment = make_label_operand("# Make a new activation record\n# Precondition: $a0 contains total required heap size, $a1 contains dynamic link, $v0 contains static link\n# Returns: start of heap address in $v0, heap contains reference to static link and old $fp value");
	append_mips(mips_comment(comment, 0));
	append_mips(mips("", OT_LABEL, OT_UNSET, OT_UNSET, make_label_operand("mk_ar"), NULL, NULL, "", 0));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($s1), make_constant_operand($v0), NULL, "Backup static link in $s1", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(9), NULL, "Allocate space systemcode", 1));
	append_mips(syscall("Allocate space on heap"));
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($s2), make_constant_operand($fp), NULL, "Backup old $fp in $s2", 1));
	/* Point $fp to the correct place */
	append_mips(mips("add", OT_REGISTER, OT_REGISTER, OT_REGISTER, make_register_operand($fp), make_register_operand($v0), make_register_operand($a0), "$fp = heap start address + heap size", 1));
	/* Save static link */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s1), make_offset_operand($v0, 0), NULL, "Save static link", 1));
	/* Save old $fp */
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s2), make_offset_operand($v0, 4), NULL, "Save old $fp", 1));	
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
	append_mips(mips("move", OT_REGISTER, OT_REGISTER, OT_UNSET, make_register_operand($s1), make_constant_operand($v0), NULL, "Backup fn address in $s1", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($a0), make_constant_operand(8), NULL, "Space required for descriptor", 1));
	append_mips(mips("li", OT_REGISTER, OT_CONSTANT, OT_UNSET, make_register_operand($v0), make_constant_operand(9), NULL, "Allocate space systemcode", 1));
	append_mips(syscall("Allocate space on heap"));
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($s1), make_offset_operand($v0, 0), NULL, "Store fn address", 1));
	append_mips(mips("sw", OT_REGISTER, OT_OFFSET, OT_UNSET, make_register_operand($v1), make_offset_operand($v0, 4), NULL, "Store static link", 1));
	append_mips(mips("jr", OT_REGISTER, OT_UNSET, OT_UNSET, make_register_operand($ra), NULL, NULL, "", 1));	
}

/* END RUNTIME SUPPORT FNs */

/* Get the friendly register name from the enum value */
char *register_name(enum sys_register reg_id) {
	/* Friendly names for registers */
	static char *rgs[] = {	"$zero",
						 	"$at",
						 	"$v0", "$v1",
						 	"$a0", "$a1", "$a2", "$a3",
						 	"$t0", "$t1", "$t2", "$t3", "$t4", "$t5", "$t6", "$t7",
						 	"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7",
						 	"$t8", "$t9",
						 	"$k0", "$k1",
						 	"$gp",
						 	"$sp",
						 	"$fp",
						 	"$ra"
						};
	int register_names = sizeof(rgs) / sizeof(char *);
	if (reg_id < 0 || reg_id > register_names) return "";
	return rgs[reg_id];
}