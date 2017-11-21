#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_

#include <map>
#include <typeinfo>
#include <memory>

template<typename E>
class StateMachine {
	struct TransitionBase;

	using Transitions = std::map<std::pair<size_t, E>, std::unique_ptr<TransitionBase>>;

	class StateHolderBase { public: virtual ~StateHolderBase(){} virtual StateHolderBase* inject2(E ev, const Transitions& transitions) = 0; };

	struct TransitionBase { virtual StateHolderBase* newIt(StateHolderBase*, E) = 0;};
public:

	template<typename S>
	class StateHolder : public StateHolderBase {
	public:
		std::unique_ptr<S> state_;
	public:
		StateHolder(S* state) : state_(state) {}
		StateHolderBase* inject2(E ev, const Transitions& transitions) override
		{
			typename Transitions::const_iterator ittr2 = transitions.find(std::make_pair(typeid(S).hash_code(), ev));
			if (ittr2 == transitions.end())
				return nullptr;
			return ittr2->second->newIt(this, ev);
		}
		StateHolderBase* handleEvent(E ev, std::function<StateHolderBase*(S*, E)> func)
		{
			return func(state_.release(), ev);
		}
	};
private:
	template<typename FROM>
	struct Transition : public TransitionBase {
		Transition(std::function<StateHolderBase*(FROM*, E)> func) : func_(func) {}
		StateHolderBase* newIt(StateHolderBase* from, E ev) override
		{
			return dynamic_cast<StateHolder<FROM>*>(from)->handleEvent(ev, func_);
		};
		std::function<StateHolderBase*(FROM*, E)> func_;
	};
public:
	template<typename S>
	StateMachine(S* initState) : stateHolder_(std::make_unique<StateHolder<S>>(initState)) {}
	virtual ~StateMachine() {}

	bool injectIn(E ev)
	{
		if (StateHolderBase* newState = stateHolder_->inject2(ev, transitions_)) {
			stateHolder_.reset(newState);
			return true;
		}
		return false;
	}
	bool inject(E ev)
	{
		if (injectEx_ != nullptr)
			return injectEx_(ev);
		return injectIn(ev);
	}
	template<typename FROM>
	void addEvent(E ev, std::function<StateHolderBase*(FROM*, E)> func)
	{
		transitions_[std::make_pair(typeid(FROM).hash_code(), ev)] = std::make_unique<Transition<FROM>>(func);
	}

	std::function<bool(E ev)> injectEx_;
private:
	std::unique_ptr<StateHolderBase> stateHolder_;
	Transitions transitions_;
};
#endif
