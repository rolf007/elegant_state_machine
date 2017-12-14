#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>

using namespace std;

enum class Events { Event0, Event1, Event2, Event3 };

class StateA { public: StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
class StateB { public: StateB(ostringstream& oss) : oss_(oss) { oss_ << "StateB ctor. "; } ~StateB() { oss_ << "StateB dtor. "; } ostringstream& oss_; };
class StateC { public: StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };

class MyStateMachine : public StateMachine<Events> {
	template<typename FROM, typename TO>
	static StateBase* dcTransition(unique_ptr<State<FROM>> from, Events ev)
	{
		// dc: first destroy old state, then construct new state
		ostringstream& oss = from->oss_;
		from.reset();
		return new State<TO>(oss);
	}
	template<typename FROM, typename TO>
	static StateBase* cdTransition(unique_ptr<State<FROM>> from, Events ev)
	{
		// cd: first construct new state, then destroy old state
		return new State<TO>(from->oss_);
	}
	template<typename T>
	static StateBase* nopTransition(unique_ptr<State<T>> from, Events ev)
	{
		// nop: stay in same state
		return from.release();
	}
	template<typename T>
	static StateBase* failTransition(unique_ptr<State<T>> from, Events ev)
	{
		// fail: stay in same state, and don't accept event
		from.release();
		return nullptr;
	}
public:
	MyStateMachine(ostringstream& oss) : StateMachine(new State<StateA>(oss))
	{
		addEvent<StateA>(Events::Event0, dcTransition<StateA, StateB>);
		addEvent<StateA>(Events::Event1, dcTransition<StateA, StateC>);
		addEvent<StateB>(Events::Event0, cdTransition<StateB, StateC>);
		addEvent<StateB>(Events::Event1, dcTransition<StateB, StateA>);
		addEvent<StateC>(Events::Event0, dcTransition<StateC, StateA>);
		addEvent<StateC>(Events::Event1, dcTransition<StateC, StateB>);
		addEvent<StateB>(Events::Event2, nopTransition<StateB>);
		addEvent<StateB>(Events::Event3, failTransition<StateB>);
	}
};

string getLog(ostringstream& oss)
{
	string ret = oss.str();
	oss.str("");
	oss.clear();
	return ret;
}

TEST(SimpleExample, basic)
{
	ostringstream oss;
	{
		MyStateMachine myStateMachine(oss);
		EXPECT_EQ("StateA ctor. ", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event0));
		EXPECT_EQ("StateA dtor. StateB ctor. ", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event0));
		EXPECT_EQ("StateC ctor. StateB dtor. ", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event0));
		EXPECT_EQ("StateC dtor. StateA ctor. ", getLog(oss));
		EXPECT_FALSE(myStateMachine.inject(Events::Event2));
		EXPECT_EQ("", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event1));
		EXPECT_EQ("StateA dtor. StateC ctor. ", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event1));
		EXPECT_EQ("StateC dtor. StateB ctor. ", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event2));
		EXPECT_EQ("", getLog(oss));
		EXPECT_FALSE(myStateMachine.inject(Events::Event3));
		EXPECT_EQ("", getLog(oss));
		EXPECT_TRUE(myStateMachine.inject(Events::Event1));
		EXPECT_EQ("StateB dtor. StateA ctor. ", getLog(oss));
	}
	EXPECT_EQ("StateA dtor. ", getLog(oss));
}
