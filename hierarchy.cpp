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


struct BaseState1 { virtual bool inject(string) { return false; } };

struct StateA :public BaseState1 { StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
struct StateC :public BaseState1 { StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };
struct StateX :public BaseState1 { StateX(ostringstream& oss) : oss_(oss) { oss_ << "StateX ctor. "; } ~StateX() { oss_ << "StateX dtor. "; } ostringstream& oss_; };
struct StateY :public BaseState1 { StateY(ostringstream& oss) : oss_(oss) { oss_ << "StateY ctor. "; } ~StateY() { oss_ << "StateY dtor. "; } ostringstream& oss_; };
struct StateZ :public BaseState1 { StateZ(ostringstream& oss) : oss_(oss) { oss_ << "StateZ ctor. "; } ~StateZ() { oss_ << "StateZ dtor. "; } ostringstream& oss_; };

class StateB : public StateMachine<string>, public BaseState1 {
public:
	StateB(ostringstream& oss) :
		StateMachine(new StateHolder<StateX>(oss)),
		oss_(oss)
	{
		oss_ << "StateB ctor. ";
		addEvent<StateX>("XtoY", [](unique_ptr<StateHolder<StateX>> from, string) { return new StateHolder<StateY>(from->oss_); });
	}
	~StateB() { oss_ << "StateB dtor. "; }
	ostringstream& oss_;
	virtual bool inject(string ev) override { return StateMachine::inject(ev); }
};


class MyHierarchical : public Hierarchical<StateMachine<string>> {
public:
	MyHierarchical(ostringstream& oss) : Hierarchical(new StateHolder<StateA>(oss))
	{
		addEvent<StateA>("AtoB", [this](unique_ptr<StateHolder<StateA>> from, string) {
			return new StateHolder<StateB>(from->oss_);
		});
		addPolicy<StateB>(breadthFirstPolicy<StateB, StateMachine<string>>);
		addEvent<StateB>("BtoC", [](unique_ptr<StateHolder<StateB>> from, string) { return new StateHolder<StateC>(from->oss_); });
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

class StA : public Hierarchical<StateMachine<int>>, public BaseState {
public:
	StA() : Hierarchical(new StateHolder<StX>)
	{
		addEvent<StX>(1, [this](unique_ptr<StateHolder<StX>> from, int) { return new StateHolder<StY>(); });
	}
	string getStr() const { return getState<BaseState>()->getStr();}
	virtual bool inject(int ev) override { return StateMachine::inject(ev); }
};

class MyHierarchical2 : public Hierarchical<StateMachine<int>> {
public:
	MyHierarchical2(bool depthFirst) : Hierarchical(new StateHolder<StA>)
	{
		addEvent<StA>(1, [](unique_ptr<StateHolder<StA>> from, int) { return new StateHolder<StB>(); });
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
