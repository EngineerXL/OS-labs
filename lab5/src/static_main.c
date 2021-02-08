#include "declaration.h"
#include <stdio.h>
#include <stdlib.h>

#define check(VALUE, OKVAL, MSG) if (VALUE != OKVAL) { printf("%s", MSG); return 1; }

int main() {
	int q;
	while (scanf("%d", &q) > 0) {
		if (q == 1) {
			int x, y;
			check(scanf("%d%d", &x, &y), 2, "Error reading integer!\n");
			printf("GCD(%d, %d) = %d\n", x, y, GCD(x, y));
		} else if (q == 2) {
			size_t n;
			check(scanf("%lu", &n), 1, "Error reading integer!\n");
			int* a = malloc(sizeof(int) * n);
			for (size_t i = 0; i < n; ++i) {
				check(scanf("%d", &a[i]), 1, "Error reading integer\n");
			}
			Sort(a, n);
			printf("Sorted: ");
			for (size_t i = 0; i < n; ++i) {
				printf("%d ", a[i]);
			}
			printf("\n");
			free(a);
		}
	}
}