#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_
#include <map>
#include <typeinfo>
#include <memory>

template<typename M>
class Hierarchical : public M
{
public:
	template<typename S>
	Hierarchical(S* initState, Hierarchical<M>* parent = nullptr) : M(initState), parent_(parent), child_(nullptr) { if (parent_) parent_->child_ = this; }
	~Hierarchical() { if (parent_) parent_->child_ = nullptr; }
	bool inject(typename M::EventType ev) { if (child_) return child_->inject(ev); return injectBack(ev); }
private:
	bool injectBack(typename M::EventType ev) { if (M::inject(ev)) return true; if (parent_) return parent_->injectBack(ev); return false; }
	Hierarchical<M>* parent_, * child_;
};

template<typename E>
class StateMachine {
	struct TransitionBase;
	using Transitions = std::map<std::pair<size_t, E>, std::unique_ptr<TransitionBase>>;
	class StateHolderBase { public: virtual ~StateHolderBase(){} virtual StateHolderBase* inject2(E ev, const Transitions& transitions) = 0; };
	struct TransitionBase { virtual StateHolderBase* newIt(StateHolderBase*, E) = 0;};
public:
	using EventType = E;
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
	bool inject(E ev)
	{
		if (StateHolderBase* newState = stateHolder_->inject2(ev, transitions_)) {
			stateHolder_.reset(newState);
			return true;
		}
		return false;
	}
	template<typename FROM>
	void addEvent(E ev, std::function<StateHolderBase*(FROM*, E)> func)
	{
		transitions_[std::make_pair(typeid(FROM).hash_code(), ev)] = std::make_unique<Transition<FROM>>(func);
	}
private:
	std::unique_ptr<StateHolderBase> stateHolder_;
	Transitions transitions_;
};
#endif
