#include "state_machine.h"
#include <iostream>
#include <gtest/gtest.h>

enum class Events { Event0, Event1, Event2 };


class StateA { public: StateA(int v=0) : v_(v) { cout << "StateA ctor: " << v_ << endl; } ~StateA() { cout << "StateA dtor: " << v_ << endl;} int v_;};
class StateB { public: StateB(int v=0) : v_(v) { cout << "StateB ctor: " << v_ << endl; } ~StateB() { cout << "StateB dtor: " << v_ << endl;} int v_;};
class StateC { public: StateC(int v=0) : v_(v) { cout << "StateC ctor: " << v_ << endl; } ~StateC() { cout << "StateC dtor: " << v_ << endl;} int v_;};


class MyStateMachine : public StateMachine<Events> {
	template<typename FROM, typename TO>
	static StateHolderBase* customTransition(FROM* from, Events ev)
	{
		int v = from->v_+1;
		delete from;
		TO* to = new TO(v);
		return new StateHolder<TO>(to);
	}
public:
	MyStateMachine() : StateMachine(new StateA(7))
	{
		addEvent(Events::Event0, dcTransition<StateA, StateB>);
		addEvent(Events::Event1, dcTransition<StateA, StateC>);
		addEvent(Events::Event0, cdTransition<StateB, StateC>);
		addEvent(Events::Event1, dcTransition<StateB, StateA>);
		addEvent(Events::Event0, customTransition<StateC, StateA>);
		addEvent(Events::Event1, dcTransition<StateC, StateB>);
	}
};

TEST(SalutationTest, Static)
{
	MyStateMachine myStateMachine;
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event2);
	myStateMachine.inject(Events::Event1);
	myStateMachine.inject(Events::Event1);
	myStateMachine.inject(Events::Event1);
	EXPECT_EQ(8, 7);
}
