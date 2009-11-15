/*
*	Sample --C Factorial Programme
*	Henry Thacker
*/

int fact(int n) {
	int inner_fact(int n, int a) {
		/* Mention how IF stmts need to be in curlies */
		if (n==0) return a;
		return inner_fact(n-1, a * n);
		/* Will execute anything here unfortunately */
	}
	return inner_fact(n, 1);
}

/* Main entry point */
int main() {
	return fact(6);
}