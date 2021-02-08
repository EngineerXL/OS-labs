#include "declaration.h"

int GCD(int x, int y) {
	if (x > y) {
		Swap(&x, &y);
	}
	for (int i = x; i > 1; --i) {
		if (x % i == 0 && y % i == 0) {
			return i;
		}
	}
	return 1;
}

void QuickSort(int* arr, const size_t l, const size_t r) {
	if (l + 1 >= r) {
		return;
	}
	size_t i = l, j = r - 2;
	while (i < j) {
		if (arr[i] < arr[r - 1]) {
			++i;
		} else if (arr[j] > arr[r - 1]) {
			--j;
		} else {
			Swap(&arr[i], &arr[j]);
			++i;
			--j;
		}
	}
	if (arr[i] < arr[r - 1]) {
		++i;
	}
	Swap(&arr[i], &arr[r - 1]);
	QuickSort(arr, l, i);
	QuickSort(arr, i + 1, r);
}

void Sort(int* arr, const size_t n) {
	QuickSort(arr, 0, n);
}