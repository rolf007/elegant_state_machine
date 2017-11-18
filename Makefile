all: state_machine


state_machine: state_machine.cpp
	g++ -std=c++14 state_machine.cpp -o $@
