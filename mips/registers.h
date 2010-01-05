#ifndef __REGS_H
#define __REGS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "environment.h"
#include "codegen_utils.h"

#define REG_COUNT 31

#define REG_NONE_FREE -500
#define REG_VALUE_NOT_AVAILABLE -501

typedef struct register_contents {
	value *contents; /* Value stored in the register */
	int accesses; /* How many times the value has been referenced */
	int assignment_id; /* What order this assignment was made */
	int modified;
}register_contents;

int already_in_reg(register_contents **, value *, environment *env, int *has_used_fn_variable);
int choose_best_reg(register_contents **, environment *);
int first_free_reg(register_contents **);
void save_t_regs(register_contents **, environment *);
void save_t_reg(register_contents **, int, environment *);
void clear_regs(register_contents **);
void print_register_view(register_contents **);
void init_register_view(register_contents **);

/* Count globally how many assignments we've made to a variable */
int regs_assignments;

#endif