function twice(function f) { 
	int g(int x) { return f(f(x)); } 
	return g;
}

function add(int a) {
	return a + 1;
}

int main() {
	function twicea = twice(add);
	return twicea(5);
}