#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>

#define check(VALUE, OKVAL, MSG) if (VALUE != OKVAL) { printf("%s", MSG); return 1; }

const char* DYN_LIB_1 = "./implementation1.so";
const char* DYN_LIB_2 = "./implementation2.so";
const char* GCD_FUNC_NAME = "GCD";
const char* SORT_FUNC_NAME = "Sort";

int main() {
	int dynLibNum = 1;
	void* handle = dlopen(DYN_LIB_1, RTLD_LAZY);
	if (handle == NULL) {
		printf("Error opening dynamic library!\n");
		return 1;
	}
	int (*GCD)(int, int);
	void (*Sort)(int*, const size_t);
	*(void**) (&GCD) = dlsym(handle, GCD_FUNC_NAME);
	*(void**) (&Sort) = dlsym(handle, SORT_FUNC_NAME);
	char* error = dlerror();
	check(error, NULL, error);
	int q;
	while (scanf("%d", &q) > 0) {
		if (q == 0) {
			check(dlclose(handle), 0, "Error closing dynamic library!\n");
			if (dynLibNum) {
				handle = dlopen(DYN_LIB_2, RTLD_LAZY);
			} else {
				handle = dlopen(DYN_LIB_1, RTLD_LAZY);
			}
			if (handle == NULL) {
				printf("Error opening dynamic library!\n");
				return 1;
			}
			*(void**) (&GCD) = dlsym(handle, GCD_FUNC_NAME);
			*(void**) (&Sort) = dlsym(handle, SORT_FUNC_NAME);
			error = dlerror();
			check(error, NULL, error);
			dynLibNum = dynLibNum ^ 1;
		} else if (q == 1) {
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
	check(dlclose(handle), 0, "Error closing dynamic library!\n");
}