#ifndef STATE_MACHINE_H_
#define STATE_MACHINE_H_
#include <map>
#include <typeinfo>
#include <memory>

template<typename M>
class Hierarchical : public M {
	struct PolicyBase { virtual bool doIt(typename M::StateHolderBase*, M* machine, typename M::EventType) = 0;};
	template<typename S>
	struct Policy2 : PolicyBase {
		Policy2(std::function<bool(S* state, M* machine, typename M::EventType ev)> func) : func_(func) {}
		virtual bool doIt(typename M::StateHolderBase* state, M* machine, typename M::EventType ev) override {
			return func_(dynamic_cast<typename M::template StateHolder<S>*>(state)->state_.get(), machine, ev);
		}
		std::function<bool(S* state, M* machine, typename M::EventType ev)> func_;
	};
public:
	using Policy = std::function<bool(Hierarchical*, typename M::EventType)>;
	template<typename S>
	Hierarchical(S* initState) : M(initState) {  }
	bool inject(typename M::EventType ev) {
		typename Policies::const_iterator ittr = policies_.find(M::stateHolder_->getHash());
		if (ittr == policies_.end())
			return M::inject(ev);
		return ittr->second->doIt(M::stateHolder_.get(), this, ev);
	}
	bool injectMachine(typename M::EventType ev) { return M::inject(ev); }
	template<typename S>
	void addPolicy(std::function<bool(S* state, M* machine, typename M::EventType ev)> func) {
		policies_[typeid(S).hash_code()] = std::make_unique<Policy2<S>>(func);
	}
private:
	using Policies = std::map<size_t, std::unique_ptr<PolicyBase>>;
	Policies policies_;
};

template<typename E, typename B = void>
class StateMachine {
	struct TransitionBase;
	using Transitions = std::map<std::pair<size_t, E>, std::unique_ptr<TransitionBase>>;
protected:
	class StateHolderBase { public: virtual ~StateHolderBase(){} virtual B* getState() const = 0; virtual size_t getHash() const = 0; };
private:
	struct TransitionBase { virtual StateHolderBase* newIt(StateHolderBase*, E) = 0;};
public:
	using EventType = E;
	template<typename S>
	class StateHolder : public StateHolderBase {
	public:
		std::unique_ptr<S> state_;
	public:
		StateHolder(S* state) : state_(state) {}
		virtual size_t getHash() const { return typeid(S).hash_code(); }
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
		typename Transitions::const_iterator ittr = transitions_.find(std::make_pair(stateHolder_->getHash(), ev));
		if (ittr == transitions_.end())
			return false;
		stateHolder_.reset(ittr->second->newIt(stateHolder_.get(), ev));
		return true;
	}
	template<typename FROM>
	void addEvent(E ev, std::function<StateHolderBase*(std::unique_ptr<FROM>, E)> func)
	{
		transitions_[std::make_pair(typeid(FROM).hash_code(), ev)] = std::make_unique<Transition<FROM>>(func);
	}
	B* getState() const { return stateHolder_->getState(); }
protected:
	std::unique_ptr<StateHolderBase> stateHolder_;
private:
	Transitions transitions_;
};
#endif
