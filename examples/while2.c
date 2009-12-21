/*Result: 3*/

/*
*	Sample --C While Loop Programme
*	Henry Thacker
*/

/* Main entry point */
int main() {
	int i = 5;
	while (i > 1)  {
		i = i - 1;
		if (i==4) {
			continue;
		}
		return i;
	}
	return 5;
}