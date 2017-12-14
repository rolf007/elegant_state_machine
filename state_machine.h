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
		Policy2(std::function<bool(S* state, M* machine, typename M::EventType ev)> policy) : policyFunc_(policy) {}
		virtual bool doIt(typename M::StateHolderBase* state, M* machine, typename M::EventType ev) override {
			return policyFunc_(dynamic_cast<typename M::template StateHolder<S>*>(state), machine, ev);
		}
		std::function<bool(S* state, M* machine, typename M::EventType ev)> policyFunc_;
	};
public:
	using Policy = std::function<bool(Hierarchical*, typename M::EventType)>;
	template<typename S>
	Hierarchical(S* initState) : M(initState) {}
	bool inject(typename M::EventType ev) {
		typename Policies::const_iterator ittr = policies_.find(M::stateHolder_->getHash());
		if (ittr == policies_.end())
			return M::inject(ev);
		return ittr->second->doIt(M::stateHolder_, this, ev);
	}
	template<typename S>
	void addPolicy(std::function<bool(S* state, M* machine, typename M::EventType ev)> func) {
		policies_[typeid(S).hash_code()] = std::make_unique<Policy2<S>>(func);
	}
private:
	using Policies = std::map<size_t, std::unique_ptr<PolicyBase>>;
	Policies policies_;
};

template<typename E>
class StateMachine {
	struct TransitionBase;
	using Transitions = std::map<std::pair<size_t, E>, std::unique_ptr<TransitionBase>>;
protected:
	class StateHolderBase { public: virtual ~StateHolderBase(){} virtual size_t getHash() const = 0; template<typename B> B* getState() override { return dynamic_cast<B*>(this); }
};
private:
	struct TransitionBase { virtual StateHolderBase* newIt(StateHolderBase*, E) = 0;};
public:
	using EventType = E;
	template<typename S>
	struct StateHolder : public StateHolderBase, public S {
		using S::S;
		virtual size_t getHash() const { return typeid(S).hash_code(); }
	};
private:
	template<typename FROM>
	struct Transition : public TransitionBase {
		Transition(std::function<StateHolderBase*(std::unique_ptr<StateHolder<FROM>>, E)> func) : func_(func) {}
		StateHolderBase* newIt(StateHolderBase* from, E ev) override
		{
			return func_(std::unique_ptr<StateHolder<FROM>>(dynamic_cast<StateHolder<FROM>*>(from)), ev);
		};
		std::function<StateHolderBase*(std::unique_ptr<StateHolder<FROM>>, E)> func_;
	};
public:
	template<typename S>
	StateMachine(S* initState) : stateHolder_(initState) {}
	virtual ~StateMachine() { delete stateHolder_; }
	bool inject(E ev)
	{
		typename Transitions::const_iterator ittr = transitions_.find(std::make_pair(stateHolder_->getHash(), ev));
		if (ittr == transitions_.end())
			return false;
		stateHolder_ = ittr->second->newIt(stateHolder_, ev);
		return true;
	}
	template<typename FROM>
	void addEvent(E ev, std::function<StateHolderBase*(std::unique_ptr<StateHolder<FROM>>, E)> func)
	{
		transitions_[std::make_pair(typeid(FROM).hash_code(), ev)] = std::make_unique<Transition<FROM>>(func);
	}
	template<typename B>
	B* getState() const { return stateHolder_->template getState<B>(); }
protected:
	StateHolderBase* stateHolder_;
private:
	Transitions transitions_;
};
#endif
