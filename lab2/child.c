#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

signed main() {
	int x, y, z;
	while (scanf("%d%d%d", &x, &y, &z) > 0) {
		int res = x + y + z;
		write(fileno(stdout), &res, sizeof(int));
	}
}