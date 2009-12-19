int main() {
	int a = 10;
	if (1) {
		a = 5;
	}
	else {
		a = 9;
	}
	if (0) {
		a = a - 1;
	}
	else {
		a = a - 3;
	}
	return a;
}