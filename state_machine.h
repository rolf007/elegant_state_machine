#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_
#include <map>
#include <typeinfo>
#include <memory>
#include <iostream>

template<typename M>
class Hierarchical : public M
{
public:
	using Policy = std::function<bool(Hierarchical*, typename M::EventType)>;
	template<typename S>
	Hierarchical(S* initState, Policy policy, Hierarchical* parent = nullptr) : M(initState), parent_(parent), child_(nullptr), policy_(policy) { if (parent_) parent_->child_ = this; }
	~Hierarchical() { if (parent_) parent_->child_ = nullptr; }
	bool inject(typename M::EventType ev) { std::cout << "inject:" << (void*)this << std::endl; if (child_) return child_->inject(ev); std::cout << "no child_" << std::endl; return policy_(this, ev); }
	bool injectPolicy(typename M::EventType ev) { return policy_(this, ev); }
	Hierarchical* parent() const { return parent_; }
private:
	Hierarchical* parent_, * child_;
	Policy policy_;
};

template<typename E, typename B = void>
class StateMachine {
	struct TransitionBase;
	using Transitions = std::map<std::pair<size_t, E>, std::unique_ptr<TransitionBase>>;
	class StateHolderBase { public: virtual ~StateHolderBase(){} virtual StateHolderBase* inject2(E ev, const Transitions& transitions) = 0; virtual B* getState() const = 0; };
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
		StateHolderBase* handleEvent(E ev, std::function<StateHolderBase*(std::unique_ptr<S>, E)> func)
		{
			return func(move(state_), ev);
		}
		virtual B* getState() const override { return static_cast<B*>(state_.get()); }
	};
private:
	template<typename FROM>
	struct Transition : public TransitionBase {
		Transition(std::function<StateHolderBase*(std::unique_ptr<FROM>, E)> func) : func_(func) {}
		StateHolderBase* newIt(StateHolderBase* from, E ev) override
		{
			return dynamic_cast<StateHolder<FROM>*>(from)->handleEvent(ev, func_);
		};
		std::function<StateHolderBase*(std::unique_ptr<FROM>, E)> func_;
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
	void addEvent(E ev, std::function<StateHolderBase*(std::unique_ptr<FROM>, E)> func)
	{
		transitions_[std::make_pair(typeid(FROM).hash_code(), ev)] = std::make_unique<Transition<FROM>>(func);
	}
	B* getState() const
	{
		return stateHolder_->getState();
	}
private:
	std::unique_ptr<StateHolderBase> stateHolder_;
	Transitions transitions_;
};
#endif
