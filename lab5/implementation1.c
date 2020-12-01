#include "declaration.h"

int GCD(int x, int y) {
	while (y > 0) {
		if (x >= y) {
			x = x % y;
		}
		Swap(&x, &y);
	}
	return x;
}

void Sort(int* arr, const size_t size) {
	for (size_t i = 0; i < size; ++i) {
		for (size_t j = 1; j < size; ++j) {
			if (arr[j - 1] > arr[j]) {
				Swap(&arr[j - 1], &arr[j]);
			}
		}
	}
}