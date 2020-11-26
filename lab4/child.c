#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

/*
 * child.c
 */

int main(int argc, char** argv) {
	/* Shared file */
	int fd = shm_open(argv[1], O_RDWR, S_IRWXU);
	if (fd == -1) {
		printf("Error opening shared file in child process!\n");
		return 1;
	}
	struct stat statbuf;
	if (fstat(fd, &statbuf) == -1) {
		printf("Error getting shared file size in child!\n");
		return 1;
	}
	char* sharedFile = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (sharedFile == MAP_FAILED) {
		printf("Error mapping shared file in child process!\n");
		return 1;
	}
	/* Shared mutex */
	int fdMutex = shm_open(argv[2], O_RDWR, S_IRWXU);
	if (fdMutex == -1) {
		printf("Error opening shared mutex file in child process!\n");
		return 1;
	}
	pthread_mutex_t* mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdMutex, 0);
	if (mutex == MAP_FAILED) {
		printf("Error mapping shared mutex file in child process!\n");
		return 1;
	}
	/* Shared cond */
	int fdCond = shm_open(argv[3], O_RDWR, S_IRWXU);
	if (fdCond == -1) {
		printf("Error opening shared cond file in child process!\n");
		return 1;
	}
	pthread_cond_t* condition = mmap(NULL, sizeof(pthread_cond_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdCond, 0);
	if (condition == MAP_FAILED) {
		printf("Error mapping shared cond file in child process!\n");
		return 1;
	}

	int num = 0, sum = 0, minus = 0;
	char c;
	char* sumString = malloc(sizeof(char) * statbuf.st_size);
	if (sumString == NULL) {
		printf("Error allocating memory in child!\n");
		return 1;
	}
	while (scanf("%c", &c) > 0) {
		if (c == ' ' || c == '\t') {
			sum = minus ? sum - num : sum + num;
			num = 0;
			minus = 0;
		} else if (c == '-') {
			minus = 1;
		} else if (c == '\n') {
			sum = minus ? sum - num : sum + num;
			num = 0;
			minus = 0;
			int tmpSum = sum, i = 0;
			while (tmpSum != 0) {
				sumString[i++] = '0' + abs(tmpSum % 10);
				tmpSum = tmpSum / 10;
			}
			if (sum == 0) {
				sumString[i++] = '0';
			}
			if (pthread_mutex_lock(mutex) == -1) {
				printf("Error locking mutex in child!\n");
				return 1;
			}
			while (sharedFile[0] != 0) {
				if (pthread_cond_wait(condition, mutex)) {
					printf("Error waiting cond in child!\n");
					return 1;
				}
			}
			if (sum < 0) {
				sharedFile[0] = '-';
				for (int j = 0; j < i; ++j) {
					sharedFile[1 + j] = sumString[i - j - 1];
				}
			} else {
				for (int j = 0; j < i; ++j) {
					sharedFile[j] = sumString[i - j - 1];
				}
			}
			if (pthread_cond_signal(condition)) {
				printf("Error sending signal in child!\n");
				return 1;
			}
			if (pthread_mutex_unlock(mutex)) {
				printf("Error unlocking mutex in child!\n");
				return 1;
			}
			sum = 0;
		} else if ('0' <= c && c <= '9') {
			num = num * 10 + c - '0';
		}
	}
	if (pthread_mutex_lock(mutex) == -1) {
		printf("Error locking mutex in child!\n");
		return 1;
	}
	while (sharedFile[0] != 0) {
		if (pthread_cond_wait(condition, mutex)) {
			printf("Error waiting cond in child!\n");
			return 1;
		}
	}
	sharedFile[0] = 'a';
	if (pthread_cond_signal(condition)) {
		printf("Error sending signal in child!\n");
		return 1;
	}
	if (pthread_mutex_unlock(mutex)) {
		printf("Error unlocking mutex in child!\n");
		return 1;
	}
	free(sumString);
	if (munmap(mutex, sizeof(pthread_mutex_t)) == -1) {
		printf("Error unmapping shared mutex file in child!");
		return 1;
	}
	if (munmap(condition, sizeof(pthread_cond_t)) == -1) {
		printf("Error unmapping shared cond file in child!");
		return 1;
	}
	if (munmap(sharedFile, statbuf.st_size) == -1) {
		printf("Error unmapping shared file in child!");
		return 1;
	}
	return 0;
}