all: state_machine


state_machine: state_machine.cpp
	g++ -std=c++14 -lgtest -lgtest_main state_machine.cpp -o $@
