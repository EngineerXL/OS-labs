FLAGS = -g -O2 -pedantic -std=c11 -Wall -Werror

static1: implementation1 static_main
	gcc $(FLAGS) implementation1.o static_main.o -o solution

implementation1: declaration.h implementation1.c
	gcc $(FLAGS) -c implementation1.c

static2: implementation2 static_main
	gcc $(FLAGS) implementation2.o static_main.o -o solution

implementation2: declaration.h implementation2.c
	gcc $(FLAGS) -c implementation2.c

static_main: static_main.c
	gcc $(FLAGS) -c static_main.c

dynamic: dynamic_main dynamic1 dynamic2
	gcc $(FLAGS) dynamic_main.o -o solution -ldl

dynamic_main: dynamic_main.c
	gcc $(FLAGS) -c dynamic_main.c -ldl

dynamic1: declaration.h implementation1.c
	gcc $(FLAGS) -shared -o implementation1.so -fPIC implementation1.c

dynamic2: declaration.h implementation2.c
	gcc $(FLAGS) -shared -o implementation2.so -fPIC implementation2.c

clean:
	rm -rf *.o *.so solution