all: state_machine


state_machine: state_machine.cpp state_machine.h hierarchy.cpp hierarchy2.cpp
	g++ -std=c++14 -lgtest -lgtest_main state_machine.cpp hierarchy.cpp hierarchy2.cpp -o $@
