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
struct StateA { StateA(ostringstream& oss) : oss_(oss) { oss_ << "StateA ctor. "; } ~StateA() { oss_ << "StateA dtor. "; } ostringstream& oss_; };
struct StateC { StateC(ostringstream& oss) : oss_(oss) { oss_ << "StateC ctor. "; } ~StateC() { oss_ << "StateC dtor. "; } ostringstream& oss_; };
struct StateX { StateX(ostringstream& oss) : oss_(oss) { oss_ << "StateX ctor. "; } ~StateX() { oss_ << "StateX dtor. "; } ostringstream& oss_; };
struct StateY { StateY(ostringstream& oss) : oss_(oss) { oss_ << "StateY ctor. "; } ~StateY() { oss_ << "StateY dtor. "; } ostringstream& oss_; };
struct StateZ { StateZ(ostringstream& oss) : oss_(oss) { oss_ << "StateZ ctor. "; } ~StateZ() { oss_ << "StateZ dtor. "; } ostringstream& oss_; };


template<typename E, typename B = void>
bool breadthFirstPolicy(Hierarchical<StateMachine<E, B>>* self, E ev)
{
	cout << "executing breadthFirstPolicy!" << endl;
	if (self->StateMachine<E, B>::inject(ev))
		return true;
	if (self->parent())
		return self->parent()->injectPolicy(ev);
	return false; 
}

template<typename E, typename B = void>
bool depthFirstPolicy(Hierarchical<StateMachine<E, B>>* self, E ev)
{
	cout << "executing depthFirstPolicy!" << endl;
	if (self->parent())
	{
		cout << "Rolf0" << endl;
		if (self->parent()->injectPolicy(ev))
		{
		cout << "Rolf1" << endl;
			return true;
		}
	}
	cout << "typename:" << typeid(self).name() << endl;
	cout << "Rolf2" << endl;
	return self->StateMachine<E, B>::inject(ev);
}

class StateB : public Hierarchical<StateMachine<string>> {
public:
	StateB(ostringstream& oss, Hierarchical* parent) :
		Hierarchical(new StateX(oss), breadthFirstPolicy<string>, parent),
		oss_(oss)
	{
		oss_ << "StateB ctor. ";
		addEvent<StateX>("XtoY", [](unique_ptr<StateX> from, string) { StateY* to = new StateY(from->oss_); return new StateHolder<StateY>(to); });
	}
	~StateB() { oss_ << "StateB dtor. "; }
	ostringstream& oss_;
};


class MyHierarchical : public Hierarchical<StateMachine<string>> {
public:
	MyHierarchical(ostringstream& oss) : Hierarchical(new StateA(oss), breadthFirstPolicy<string>)
	{
		addEvent<StateA>("AtoB", [this](unique_ptr<StateA> from, string) {
			StateB* to = new StateB(from->oss_, this);
			return new StateHolder<StateB>(to);
		});
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

struct BaseState { virtual string getStr() const = 0; };
struct StB : BaseState { string getStr() const { return "B";} };
struct StX : BaseState { string getStr() const { return "X";} };
struct StY : BaseState { string getStr() const { return "Y";} };

class StA : public Hierarchical<StateMachine<int, BaseState>>, public BaseState {
public:
	StA(Hierarchical* parent, function<bool(Hierarchical*, int)> policy) :
		Hierarchical(new StX, policy, parent)
	{
		cout << "StA::ctor" << (void*)this << ", par = " << parent << endl;
		addEvent<StX>(1, [this](unique_ptr<StX> from, int) { return new StateHolder<StY>(new StY); });
	}
	string getStr() const { return getState()->getStr();}
};


class MyHierarchical2 : public Hierarchical<StateMachine<int, BaseState>> {
public:
	MyHierarchical2(function<bool(Hierarchical*, int)> policy) : Hierarchical(new StA(this, policy), policy)
	{
		cout << "MyHierarchical2::ctor" << (void*)this << endl;
		addEvent<StA>(1, [this](unique_ptr<StA> from, int) { return new StateHolder<StB>(new StB); });
	}
	string getStr() const { return getState()->getStr();}
};

TEST(MyHierarchical, depthFirstPolicy)
{
	MyHierarchical2 hierarchical(depthFirstPolicy<int, BaseState>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject(1));
	EXPECT_EQ("B", hierarchical.getStr());
}

TEST(MyHierarchical, breadthFirstPolicy)
{
	MyHierarchical2 hierarchical(breadthFirstPolicy<int, BaseState>);
	EXPECT_EQ("X", hierarchical.getStr());
	EXPECT_TRUE(hierarchical.inject(1));
	EXPECT_EQ("Y", hierarchical.getStr());
}

struct I {};
struct SM { SM(I* i){} void foo() { cout << "SM::foo" << endl; } };
struct H
{
	H(H* parent = nullptr) : parent_(parent) { if (parent_) parent_->child_.insert(this); }
	~H() { if (parent_) parent_->child_.erase(this); }
	H* parent_;
	set<H*> child_;
	void dump() {
		cout << "this = " << (void*)this << endl;
		for (H* c: child_)
			cout << "child = " << (void*)c << endl;
		cout << "parent = " << (void*)parent_ << endl;
	}
	void foo() { cout << "H::foo" << endl; }
};

template<typename IS>
struct HSM : H,SM {
	HSM(H* parent, function<IS*()> f) : H(parent), SM(f()) {}
	
};

struct HChild : public HSM<I>, I {
	HChild(H* parent) : HSM(parent, [](){ return new I;}) { }
};


struct HPar : public HSM<I> {
	HPar() : HSM(nullptr, [this](){return new HChild(this);}) { }
};

TEST(teste, tsets)
{
	HPar par;
	//par.foo();
	par.dump();
	for (H* c: par.child_)
		c->dump();
}

}
