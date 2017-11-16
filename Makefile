all: state_machine


state_machine: state_machine.cpp
	g++ -std=c++11 state_machine.cpp -o $@
