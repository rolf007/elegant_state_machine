#include "state_machine.h"
#include <gtest/gtest.h>
#include <iostream>
#include <ostream>

using namespace std;

/*
/-----------------------StateMachine--------------------------------\
|                                                                   |
|  /----\       /-----------StateMachine------------\      /----\   |
|  |A   |       |B                                  |      |C   |   |
|  |    +------>+   /----\      /----\      /----\  +----->+    |   |
|  |    |       |   |X   |      |Y   |      |Z   |  |      |    |   |
|  |    |       |   |    +----->+    +----->+    |  |      |    |   |
|  |    |       |   |    |      |    |      |    |  |      |    |   |
|  |    |       |   \----/      \----/      \----/  |      |    |   |
|  |    |       |                                   |      |    |   |
|  \----/       \-----------------------------------/      \----/   |
|                                                                   |
\-------------------------------------------------------------------/
*/

namespace {

template<typename S, typename M>
bool breadthFirstPolicy(S* state, M* machine, typename M::EventType ev)
{
	if (machine->inject(ev))
		return true;
	return state->inject(ev) ;
}

template<typename S, typename M>
bool depthFirstPolicy(S* state, M* machine, typename M::EventType ev)
{
	if (state->inject(ev))
		return true;
	return machine->inject(ev);
}


struct BaseState1 { };

struct StateA :public BaseState1 { StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
struct StateC :public BaseState1 { StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };
struct StateX :public BaseState1 { StateX(ostringstream& oss) : oss_(oss) { oss_ << "StateX ctor. "; } ~StateX() { oss_ << "StateX dtor. "; } ostringstream& oss_; };
struct StateY :public BaseState1 { StateY(ostringstream& oss) : oss_(oss) { oss_ << "StateY ctor. "; } ~StateY() { oss_ << "StateY dtor. "; } ostringstream& oss_; };
struct StateZ :public BaseState1 { StateZ(ostringstream& oss) : oss_(oss) { oss_ << "StateZ ctor. "; } ~StateZ() { oss_ << "StateZ dtor. "; } ostringstream& oss_; };

class StateB : public StateMachine<string>, public BaseState1 {
public:
	StateB(ostringstream& oss) :
		StateMachine(new State<StateX>(oss)),
		oss_(oss)
	{
		oss_ << "StateB ctor. ";
		addEvent<StateX>("XtoY", [](unique_ptr<State<StateX>> from, string) { return new State<StateY>(from->oss_); });
	}
	~StateB() { oss_ << "StateB dtor. "; }
	ostringstream& oss_;
};


class MyHierarchical : public Hierarchical<StateMachine<string>> {
public:
	MyHierarchical(ostringstream& oss) : Hierarchical(new State<StateA>(oss))
	{
		addEvent<StateA>("AtoB", [this](unique_ptr<State<StateA>> from, string) { return new State<StateB>(from->oss_); });
		addPolicy<StateB>(breadthFirstPolicy<StateB, StateMachine<string>>);
		addEvent<StateB>("BtoC", [](unique_ptr<State<StateB>> from, string) { return new State<StateC>(from->oss_); });
	}
};

string getLog(ostringstream& oss)
{
	string ret = oss.str();
	oss.str("");
	oss.clear();
	return ret;
}

TEST(MyHierarchical, simple)
{
	ostringstream oss;
	{
		MyHierarchical hierarchical(oss);
		EXPECT_EQ("StateA ctor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("AtoB"));
		EXPECT_EQ("StateX ctor. StateB ctor. StateA dtor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("XtoY"));
		EXPECT_EQ("StateY ctor. StateX dtor. ", getLog(oss));
	}
	EXPECT_EQ("StateB dtor. StateY dtor. ", getLog(oss));
}

TEST(MyHierarchical, simple2)
{
	ostringstream oss;
	{
		MyHierarchical hierarchical(oss);
		EXPECT_EQ("StateA ctor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("AtoB"));
		EXPECT_EQ("StateX ctor. StateB ctor. StateA dtor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("XtoY"));
		EXPECT_EQ("StateY ctor. StateX dtor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("BtoC"));
		EXPECT_EQ("StateC ctor. StateB dtor. StateY dtor. ", getLog(oss));
	}
	EXPECT_EQ("StateC dtor. ", getLog(oss));
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

class StA : public StateMachine<int>, public BaseState {
public:
	StA() : StateMachine(new State<StX>)
	{
		addEvent<StX>(1, [this](unique_ptr<State<StX>> from, int) { return new State<StY>(); });
	}
	string getStr() const { return getState<BaseState>()->getStr();}
};

class MyHierarchical2 : public Hierarchical<StateMachine<int>> {
public:
	MyHierarchical2(bool depthFirst) : Hierarchical(new State<StA>)
	{
		addEvent<StA>(1, [](unique_ptr<State<StA>> from, int) { return new State<StB>(); });
		if (depthFirst)
			addPolicy<StA>(depthFirstPolicy<StA, StateMachine<int>>);
		else
			addPolicy<StA>(breadthFirstPolicy<StA, StateMachine<int>>);
	}
	string getStr() const { return getState<BaseState>()->getStr();}
};

TEST(MyHierarchical, testDepthFirstPolicy)
{
	MyHierarchical2 hierarchical(true);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject(1));
	EXPECT_EQ("Y", hierarchical.getStr());
}

TEST(MyHierarchical, testBreadthFirstPolicy)
{
	MyHierarchical2 hierarchical(false);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject(1));
	EXPECT_EQ("B", hierarchical.getStr());
}

}
