FLAGS = -std=c11 -pedantic -Wall -Werror

all: a.out child.out

a.out: main.c
	gcc $(FLAGS) main.c -o a.out

child.out: child.c
	gcc $(FLAGS) child.c -o child.out

clean:
	rm *.out