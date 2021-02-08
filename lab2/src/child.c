#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

signed main() {
	char c;
	int num = 0, sum = 0, minus = 0;
	while (scanf("%c", &c) > 0) {
		if (c == ' ' || c == '\t') {
			if (minus) {
				sum = sum - num;
			} else {
				sum = sum + num;
			}
			num = 0;
			minus = 0;
		} else if (c == '-') {
			minus = 1;
		} else if (c == '\n') {
			if (minus) {
				sum = sum - num;
			} else {
				sum = sum + num;
			}
			num = 0;
			minus = 0;
			write(STDOUT_FILENO, &sum, sizeof(int));
			sum = 0;
		} else if ('0' <= c && c <= '9') {
			num = num * 10 + c - '0';
		}
	}
}