#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>

using namespace std;

/*
/----\       /----------------------------------\      /----\
|A   |       |B                                 |      |C   |
|    +--E0-->+  /----\      /----\      /----\  +------+    +
|    |       |  |    |      |    |      |    |  |      |    |
|    |       |  | X  +------+ Y  +------+ Z  |  |      |    |
|    |       |  |    |      |    |      |    |  |      |    |
|    |       |  \----/      \----/      \----/  |      |    |
|    |       |                                  |      |    |
\----/       \----------------------------------/      \----/
*/

namespace {
struct StateA { StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
struct StateC { StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };
struct StateX { StateX(ostringstream& oss) : oss_(oss) { oss_ << "StateX ctor. "; } ~StateX() { oss_ << "StateX dtor. "; } ostringstream& oss_; };
struct StateY { StateY(ostringstream& oss) : oss_(oss) { oss_ << "StateY ctor. "; } ~StateY() { oss_ << "StateY dtor. "; } ostringstream& oss_; };
struct StateZ { StateZ(ostringstream& oss) : oss_(oss) { oss_ << "StateZ ctor. "; } ~StateZ() { oss_ << "StateZ dtor. "; } ostringstream& oss_; };

template<typename E>
class Hierarchy {
public:
	Hierarchy(StateMachine<E>& par, function<bool(E)> f) : par_(par)
	{
		par_.injectEx_ = f;
	}
	~Hierarchy() { par_.injectEx_ = nullptr; }
	StateMachine<E>& par_;
};

class StateB : public StateMachine<string>, Hierarchy<string> {
public:
	StateB(ostringstream& oss, StateMachine<string>& par) :
		StateMachine(new StateX(oss)),
		Hierarchy(par, [this](string ev){if (inject(ev)) return true; return par_.injectIn(ev);}),
		oss_(oss)
	{
		oss_ << "StateB ctor. ";
		addEvent<StateX>("XtoY", [](StateX* from, string) { StateY* to = new StateY(from->oss_); delete from; return new StateHolder<StateY>(to); });
	}
	~StateB() { oss_ << "StateB dtor. "; }
	ostringstream& oss_;
};

class Hierarchical : public StateMachine<string> {
public:
	Hierarchical(ostringstream& oss) : StateMachine(new StateA(oss))
	{
		addEvent<StateA>("AtoB", [this](StateA* from, string) {
			StateB* to = new StateB(from->oss_, *this);
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

TEST(Hierarchical, simple)
{
	ostringstream oss;
	{
		Hierarchical hierarchical(oss);
		EXPECT_EQ("StateA ctor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("AtoB"));
		EXPECT_EQ("StateX ctor. StateB ctor. StateA dtor. ", getLog(oss));
		EXPECT_TRUE(hierarchical.inject("XtoY"));
		EXPECT_EQ("StateY ctor. StateX dtor. ", getLog(oss));
	}
	EXPECT_EQ("StateB dtor. StateY dtor. ", getLog(oss));
}

TEST(Hierarchical, simple2)
{
	ostringstream oss;
	{
		Hierarchical hierarchical(oss);
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
