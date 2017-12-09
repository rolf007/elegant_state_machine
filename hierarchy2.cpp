#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>
#include <set>
#include <string>

using namespace std;

namespace {

template<typename E, typename B = void>
bool breadthFirstPolicy(Hierarchical<StateMachine<E, B>>* self, E ev)
{
	if (self->injectMachine(ev))
		return true;
	return self->getState()->inject(atoi(ev.c_str()));
}

template<typename E, typename B = void>
bool depthFirstPolicy(Hierarchical<StateMachine<E, B>>* self, E ev)
{
	if (self->getState()->inject(atoi(ev.c_str())))
		return true;
	return self->injectMachine(ev);
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

struct BaseStateABC { virtual string getStr() const = 0; virtual bool inject(int) { return false; } };
struct BaseStateXYZ { virtual string getStr() const = 0; };
struct StB : BaseStateABC { string getStr() const { return "B";} };
struct StX : BaseStateXYZ { string getStr() const { return "X";} };
struct StY : BaseStateXYZ { string getStr() const { return "Y";} };

class StA : public StateMachine<int, BaseStateXYZ>, public BaseStateABC {
public:
	StA() : StateMachine(new StX)
	{
		addEvent<StX>(1, [this](unique_ptr<StX> from, int) { return new StateHolder<StY>(new StY); });
	}
	string getStr() const { return getState()->getStr();}
	virtual bool inject(int ev) override { return StateMachine::inject(ev); }
};


class MyHierarchical2 : public Hierarchical<StateMachine<string, BaseStateABC>> {
public:
	MyHierarchical2(function<bool(Hierarchical*, string)>policy) : Hierarchical(new StA, policy)
	{
		addEvent<StA>("1", [this](unique_ptr<StA> from, string) { return new StateHolder<StB>(new StB); });
	}
	string getStr() const { return getState()->getStr();}
};

TEST(MyHierarchical, different_event_types_depthFirst)
{
	MyHierarchical2 hierarchical(depthFirstPolicy<string, BaseStateABC>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("Y", hierarchical.getStr());
}

TEST(MyHierarchical, different_event_types_breadthFirst)
{
	MyHierarchical2 hierarchical(breadthFirstPolicy<string, BaseStateABC>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("B", hierarchical.getStr());
}

}
