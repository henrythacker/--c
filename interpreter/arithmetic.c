#include "arithmetic.h"

/**
*	arithmetic.c by Henry Thacker
*	Version 2 - Rewritten 14/11/2009
*
*	Arithmetic operations for the --C language
*
*/

value *arithmetic(environment *env, int operator, value *operand1, value *operand2) {
	int i_operand1 = to_int(env, operand1);
	int i_operand2 = to_int(env, operand2);	
	switch(operator) {
		case '+':
			return int_value(i_operand1 + i_operand2);
		case '-':
			return int_value(i_operand1 - i_operand2);		
		case '*':
			return int_value(i_operand1 * i_operand2);		
		case '/':
			return int_value(i_operand1 / i_operand2);		
		case '%':
			return int_value(i_operand1 % i_operand2);		
		case '>':
			return int_value(i_operand1 > i_operand2);		
		case '<':
			return int_value(i_operand1 < i_operand2);		
		case NE_OP:
			return int_value(i_operand1 != i_operand2);		
		case EQ_OP:
			return int_value(i_operand1 == i_operand2);		
		case LE_OP:
			return int_value(i_operand1 <= i_operand2);		
		case GE_OP:
			return int_value(i_operand1 >= i_operand2);		
		case '!':
			return int_value(!i_operand1);	
		default:
			return NULL;
	}
}