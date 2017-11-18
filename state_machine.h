#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

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
#endif
