#ifndef DECLARATION_H
#define DECLARATION_H

#include <stddef.h>

inline void Swap(int* x, int* y) {
	int tmp = *x;
	*x = *y;
	*y = tmp;
}

extern int GCD(int x, int y);
extern void Sort(int* arr, const size_t size);

#endif /* DECLARATION_H */