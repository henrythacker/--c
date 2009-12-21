/*Result: 34*/

/*
*	Sample --C Fibonacci Programme
*	Henry Thacker
*/

int fib(int n) {
	if (n < 2) 
		return n;
	else
		return fib(n - 1) + fib(n - 2);
}

/* Main entry point */
int main() {
	return fib(9);
}