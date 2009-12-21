/*Result: 5*/

int a = 1;

void fn() {
	a = 5;
}

int main() {
	fn();
	return a;
}