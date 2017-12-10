#include "state_machine.h"
#include <iostream>
#include <ostream>
#include <gtest/gtest.h>
#include <set>

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

template<typename S, typename M, typename E>
bool breadthFirstPolicy(S* state, M* machine, E ev)
{
	if (machine->inject(ev))
		return true;
	return state->inject(ev) ;
}


struct BaseState1 { virtual bool inject(string) { return false; } };

struct StateA :public BaseState1 { StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
struct StateC :public BaseState1 { StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };
struct StateX :public BaseState1 { StateX(ostringstream& oss) : oss_(oss) { oss_ << "StateX ctor. "; } ~StateX() { oss_ << "StateX dtor. "; } ostringstream& oss_; };
struct StateY :public BaseState1 { StateY(ostringstream& oss) : oss_(oss) { oss_ << "StateY ctor. "; } ~StateY() { oss_ << "StateY dtor. "; } ostringstream& oss_; };
struct StateZ :public BaseState1 { StateZ(ostringstream& oss) : oss_(oss) { oss_ << "StateZ ctor. "; } ~StateZ() { oss_ << "StateZ dtor. "; } ostringstream& oss_; };

class StateB : public StateMachine<string, BaseState1>, public BaseState1 {
public:
	StateB(ostringstream& oss) :
		StateMachine(new StateX(oss)),
		oss_(oss)
	{
		oss_ << "StateB ctor. ";
		addEvent<StateX>("XtoY", [](unique_ptr<StateX> from, string) { StateY* to = new StateY(from->oss_); return new StateHolder<StateY>(to); });
	}
	~StateB() { oss_ << "StateB dtor. "; }
	ostringstream& oss_;
	virtual bool inject(string ev) override { return StateMachine::inject(ev); }
};


class MyHierarchical : public Hierarchical<StateMachine<string, BaseState1>> {
public:
	MyHierarchical(ostringstream& oss) : Hierarchical(new StateA(oss))
	{
		addEvent<StateA>("AtoB", [this](unique_ptr<StateA> from, string) {
			StateB* to = new StateB(from->oss_);
			return new StateHolder<StateB>(to);
		});
		addPolicy<StateB>(breadthFirstPolicy<StateB, StateMachine<string, BaseState1>, string>);
		addEvent<StateB>("BtoC", [](unique_ptr<StateB> from, string) { StateC* to = new StateC(from->oss_); return new StateHolder<StateC>(to); });
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

struct BaseState { virtual string getStr() const = 0; virtual bool inject(int) { return false; } };
struct StB : BaseState { string getStr() const { return "B";} };
struct StX : BaseState { string getStr() const { return "X";} };
struct StY : BaseState { string getStr() const { return "Y";} };

class StA : public Hierarchical<StateMachine<int, BaseState>>, public BaseState {
public:
	StA() : Hierarchical(new StX)
	{
		addEvent<StX>(1, [this](unique_ptr<StX> from, int) { return new StateHolder<StY>(new StY); });
	}
	string getStr() const { return getState()->getStr();}
	virtual bool inject(int ev) override { return StateMachine::inject(ev); }
};

template<typename S, typename M, typename E>
bool depthFirstPolicy(S* state, M* machine, E ev)
{
	if (state->inject(ev))
		return true;
	return machine->inject(ev);
}

class MyHierarchical2 : public Hierarchical<StateMachine<int, BaseState>> {
public:
	MyHierarchical2(bool depthFirst) : Hierarchical(new StA())
	{
		addEvent<StA>(1, [](unique_ptr<StA> from, int) { return new StateHolder<StB>(new StB); });
		if (depthFirst)
			addPolicy<StA>(depthFirstPolicy<StA, StateMachine<int, BaseState>, int>);
		else
			addPolicy<StA>(breadthFirstPolicy<StA, StateMachine<int, BaseState>, int>);
	}
	string getStr() const { return getState()->getStr();}
};

TEST(MyHierarchical, testDepthFirstPolicy)
{
	MyHierarchical2 hierarchical(true);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject(1));
	EXPECT_EQ("Y", hierarchical.getStr());
}

TEST(MyHierarchical, breadthFirstPolicy)
{
	MyHierarchical2 hierarchical(false);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject(1));
	EXPECT_EQ("B", hierarchical.getStr());
}

struct I {};
struct SM { SM(I* i){} void foo() { cout << "SM::foo" << endl; } };
struct H;
struct H
{
	static map<H*, set<H*>> child;
	H(H* parent = nullptr) : parent_(parent) { if (parent_) child[parent_].insert(this); }
	~H() { if (parent_) child[parent_].erase(this); }
	H* parent_;
	void dump() {
		cout << "this = " << (void*)this << endl;
		for (H* c: child[this])
			cout << "child = " << (void*)c << endl;
		cout << "parent = " << (void*)parent_ << endl;
	}
	void foo() { cout << "H::foo" << endl; }
};
map<H*, set<H*>> H::child;



struct HSM : H,SM {
	HSM(H* parent, I* f) : H(parent), SM(f) { }
};

struct HChild : public HSM, I {
	HChild(H* parent) : HSM(parent, new I) { }
};


struct HPar : public HSM {
	HPar() : HSM(nullptr, new HChild(this)) {  }
};

TEST(teste, tsets)
{
	HPar par;
	//par.dump();
	//for (H* c: par.child_)
	//	c->dump();
	EXPECT_EQ(H::child[&par].size(), 1);
	H* child = *H::child[&par].begin();
	EXPECT_EQ(par.parent_, nullptr);
	EXPECT_EQ(child->parent_, &par);
	EXPECT_EQ(H::child[child].size(), 0);

	HChild* child2 = new HChild(&par);
	delete child;

	EXPECT_EQ(H::child[&par].size(), 1);
	EXPECT_EQ(child2, *H::child[&par].begin());
	EXPECT_EQ(par.parent_, nullptr);
	EXPECT_EQ(child2->parent_, &par);
	EXPECT_EQ(H::child[child2].size(), 0);
}

}
