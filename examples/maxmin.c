/*
*	Sample --C max/min integer Programme
*	Henry Thacker
*/

int max(int a, int b) {
	if (a >= b) {
		return a;
	}
	return b;
}

int min(int a, int b) {
	if (a <= b) {
		return a;
	}
	return b;
}


int main() {
	int a = 5, b = 9;
	return max(a, b);
}