#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

signed main() {
	char* s = NULL;
	s = malloc(sizeof(char) * 256);
	if (s == NULL) {
		printf("Error allocating memory!");
		return -1;
	}
	for (int i = 0; i < 256; i++) {
		s[i] = 0;
	}
	scanf("%s", s);
	int fd[2];
	if (pipe(fd) == -1) {
		printf("Error creating pipe!");
		return 1;
	}
	int id = fork();
	if (id == -1) {
		printf("Error creating process!");
		return 2;
	} else if (id == 0) {
		close(fd[0]);
		// printf("[%d] is a child process\n", getpid());
		FILE * input = NULL;
		input = freopen(s, "r", stdin);
		if (input == NULL) {
			printf("Error opening file!\n");
			return 3;
		}
		if (dup2(fd[1], STDOUT_FILENO) == -1) {
			printf("Error changing stdout!\n");
			return 4;
		}
		char * const * argv = NULL;
		if (execv("child.out", argv) == -1) {
			printf("Error executing child process!\n");
			return 5;
		}
	} else {
		close(fd[1]);
		// printf("[%d] is a parent process\n", getpid());
		int res;
		while (read(fd[0], &res, sizeof(int)) > 0) {
			printf("%d\n", res);
		}
		close(fd[0]);
	}
	return 0;
}