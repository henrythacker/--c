#ifndef __CGU_H
#define __CGU_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "environment.h"

#define OT_UNSET 2020
#define OT_REGISTER 3030
#define OT_OFFSET 4040
#define OT_CONSTANT 5050
#define OT_LABEL 6060
#define OT_FN_LABEL 7070
#define OT_ZERO_ADDRESS 8080
#define OT_COMMENT 9090

/* System registers enumeration */
enum sys_register {
	$zero = 0, 
	$at = 1,	
	$v0 = 2, $v1 = 3,
	$a0 = 4, $a1 = 5, $a2 = 6, $a3 = 7,
	$t0 = 8, $t1 = 9, $t2 = 10, $t3 = 11, $t4 = 12, $t5 = 13, $t6 = 14,	$t7 = 15,					
	$s0 = 16, $s1 = 17, $s2 = 18, $s3 = 19, $s4 = 20, $s5 = 21, $s6 = 22, $s7 = 23,
	$t8 = 24, $t9 = 25,
	$k0 = 26, $k1 = 27,
	$gp = 28,
	$sp = 29,
	$fp = 30,
	$ra = 31				
};

typedef struct register_offset {
	enum sys_register reg;
	int offset;
}register_offset;

typedef union operand {
	enum sys_register reg;
	struct register_offset* reg_offset;
	int constant;
	char *label;
}operand;

typedef struct mips_instruction {
	char *operation;
	int operand1_type;
	int operand2_type;
	int operand3_type;
	union operand *operand1;
	union operand *operand2;
	union operand *operand3;		
	char *comment;
	int indent_count;
	struct mips_instruction *next;
}mips_instruction;

mips_instruction *mips_comment(operand *, int);
mips_instruction *mips(char *, int, int, int, operand *, operand *, operand *, char *, int);
operand *make_register_operand(int);
operand *make_offset_operand();
operand *make_constant_operand(int);
operand *make_label_operand(char *, ...);

#endif