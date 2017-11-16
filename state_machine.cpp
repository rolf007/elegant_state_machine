#include <iostream>
#include <map>

using namespace std;;

template<typename E>
class StateHolderBase { public: virtual ~StateHolderBase() {} virtual StateHolderBase<E>* inject2(E ev) = 0; };

template<typename E>
struct StateNewerBase { virtual StateHolderBase<E>* newIt(StateHolderBase<E>*, E)=0;};

template<typename FU, typename E>
struct StateNewer : public StateNewerBase<E> { StateHolderBase<E>* newIt(StateHolderBase<E>*, E); };

template<typename E, typename S>
struct MapHolder { static map<E, StateNewerBase<E>*> m3; };

template<typename E, typename S>
map<E, StateNewerBase<E>*> MapHolder<E, S>::m3;

template<typename S, typename E>
class StateHolder : public StateHolderBase<E> {
public:
	StateHolder(S* state) : state(state) {}
	virtual StateHolderBase<E>* inject2(E ev)
	{
		typename map<E, StateNewerBase<E>*>::const_iterator ittr = MapHolder<E, S>::m3.find(ev);
		if (ittr == MapHolder<E, S>::m3.end())
			return this;
		StateHolderBase<E>* newStateHolder = ittr->second->newIt(this, ev);
		delete this;
		return newStateHolder;
	}
	S* state;
};


template<typename FU, typename E>
StateHolderBase<E>* StateNewer<FU, E>::newIt(StateHolderBase<E>* from, E ev) {
	StateHolder<typename FU::FROM, E>* oldStateHolder = dynamic_cast<StateHolder<typename FU::FROM, E>*>(from);
	return new StateHolder<typename FU::TO, E>((*FU::tr)(oldStateHolder->state, ev));
};



template<typename E>
class StateMachine {
public:
	template<typename S>
	StateMachine(S* initState) : stateHolder_(new StateHolder<S, E>(initState)) {}
	~StateMachine() { }

	void inject(E ev)
	{
		stateHolder_ = stateHolder_->inject2(ev);
	}
	template<typename FU>
	void addEvent(E ev)
	{
		MapHolder<E, typename FU::FROM>::m3[ev] = new StateNewer<FU, E>;
	}
	StateHolderBase<E>* stateHolder_;
};



template<typename F, typename T, typename E>
struct Transit {
	using FROM = F;
	using TO = T;
	static TO* tr(FROM* from, E ev)
	{
		int v = from->v_;
		delete from;
		TO* to = new TO(v);
		return to;
	}
};

class StateA { public: StateA(int v) : v_(v) { cout << "StateA ctor: " << v_ << endl; } ~StateA() { cout << "StateA dtor: " << v_ << endl;} int v_;};
class StateB { public: StateB(int v) : v_(v) { cout << "StateB ctor: " << v_ << endl; } ~StateB() { cout << "StateB dtor: " << v_ << endl;} int v_;};
class StateC { public: StateC(int v) : v_(v) { cout << "StateC ctor: " << v_ << endl; } ~StateC() { cout << "StateC dtor: " << v_ << endl;} int v_;};
enum class Events { Event0, Event1, Event2 };

class MyStateMachine : public StateMachine<Events> {
public:
	MyStateMachine() : StateMachine(new StateA(7))
	{
		addEvent<Transit<StateA, StateB, Events>>(Events::Event0);
		addEvent<Transit<StateA, StateC, Events>>(Events::Event1);
		addEvent<Transit<StateB, StateC, Events>>(Events::Event0);
		addEvent<Transit<StateB, StateA, Events>>(Events::Event1);
		addEvent<Transit<StateC, StateA, Events>>(Events::Event0);
		addEvent<Transit<StateC, StateB, Events>>(Events::Event1);
	}
};


int main()
{
	MyStateMachine myStateMachine;
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event0);
	myStateMachine.inject(Events::Event1);
	myStateMachine.inject(Events::Event1);
	myStateMachine.inject(Events::Event1);

	return 0;
}
