FLAGS = -g -O2 -pedantic -std=c11 -Wall -Werror

static1: dynamic1 static_main
	gcc $(FLAGS) static_main.o -L. -limpl1 -o solution

static2: dynamic2 static_main
	gcc $(FLAGS) static_main.o -L. -limpl2 -o solution

static_main: static_main.c
	gcc $(FLAGS) -c static_main.c

dynamic: dynamic_main dynamic1 dynamic2
	gcc $(FLAGS) dynamic_main.o -o solution -ldl

dynamic_main: dynamic_main.c
	gcc $(FLAGS) -c dynamic_main.c -ldl

dynamic1: declaration.h implementation1.c
	gcc $(FLAGS) -shared -o libimpl1.so -fPIC implementation1.c

dynamic2: declaration.h implementation2.c
	gcc $(FLAGS) -shared -o libimpl2.so -fPIC implementation2.c

clean:
	rm -rf *.o *.so solution
