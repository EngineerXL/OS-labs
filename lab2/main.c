#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

signed main() {
	char* s = malloc(sizeof(char) * 64);
	for (int i = 0; i < 64; i++) {
		s[i] = 0;
	}
	scanf("%s", s);
	int fd[2];
	pipe(fd);
	int id = fork();
	if (id == -1) {
		return -1;
	} else if (id == 0) {
		close(fd[0]);
		// printf("[%d] is a child process\n", getpid());
		freopen(s, "r", stdin);
		dup2(fd[1], fileno(stdout));
		execv("child.out", NULL);
	} else {
		close(fd[1]);
		// printf("[%d] is a parent process\n", getpid());
		// freopen("output.txt", "w", stdout);
		int res;
		while (read(fd[0], &res, sizeof(int)) > 0) {
			printf("%d\n", res);
		}
		close(fd[0]);
	}
	return 0;
}