#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define check_ok(VALUE, OKVAL, MSG) if (VALUE != OKVAL) { printf("%s", MSG); return 1; }
#define check_wrong(VALUE, WRONGVAL, MSG) if (VALUE == WRONGVAL) { printf("%s", MSG); return 1; }

int main(int argc, char** argv) {
	/* Shared file */
	int fd = shm_open(argv[1], O_RDWR, S_IRWXU);
	check_wrong(fd, -1, "Error opening shared file in child process!\n");
	struct stat statbuf;
	check_wrong(fstat(fd, &statbuf), -1, "Error getting shared file size in child!\n");
	char* sharedFile = mmap(NULL, statbuf.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	check_wrong(sharedFile, MAP_FAILED, "Error mapping shared file in child process!\n");
	/* Shared mutex */
	int fdMutex = shm_open(argv[2], O_RDWR, S_IRWXU);
	check_wrong(fdMutex, -1, "Error opening shared mutex file in child process!\n");
	pthread_mutex_t* mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdMutex, 0);
	check_wrong(mutex, MAP_FAILED, "Error mapping shared mutex file in child process!\n");
	/* Shared cond */
	int fdCond = shm_open(argv[3], O_RDWR, S_IRWXU);
	check_wrong(fdCond, -1, "Error opening shared cond file in child process!\n");
	pthread_cond_t* condition = mmap(NULL, sizeof(pthread_cond_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdCond, 0);
	check_wrong(condition, MAP_FAILED, "Error mapping shared cond file in child process!\n");

	int num = 0, sum = 0, minus = 0;
	char c;
	char* sumString = malloc(sizeof(char) * statbuf.st_size);
	check_wrong(sumString, NULL, "Error allocating memory in child!\n");
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
			check_ok(pthread_mutex_lock(mutex), 0, "Error locking mutex in child!\n");
			while (sharedFile[0] != 0) {
				check_ok(pthread_cond_wait(condition, mutex), 0, "Error waiting cond in child!\n");
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
			check_ok(pthread_cond_signal(condition), 0, "Error sending signal in child!\n");
			check_ok(pthread_mutex_unlock(mutex), 0, "Error unlocking mutex in child!\n");
			sum = 0;
		} else if ('0' <= c && c <= '9') {
			num = num * 10 + c - '0';
		}
	}
	check_ok(pthread_mutex_lock(mutex), 0, "Error locking mutex in child!\n");
	while (sharedFile[0] != 0) {
		check_ok(pthread_cond_wait(condition, mutex), 0, "Error waiting cond in child!\n");
	}
	sharedFile[0] = 'a';
	check_ok(pthread_cond_signal(condition), 0, "Error sending signal in child!\n");
	check_ok(pthread_mutex_unlock(mutex), 0, "Error unlocking mutex in child!\n");

	check_wrong(munmap(mutex, sizeof(pthread_mutex_t)), -1, "Error unmapping shared mutex file in child!");
	check_wrong(munmap(condition, sizeof(pthread_cond_t)), -1, "Error unmapping shared cond file in child!");
	check_wrong(munmap(sharedFile, statbuf.st_size), -1, "Error unmapping shared file in child!");
	free(sumString);
	return 0;
}