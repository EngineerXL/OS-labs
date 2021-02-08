FLAGS = -g -O2 -pedantic -pthread -std=c++17 -Wall -Werror

all: main.cpp
	g++ $(FLAGS) main.cpp -o solution -lncurses -lzmq

run: all
	./solution

clean:
	rm -rf solution
