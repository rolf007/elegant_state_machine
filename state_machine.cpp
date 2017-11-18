#include <iostream>
#include <map>
#include <typeinfo>
#include <memory>

using namespace std;

template<typename E>
class StateMachine {
	struct TransitionBase;

	using Transitions = map<pair<size_t, E>, unique_ptr<TransitionBase>>;

	class StateHolderBase { public: virtual StateHolderBase* inject2(E ev, const Transitions& transitions) = 0; virtual void end() = 0;};

	struct TransitionBase { virtual StateHolderBase* newIt(StateHolderBase*, E) = 0;};
public:

	template<typename S>
	class StateHolder : public StateHolderBase {
		S* state_;
	public:
		StateHolder(S* state) : state_(state) {}
		StateHolderBase* inject2(E ev, const Transitions& transitions) override
		{
			typename Transitions::const_iterator ittr2 = transitions.find(make_pair(typeid(S).hash_code(), ev));
			if (ittr2 == transitions.end())
				return nullptr;
			return ittr2->second->newIt(this, ev);
		}
		void end() override { delete state_; }
		StateHolderBase* handleEvent(E ev, StateHolderBase*(*func)(S*, E))
		{
			return (*func)(state_, ev);
		}
	};
private:
	template<typename FROM>
	struct Transition : public TransitionBase {
		Transition(StateHolderBase*(*func)(FROM*, E)) : func_(func) {}
		StateHolderBase* newIt(StateHolderBase* from, E ev) override
		{
			return dynamic_cast<StateHolder<FROM>*>(from)->handleEvent(ev, func_);
		};
		StateHolderBase*(*func_)(FROM*, E);
	};
public:
	template<typename S>
	StateMachine(S* initState) : stateHolder_(make_unique<StateHolder<S>>(initState)) {}
	~StateMachine() { stateHolder_->end(); }

	void inject(E ev)
	{
		if (StateHolderBase* newState = stateHolder_->inject2(ev, transitions_))
			stateHolder_.reset(newState);
	}
	template<typename FROM>
	void addEvent(E ev, StateHolderBase*(*func)(FROM*, E))
	{
		transitions_[make_pair(typeid(FROM).hash_code(), ev)] = make_unique<Transition<FROM>>(func);
	}

	template<typename FROM, typename TO>
	static StateHolderBase* dcTransition(FROM* from, E ev)
	{
		delete from;
		TO* to = new TO();
		return new StateHolder<TO>(to);
	}

	template<typename FROM, typename TO>
	static StateHolderBase* cdTransition(FROM* from, E ev)
	{
		TO* to = new TO();
		delete from;
		return new StateHolder<TO>(to);
	}
private:
	unique_ptr<StateHolderBase> stateHolder_;
	Transitions transitions_;
};

enum class Events { Event0, Event1, Event2 };


class StateA { public: StateA(int v=0) : v_(v) { cout << "StateA ctor: " << v_ << endl; } ~StateA() { cout << "StateA dtor: " << v_ << endl;} int v_;};
class StateB { public: StateB(int v=0) : v_(v) { cout << "StateB ctor: " << v_ << endl; } ~StateB() { cout << "StateB dtor: " << v_ << endl;} int v_;};
class StateC { public: StateC(int v=0) : v_(v) { cout << "StateC ctor: " << v_ << endl; } ~StateC() { cout << "StateC dtor: " << v_ << endl;} int v_;};


class MyStateMachine : public StateMachine<Events> {
	template<typename FROM, typename TO>
	static StateHolderBase* customTransition(FROM* from, Events ev)
	{
		int v = from->v_+1;
		delete from;
		TO* to = new TO(v);
		return new StateHolder<TO>(to);
	}
public:
	MyStateMachine() : StateMachine(new StateA(7))
	{
		addEvent(Events::Event0, dcTransition<StateA, StateB>);
		addEvent(Events::Event1, dcTransition<StateA, StateC>);
		addEvent(Events::Event0, cdTransition<StateB, StateC>);
		addEvent(Events::Event1, dcTransition<StateB, StateA>);
		addEvent(Events::Event0, customTransition<StateC, StateA>);
		addEvent(Events::Event1, dcTransition<StateC, StateB>);
	}
};


int main()
{
	MyStateMachine myStateMachine;
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event2);
	myStateMachine.inject(Events::Event1);
	myStateMachine.inject(Events::Event1);
	myStateMachine.inject(Events::Event1);

	return 0;
}
