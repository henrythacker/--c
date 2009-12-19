#include "utilities.h"

/* Assertion with error text */
void assert(int assertion, char *error) {
	if (!assertion) {
		printf("TAC Error: %s\n", error);
		exit(-1);
	}
}