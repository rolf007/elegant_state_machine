#include <iostream>
#include <map>

using namespace std;

template<typename E>
class StateMachine {
public:
	class StateHolderBase { public: virtual ~StateHolderBase() {} virtual StateHolderBase* inject2(E ev) = 0; };

	struct StateNewerBase { virtual StateHolderBase* newIt(StateHolderBase*, E)=0;};

	template<typename S>
	struct MapHolder { static map<E, StateNewerBase*> m3; };

	template<typename S>
	class StateHolder : public StateHolderBase {
	public:
		StateHolder(S* state) : state(state) {}
		virtual StateHolderBase* inject2(E ev)
		{
			typename map<E, StateNewerBase*>::const_iterator ittr = MapHolder<S>::m3.find(ev);
			if (ittr == MapHolder<S>::m3.end())
				return this;
			StateHolderBase* newStateHolder = ittr->second->newIt(this, ev);
			delete this;
			return newStateHolder;
		}
		S* state;
	};

	template<typename FROM>
	struct StateNewer : public StateNewerBase {
		StateNewer(StateHolderBase*(*func)(FROM*, E)) : func_(func) {
		}
		StateHolderBase* newIt(StateHolderBase* from, E ev)
		{
			StateHolder<FROM>* oldStateHolder = dynamic_cast<StateHolder<FROM>*>(from);
			return (*func_)(oldStateHolder->state, ev);
		};
		StateHolderBase*(*func_)(FROM*, E);
	};

	template<typename S>
	StateMachine(S* initState) : stateHolder_(new StateHolder<S>(initState)) {}
	~StateMachine() { }

	void inject(E ev)
	{
		stateHolder_ = stateHolder_->inject2(ev);
	}
	template<typename FROM>
	void addEvent(E ev, StateHolderBase*(*func)(FROM*, E))
	{
		MapHolder<FROM>::m3[ev] = new StateNewer<FROM>(func);
	}
	StateHolderBase* stateHolder_;

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
};

template<typename E>
template<typename S>
map<E, typename StateMachine<E>::StateNewerBase*> StateMachine<E>::MapHolder<S>::m3;

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
