#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

/*
 * main.c
 */

// Ubuntu has 255 symbol filename limit
const unsigned long long FILENAME_LIMIT = 255;
const unsigned long long SHARED_MEMORY_SIZE = 16;
const char* CHILD_EXECUTABLE_NAME = "child.out";
const char* SHARED_FILE_NAME = "shared_file";
const char* SHARED_MUTEX_NAME = "shared_mutex";
const char* SHARED_COND_NAME = "shared_cond";

int main() {
	char* s = malloc(sizeof(char) * (FILENAME_LIMIT + 1));
	if (s == NULL) {
		printf("Error allocating memory!\n");
		return 1;
	}
	for (int i = 0; i < FILENAME_LIMIT + 1; i++) {
		s[i] = 0;
	}
	if (!(scanf("%s", s) > 0)) {
		printf("Error reading file name!\n");
		return 1;
	}
	FILE* input = fopen(s, "r");
	if (input == NULL) {
		printf("Error opening input file!\n");
		return 1;
	}
	/* Shared file */
	int fd = shm_open(SHARED_FILE_NAME, O_RDWR | O_CREAT, S_IRWXU);
	if (fd == -1) {
		printf("Error creating shared file!\n");
		return 1;
	}
	if (ftruncate(fd, SHARED_MEMORY_SIZE) == -1) {
		printf("Error truncating shared file!\n");
		return 1;
	}
	/* Shared mutex */
	int fdMutex = shm_open(SHARED_MUTEX_NAME, O_RDWR | O_CREAT, S_IRWXU);
	if (ftruncate(fdMutex, sizeof(pthread_mutex_t))) {
		printf("Error creating shared mutex file!\n");
		return 1;
	}
	pthread_mutexattr_t mattr;
	if (pthread_mutexattr_init(&mattr)) {
		printf("Error initializing mutex attribute!\n");
		return 3;
	}
    if (pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED)) {
    	printf("Error sharing mutex attribute!\n");
		return 1;
    }
	pthread_mutex_t* mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdMutex, 0);
	if (mutex == MAP_FAILED) {
		printf("Error mapping shared mutex!\n");
		return 1;
	}
	if (pthread_mutex_init(mutex, &mattr)) {
		printf("Error initializing mutex!\n");
		return 1;
	}
	if (pthread_mutexattr_destroy(&mattr)) {
		printf("Error destoying mutex attribute!\n");
		return 1;
	}
	/* Shared cond */
	int fdCond = shm_open(SHARED_COND_NAME, O_RDWR | O_CREAT, S_IRWXU);
	if (ftruncate(fdCond, sizeof(pthread_cond_t))) {
		printf("Error creating shared cond file!\n");
		return 1;
	}
	pthread_condattr_t cattr;
	if (pthread_condattr_init(&cattr)) {
		printf("Error initializing cond attribute!\n");
		return 1;
	}
    if(pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED)) {
    	printf("Error sharing cond attribute!\n");
		return 1;
    }
	pthread_cond_t* condition = (pthread_cond_t*)mmap(NULL, sizeof(pthread_cond_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdCond, 0);
	if (mutex == MAP_FAILED) {
		printf("Error mapping shared cond!\n");
		return 1;
	}
	if (pthread_cond_init(condition, &cattr)) {
		printf("Error initializing cond!\n");
		return 1;	
	}
	if (pthread_condattr_destroy(&cattr)) {
		printf("Error destoying cond attribute!\n");
		return 1;
	}

	int id = fork();
	if (id == -1) {
		printf("Error creating process!");
		return 1;
	} else if (id == 0) {
		if (dup2(fileno(input), fileno(stdin)) == -1) {
			printf("Error changing stdin in child process!");
			return 1;
		}
		char** argv = malloc(sizeof(char*) * 5);
		if (argv == NULL) {
			printf("Erorr allocating memory!");
			return 1;
		}
		argv[0] = malloc(sizeof(char) * 10);
		memcpy(argv[0], CHILD_EXECUTABLE_NAME, 10);
		argv[1] = malloc(sizeof(char) * 12);
		memcpy(argv[1], SHARED_FILE_NAME, 12);
		argv[2] = malloc(sizeof(char) * 13);
		memcpy(argv[2], SHARED_MUTEX_NAME, 13);
		argv[3] = malloc(sizeof(char) * 12);
		memcpy(argv[3], SHARED_COND_NAME, 12);
		argv[4] = NULL;
		if (execv(CHILD_EXECUTABLE_NAME, argv) == -1) {
			printf("Error executing child process!\n");
			return 1;
		}
	} else {
		char* sharedFile = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		if (sharedFile == MAP_FAILED) {
			printf("Error creating shared file!");
			return 1;
		}
		while (1) {
			if (pthread_mutex_lock(mutex)) {
				printf("Error locking mutex in parent!\n");
				return 1;
			}
			while (sharedFile[0] == 0) {
				if (pthread_cond_wait(condition, mutex)) {
					printf("Error waiting cond in parent!\n");
					return 1;
				}
			}
			if (sharedFile[0] == 'a') {
				if (pthread_mutex_unlock(mutex)) {
					printf("Error unlocking mutex in parent!\n");
					return 1;
				}
				break;
			}
			printf("%s\n", sharedFile);
			for (int j = 0; j < SHARED_MEMORY_SIZE; ++j) {
				sharedFile[j] = 0;
			}
			if (pthread_cond_signal(condition)) {
				printf("Error sending signal in parent!\n");
				return 1;
			}
			if (pthread_mutex_unlock(mutex)) {
				printf("Error unlocking mutex in parent!\n");
				return 1;
			}
		}
		if (munmap(sharedFile, SHARED_MEMORY_SIZE) == -1) {
			printf("Error unmapping fd1!");
			return 1;
		}
	}
	if (pthread_mutex_destroy(mutex)) {
		printf("Error destroying mutex!\n");
		return 1;
	}
	if (munmap(mutex, sizeof(pthread_mutex_t))) {
		printf("Error unmapping mutex!\n");
		return 1;
	}
	if (pthread_cond_destroy(condition)) {
		printf("Error destroying cond!\n");
		return 1;
	}
	if (munmap(condition, sizeof(pthread_cond_t))) {
		printf("Error unmapping cond!\n");
		return 1;
	}
	if (shm_unlink(SHARED_FILE_NAME)) {
		printf("Error unlinking shared file!\n");
		return 1;
	}
	if (shm_unlink(SHARED_MUTEX_NAME)) {
		printf("Error unlinking shared mutex file!\n");
		return 1;
	}
	if (shm_unlink(SHARED_COND_NAME)) {
		printf("Error unlinking shared cond file!\n");
		return 1;
	}
	if (fclose(input)) {
		printf("Error closing input file!\n");
		return 1;
	}
	free(s);
	return 0;
}