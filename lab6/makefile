FLAGS = -g -O2 -pedantic -pthread -std=c++17 -Wall -Werror -Wextra

all: control calculation

control: control.cpp topology.hpp zmq_std.hpp
	g++ $(FLAGS) control.cpp -lzmq -o control

calculation: calculation_node.o search.o
	g++ $(FLAGS) calculation_node.o search.o -lzmq -o calculation

calculation_node.o: calculation_node.cpp zmq_std.hpp
	g++ $(FLAGS) -c calculation_node.cpp -lzmq

search.o: search.cpp search.hpp
	g++ $(FLAGS) -c search.cpp

clean:
	rm -rf *.o calculation control
