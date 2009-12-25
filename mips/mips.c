#include "mips.h"

void write_preamble() {
	struct tm *local_time;
	time_t gen_time;
	gen_time = time(NULL);
	local_time = localtime(&gen_time);
	printf("# Compiled from --C\n# %s\n\n", asctime(local_time));
}

/* Generate MIPS code for given tree */
void code_gen(NODE *tree) {
	tac_quad *quad = start_tac_gen(tree);
	write_preamble();
}