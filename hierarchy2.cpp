#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>
#include <set>
#include <string>

using namespace std;

namespace {

template<typename S, typename M, typename E>
bool breadthFirstPolicy(S* state, M* machine, E ev)
{
	if (machine->inject(ev))
		return true;
	return state->inject(atoi(ev.c_str()));
}

template<typename S, typename M, typename E>
bool depthFirstPolicy(S* state, M* machine, E ev)
{
	if (state->inject(atoi(ev.c_str())))
		return true;
	return machine->inject(ev);
}

/*
/----------StateMachine--------------------\
|                                          |
|  /-----StateMachine------\      /----\   |
|  |A                      |      |B   |   |
|  |   /----\      /----\  +----->+    |   |
|  |   |X   |      |Y   |  |      |    |   |
|  |   |    +----->+    |  |      |    |   |
|  |   |    |      |    |  |      |    |   |
|  |   \----/      \----/  |      |    |   |
|  |                       |      |    |   |
|  \-----------------------/      \----/   |
|                                          |
\------------------------------------------/
*/

struct BaseState { virtual string getStr() const = 0; };
struct StB : BaseState { string getStr() const { return "B";} };
struct StX : BaseState { string getStr() const { return "X";} };
struct StY : BaseState { string getStr() const { return "Y";} };

class StA : public StateMachine<int, BaseState>, public BaseState{
public:
	StA() : StateMachine(new StX)
	{
		addEvent<StX>(1, [this](unique_ptr<StX> from, int) { return new StateHolder<StY>(new StY); });
	}
	string getStr() const { return getState()->getStr();}
};

class MyHierarchical2 : public Hierarchical<StateMachine<string, BaseState>> {
public:
	MyHierarchical2(bool depthFirst) : Hierarchical(new StA)
	{
		addEvent<StA>("1", [](unique_ptr<StA> from, string) { return new StateHolder<StB>(new StB); });
		if (depthFirst)
			addPolicy<StA>(depthFirstPolicy<StA, StateMachine<string, BaseState>, string>);
		else
			addPolicy<StA>(breadthFirstPolicy<StA, StateMachine<string, BaseState>, string>);
	}
	string getStr() const { return getState()->getStr();}
};

TEST(MyHierarchical, different_event_types_depthFirst)
{
	MyHierarchical2 hierarchical(true);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("Y", hierarchical.getStr());
}

TEST(MyHierarchical, different_event_types_breadthFirst)
{
	MyHierarchical2 hierarchical(false);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("B", hierarchical.getStr());
}

}
