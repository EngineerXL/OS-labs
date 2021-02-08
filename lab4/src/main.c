#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define check_ok(VALUE, OKVAL, MSG) if (VALUE != OKVAL) { printf("%s", MSG); return 1; }
#define check_wrong(VALUE, WRONGVAL, MSG) if (VALUE == WRONGVAL) { printf("%s", MSG); return 1; }

/* Ubuntu has 255 symbol filename limit */
const unsigned long long FILENAME_LIMIT = 255;
const unsigned long long SHARED_MEMORY_SIZE = 16;
const char* CHILD_EXECUTABLE_NAME = "child.out";
const char* SHARED_FILE_NAME = "shared_file";
const char* SHARED_MUTEX_NAME = "shared_mutex";
const char* SHARED_COND_NAME = "shared_cond";

int main() {
	char* s = malloc(sizeof(char) * (FILENAME_LIMIT + 1));
	check_wrong(s, NULL, "Error allocating memory!\n");
	for (int i = 0; i < FILENAME_LIMIT + 1; i++) {
		s[i] = 0;
	}
	if (!(scanf("%s", s) > 0)) {
		printf("Error reading file name!\n");
		return 1;
	}
	FILE* input = fopen(s, "r");
	check_wrong(input, NULL, "Error opening input file!\n");
	/* Shared file */
	int fd = shm_open(SHARED_FILE_NAME, O_RDWR | O_CREAT, S_IRWXU);
	check_wrong(fd, -1, "Error creating shared file!\n");
	check_ok(ftruncate(fd, SHARED_MEMORY_SIZE), 0, "Error truncating shared file!\n");
	/* Shared mutex */
	int fdMutex = shm_open(SHARED_MUTEX_NAME, O_RDWR | O_CREAT, S_IRWXU);
	check_ok(ftruncate(fdMutex, sizeof(pthread_mutex_t)), 0, "Error creating shared mutex file!\n");

	pthread_mutexattr_t mutex_attribute;
	check_ok(pthread_mutexattr_init(&mutex_attribute), 0, "Error initializing mutex attribute!\n");
	check_ok(pthread_mutexattr_setpshared(&mutex_attribute, PTHREAD_PROCESS_SHARED), 0, "Error sharing mutex attribute!\n");

	pthread_mutex_t* mutex = mmap(NULL, sizeof(pthread_mutex_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdMutex, 0);
	check_wrong(mutex, MAP_FAILED, "Error mapping shared mutex!\n");
	check_ok(pthread_mutex_init(mutex, &mutex_attribute), 0, "Error initializing mutex!\n");
	check_ok(pthread_mutexattr_destroy(&mutex_attribute), 0, "Error destoying mutex attribute!\n");
	/* Shared cond */
	int fdCond = shm_open(SHARED_COND_NAME, O_RDWR | O_CREAT, S_IRWXU);
	check_ok(ftruncate(fdCond, sizeof(pthread_cond_t)), 0, "Error creating shared cond file!\n");

	pthread_condattr_t condition_attribute;
	check_ok(pthread_condattr_init(&condition_attribute), 0, "Error initializing cond attribute!\n");
	check_ok(pthread_condattr_setpshared(&condition_attribute, PTHREAD_PROCESS_SHARED), 0, "Error sharing cond attribute!\n");

	pthread_cond_t* condition = (pthread_cond_t*)mmap(NULL, sizeof(pthread_cond_t), PROT_READ | PROT_WRITE, MAP_SHARED, fdCond, 0);
	check_wrong(mutex, MAP_FAILED, "Error mapping shared cond!\n");
	check_ok(pthread_cond_init(condition, &condition_attribute), 0, "Error initializing cond!\n");
	check_ok(pthread_condattr_destroy(&condition_attribute), 0, "Error destoying cond attribute!\n");
	/* Creating child process */
	int id = fork();
	check_wrong(id, -1, "Error creating process!\n");
	if (id == 0) {
		check_wrong(dup2(fileno(input), fileno(stdin)), -1, "Error changing stdin in child process!");
		char** argv = malloc(sizeof(char*) * 5);
		check_wrong(argv, NULL, "Erorr allocating memory!");
		argv[0] = malloc(sizeof(char) * 10);
		memcpy(argv[0], CHILD_EXECUTABLE_NAME, 10);
		argv[1] = malloc(sizeof(char) * 12);
		memcpy(argv[1], SHARED_FILE_NAME, 12);
		argv[2] = malloc(sizeof(char) * 13);
		memcpy(argv[2], SHARED_MUTEX_NAME, 13);
		argv[3] = malloc(sizeof(char) * 12);
		memcpy(argv[3], SHARED_COND_NAME, 12);
		argv[4] = NULL;
		check_wrong(execv(CHILD_EXECUTABLE_NAME, argv), -1, "Error executing child process!\n");
	} else {
		char* sharedFile = mmap(NULL, SHARED_MEMORY_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
		check_wrong(sharedFile, MAP_FAILED, "Error creating shared file!");
		while (1) {
			check_ok(pthread_mutex_lock(mutex), 0, "Error locking mutex in parent!\n");
			while (sharedFile[0] == 0) {
				check_ok(pthread_cond_wait(condition, mutex), 0, "Error waiting cond in parent!\n");
			}
			if (sharedFile[0] == 'a') {
				check_ok(pthread_mutex_unlock(mutex), 0, "Error unlocking mutex in parent!\n");
				break;
			}
			printf("%s\n", sharedFile);
			for (int j = 0; j < SHARED_MEMORY_SIZE; ++j) {
				sharedFile[j] = 0;
			}
			check_ok(pthread_cond_signal(condition), 0, "Error sending signal in parent!\n");
			check_ok(pthread_mutex_unlock(mutex), 0, "Error unlocking mutex in parent!\n");
		}
		check_wrong(munmap(sharedFile, SHARED_MEMORY_SIZE), -1, "Error unmapping fd1!");
	}
	check_ok(pthread_mutex_destroy(mutex), 0, "Error destroying mutex!\n");
	check_ok(munmap(mutex, sizeof(pthread_mutex_t)), 0, "Error unmapping mutex!\n");
	check_ok(pthread_cond_destroy(condition), 0, "Error destroying cond!\n");
	check_ok(munmap(condition, sizeof(pthread_cond_t)), 0, "Error unmapping cond!\n");

	check_wrong(shm_unlink(SHARED_FILE_NAME), -1, "Error unlinking shared file!\n");
	check_wrong(shm_unlink(SHARED_MUTEX_NAME), -1, "Error unlinking shared mutex file!\n");
	check_wrong(shm_unlink(SHARED_COND_NAME), -1, "Error unlinking shared cond file!\n");
	
	check_ok(fclose(input), 0, "Error closing input file!\n");
	free(s);
	return 0;
}
