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

class StA : public StateMachine<int>, public BaseState{
public:
	StA() : StateMachine(new StateHolder<StX>)
	{
		addEvent<StX>(1, [this](unique_ptr<StateHolder<StX>> from, int) { return new StateHolder<StY>(); });
	}
	string getStr() const { return getState<BaseState>()->getStr();}
};

class MyHierarchical2 : public Hierarchical<StateMachine<string>> {
public:

	MyHierarchical2(function<bool(StA*, StateMachine<string>*, string)> policy) : Hierarchical(new StateHolder<StA>)
	{
		addEvent<StA>("1", [](unique_ptr<StateHolder<StA>> from, string) { return new StateHolder<StB>(); });
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
}

TEST(MyHierarchical, different_event_types_breadthFirst)
{
	MyHierarchical2 hierarchical(breadthFirstPolicy<StA, StateMachine<string>>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject("1"));
	EXPECT_EQ("B", hierarchical.getStr());
}


class HolderBase
{
public:
	virtual ~HolderBase() = default;    
	template <class X> X* get() { return dynamic_cast<X*>(this); }
};

template <class T>
class Holder : public HolderBase, public T
{
public:
	using T::T;
};

struct A {
    virtual ~A() = default;
    A(int a) : a(a) {};
    int a;
};

struct B : A {
    B(int a, int b) : A(a), b(b) {};
    int b;
};

struct C : A {
    C(int a, int c) : A(a), c(c) {};
    int c;
};

TEST(h2, h2)
{
	std::vector<std::unique_ptr<HolderBase>> v;
	v.emplace_back(std::make_unique<Holder<B>>(7,40));
	v.emplace_back(std::make_unique<Holder<C>>(0,42));

	A* a = v[0]->template get<A>();
	B* b = v[0]->template get<B>();
	C* c = v[0]->template get<C>();

	std::cout << a << " " << b << " " << c << "\n";

	a = v[1]->template get<A>();
	b = v[1]->template get<B>();
	c = v[1]->template get<C>();

	std::cout << a << " " << b << " " << c << "\n";
}

}
