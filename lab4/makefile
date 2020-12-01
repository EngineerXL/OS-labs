FLAGS = -O2 -pedantic -pthread -std=c11 -Wall -Werror

all: main.out child.out

main.out: main.c
	gcc $(FLAGS) main.c -o main.out -lrt

child.out: child.c
	gcc $(FLAGS) child.c -o child.out -lrt

clean:
	rm -rf *.out