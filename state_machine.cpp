#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>

using namespace std;

enum class Events { Event0, Event1, Event2 };


class StateA { public: StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
class StateB { public: StateB(ostringstream& oss) : oss_(oss) { oss_ << "StateB ctor. "; } ~StateB() { oss_ << "StateB dtor. "; } ostringstream& oss_; };
class StateC { public: StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };


class MyStateMachine : public StateMachine<Events> {
	template<typename FROM, typename TO>
	static StateHolderBase* dcTransition(unique_ptr<FROM> from, Events ev)
	{
		ostringstream& oss = from->oss_;
		from.reset();
		return new StateHolder<TO>(new TO(oss));
	}

	template<typename FROM, typename TO>
	static StateHolderBase* cdTransition(unique_ptr<FROM> from, Events ev)
	{
		TO* to = new TO(from->oss_);
		return new StateHolder<TO>(to);
	}

public:
	MyStateMachine(ostringstream& oss) : StateMachine(new StateA(oss))
	{
		addEvent<StateA>(Events::Event0, dcTransition<StateA, StateB>);
		addEvent<StateA>(Events::Event1, dcTransition<StateA, StateC>);
		addEvent<StateB>(Events::Event0, cdTransition<StateB, StateC>);
		addEvent<StateB>(Events::Event1, dcTransition<StateB, StateA>);
		addEvent<StateC>(Events::Event0, dcTransition<StateC, StateA>);
		addEvent<StateC>(Events::Event1, dcTransition<StateC, StateB>);
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
		EXPECT_TRUE(myStateMachine.inject(Events::Event1));
		EXPECT_EQ("StateB dtor. StateA ctor. ", getLog(oss));
	}
	EXPECT_EQ("StateA dtor. ", getLog(oss));
}
