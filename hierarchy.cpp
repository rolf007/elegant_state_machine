#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>

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
struct StateA { StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
struct StateC { StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };
struct StateX { StateX(ostringstream& oss) : oss_(oss) { oss_ << "StateX ctor. "; } ~StateX() { oss_ << "StateX dtor. "; } ostringstream& oss_; };
struct StateY { StateY(ostringstream& oss) : oss_(oss) { oss_ << "StateY ctor. "; } ~StateY() { oss_ << "StateY dtor. "; } ostringstream& oss_; };
struct StateZ { StateZ(ostringstream& oss) : oss_(oss) { oss_ << "StateZ ctor. "; } ~StateZ() { oss_ << "StateZ dtor. "; } ostringstream& oss_; };


class StateB : public Hierarchical<StateMachine<string>> {
public:
	StateB(ostringstream& oss, Hierarchical<StateMachine<string>>* parent) :
		Hierarchical(new StateX(oss), parent),
		oss_(oss)
	{
		oss_ << "StateB ctor. ";
		addEvent<StateX>("XtoY", [](StateX* from, string) { StateY* to = new StateY(from->oss_); delete from; return new StateHolder<StateY>(to); });
	}
	~StateB() { oss_ << "StateB dtor. "; }
	ostringstream& oss_;
};


class MyHierarchical : public Hierarchical<StateMachine<string>> {
public:
	MyHierarchical(ostringstream& oss) : Hierarchical(new StateA(oss))
	{
		addEvent<StateA>("AtoB", [this](StateA* from, string) {
			StateB* to = new StateB(from->oss_, this);
			delete from;
			return new StateHolder<StateB>(to);
		});
		addEvent<StateB>("BtoC", [](StateB* from, string) { StateC* to = new StateC(from->oss_); delete from; return new StateHolder<StateC>(to); });
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

}
