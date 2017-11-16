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


	template<typename FU>
	struct StateNewer : public StateNewerBase {
		StateHolderBase* newIt(StateHolderBase* from, E ev)
		{
			StateHolder<typename FU::FROM>* oldStateHolder = dynamic_cast<StateHolder<typename FU::FROM>*>(from);
			return new StateHolder<typename FU::TO>((*FU::tr)(oldStateHolder->state, ev));
		};
	};

	template<typename S>
	StateMachine(S* initState) : stateHolder_(new StateHolder<S>(initState)) {}
	~StateMachine() { }

	void inject(E ev)
	{
		stateHolder_ = stateHolder_->inject2(ev);
	}
	template<typename FU>
	void addEvent(E ev)
	{
		MapHolder<typename FU::FROM>::m3[ev] = new StateNewer<FU>;
	}
	StateHolderBase* stateHolder_;

	template<typename F, typename T>
	struct DCTransition {
		using FROM = F;
		using TO = T;
		static TO* tr(FROM* from, E ev)
		{
			delete from;
			TO* to = new TO();
			return to;
		}
	};

	template<typename F, typename T>
	struct CDTransition {
		using FROM = F;
		using TO = T;
		static TO* tr(FROM* from, E ev)
		{
			TO* to = new TO();
			delete from;
			return to;
		}
	};
};

template<typename E>
template<typename S>
map<E, typename StateMachine<E>::StateNewerBase*> StateMachine<E>::MapHolder<S>::m3;



enum class Events { Event0, Event1, Event2 };

template<typename F, typename T>
struct CustomTransition {
	using FROM = F;
	using TO = T;
	static TO* tr(FROM* from, Events ev)
	{
		int v = from->v_+1;
		delete from;
		TO* to = new TO(v);
		return to;
	}
};

class StateA { public: StateA(int v=0) : v_(v) { cout << "StateA ctor: " << v_ << endl; } ~StateA() { cout << "StateA dtor: " << v_ << endl;} int v_;};
class StateB { public: StateB(int v=0) : v_(v) { cout << "StateB ctor: " << v_ << endl; } ~StateB() { cout << "StateB dtor: " << v_ << endl;} int v_;};
class StateC { public: StateC(int v=0) : v_(v) { cout << "StateC ctor: " << v_ << endl; } ~StateC() { cout << "StateC dtor: " << v_ << endl;} int v_;};


class MyStateMachine : public StateMachine<Events> {
public:
	MyStateMachine() : StateMachine(new StateA(7))
	{
		addEvent<DCTransition<StateA, StateB>>(Events::Event0);
		addEvent<DCTransition<StateA, StateC>>(Events::Event1);
		addEvent<CDTransition<StateB, StateC>>(Events::Event0);
		addEvent<DCTransition<StateB, StateA>>(Events::Event1);
		addEvent<DCTransition<StateC, StateA>>(Events::Event0);
		addEvent<CustomTransition<StateC, StateB>>(Events::Event1);
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
