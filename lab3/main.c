#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct TItem {
	int A, B;
} Item;

typedef struct TThreadToken {
	Item* P;
	Item* Q;
	Item* Res;
	int L, R, N, M;
} ThreadToken;

void* ThreadFunc(void* token) {
	ThreadToken* tok = (ThreadToken*) token;
	int l = tok->L;
	int r = tok->R;
	if (l >= tok->N) {
		l = tok->N - 1;
	}
	if (r > tok->N) {
		r = tok->N;
	}
	for (int i = l; i < r; ++i) {
		for (int j = 0; j < tok->M; ++j) {
			tok->Res[tok->M * i + j].A = tok->P[i].A * tok->Q[j].A;
			tok->Res[tok->M * i + j].B = tok->P[i].B + tok->Q[j].B;
		}
	}
	return NULL;
}

signed main(int argc, char** argv) {
	int threadNumber = 0;
	for (int i = 0; argv[1][i] > 0; ++i) {
		if (argv[1][i] >= '0' && argv[1][i] <= '9') {
			threadNumber = threadNumber * 10 + argv[1][i] - '0';
		}
	}
	printf("Thread number: %d\n", threadNumber);
	pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * threadNumber);
	int n, m;
	scanf("%d%d", &n, &m);
	/* Input p(x) */
	Item* p = malloc(sizeof(Item) * n);
	for (int i = 0; i < n; ++i) {
		int a, b;
		scanf("%d%d", &a, &b);
		p[i].A = a;
		p[i].B = b;
	}
	/* Input q(x) */
	Item* q = malloc(sizeof(Item) * m);
	for (int i = 0; i < m; ++i) {
		int a, b;
		scanf("%d%d", &a, &b);
		q[i].A = a;
		q[i].B = b;
	}
	Item* res = malloc(sizeof(Item) * (n * m));
	ThreadToken* tokens = (ThreadToken*)malloc(sizeof(ThreadToken) * threadNumber);
	int step = (n + threadNumber - 1) / threadNumber;
	for (int i = 0; i < threadNumber; ++i) {
		tokens[i].P = p;
		tokens[i].Q = q;
		tokens[i].Res = res;
		tokens[i].L = i * step;
		tokens[i].R = (i + 1) * step + 1;
		tokens[i].N = n;
		tokens[i].M = m;
	}
	long double start, end;
	start = clock();
	for (int i = 0; i < threadNumber; ++i) {
		if (pthread_create(&threads[i], NULL, ThreadFunc, &tokens[i])) {
			printf("Error creating thread!\n");
			return -1;
		}
	}
	for (int i = 0; i < threadNumber; ++i) {
		if (pthread_join(threads[i], NULL)) {
			printf("Error executing thread!\n");
			return -1;
		}
	}
	end = clock();
	printf("Execution time %Lf ms\n", (end - start) / 1000.0);
	for (int i = 0; i < n * m; ++i) {
		printf("%d %d\n", res[i].A, res[i].B);
	}
	free(threads);
	free(p);
	free(q);
	free(res);
	free(tokens);
	return 0;
}
