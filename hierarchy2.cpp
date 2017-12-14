#include "state_machine.h"
#include <gtest/gtest.h>
#include <iostream>
#include <string>

using namespace std;

namespace {

template<typename S, typename M>
bool breadthFirstPolicy(S* state, M* machine, typename M::EventType ev)
{
	if (machine->inject(ev))
		return true;
	return state->inject(atoi(ev.c_str()));
}

template<typename S, typename M>
bool depthFirstPolicy(S* state, M* machine, typename M::EventType ev)
{
	if (state->inject(atoi(ev.c_str())))
		return true;
	return machine->inject(ev);
}

/*
/----------StateMachine-------------------------\
|                                               |
|  /-----StateMachine------------\     /----\   |
|  |A                            |     |B   |   |
|  |   /----\   /----\   /----\  +---->+    |   |
|  |   |X   |   |Y   |   |Z   |  |     |    |   |
|  |   |    +-->+    +-->+    |  |     |    |   |
|  |   |    |   |    |   |    |  |     |    |   |
|  |   \----/   \----/   \----/  |     |    |   |
|  |                             |     |    |   |
|  \-----------------------------/     \----/   |
|                                               |
\-----------------------------------------------/
*/

struct BaseState { virtual string getStr() const = 0; };
struct StB : BaseState { string getStr() const { return "B";} };
struct StX : BaseState { string getStr() const { return "X";} };
struct StY : BaseState { string getStr() const { return "Y";} };
struct StZ { };

class StA : public StateMachine<int>, public BaseState{
public:
	StA() : StateMachine(new State<StX>)
	{
		addEvent<StX>(1, [this](unique_ptr<State<StX>> from, int) { return new State<StY>(); });
		addEvent<StY>(2, [this](unique_ptr<State<StY>> from, int) { return new State<StZ>(); });
	}
	string getStr() const { if (BaseState* base = getState<BaseState>()) return base->getStr(); return "<none>";}
};

class MyHierarchical2 : public Hierarchical<StateMachine<string>> {
public:

	MyHierarchical2(function<bool(StA*, StateMachine<string>*, string)> policy) : Hierarchical(new State<StA>)
	{
		addEvent<StA>("1", [](unique_ptr<State<StA>> from, string) { return new State<StB>(); });
		addPolicy<StA>(policy);
	}
	string getStr() const { return getState<BaseState>()->getStr();}
};

TEST(MyHierarchical, different_event_types_depthFirst)
{
	MyHierarchical2 hierarchical(depthFirstPolicy<StA, StateMachine<string>>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("Y", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("2"));
	EXPECT_EQ("<none>", hierarchical.getStr());
}

TEST(MyHierarchical, different_event_types_breadthFirst)
{
	MyHierarchical2 hierarchical(breadthFirstPolicy<StA, StateMachine<string>>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("B", hierarchical.getStr());
}

}
