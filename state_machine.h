#ifndef ELEGANT_STATE_MACHINE_H_
#define ELEGANT_STATE_MACHINE_H_
#include <map>
#include <typeinfo>
#include <memory>

template<typename M>
class Hierarchical : public M {
	struct PolicyBase { virtual bool call(typename M::StateBase*, M* machine, typename M::EventType) = 0;};
	template<typename S>
	using PolicyFunc = std::function<bool(S* state, M* machine, typename M::EventType ev)>;
	template<typename S>
	struct Policy : PolicyBase {
		Policy(PolicyFunc<S> policy) : policyFunc_(policy) {}
		bool call(typename M::StateBase* state, M* machine, typename M::EventType ev) override {
			return policyFunc_(dynamic_cast<typename M::template State<S>*>(state), machine, ev);
		}
		PolicyFunc<S> policyFunc_;
	};
	using Policies = std::map<size_t, std::unique_ptr<PolicyBase>>;
	Policies policies_;
public:
	template<typename S>
	Hierarchical(S* initState) : M(initState) {}
	bool inject(typename M::EventType ev) {
		typename Policies::const_iterator ittr = policies_.find(M::state_->getHash());
		if (ittr == policies_.end())
			return M::inject(ev);
		return ittr->second->call(M::state_, this, ev);
	}
	template<typename S>
	void addPolicy(PolicyFunc<S> func) {
		policies_[typeid(S).hash_code()] = std::make_unique<Policy<S>>(func);
	}
};

template<typename E>
class StateMachine {
protected:
	struct StateBase {
		virtual ~StateBase(){}
		virtual size_t getHash() const = 0;
		template<typename B>
		B* getState() override { return dynamic_cast<B*>(this); }
	};
	StateBase* state_;
public:
	using EventType = E;
	template<typename S>
	struct State : public StateBase, public S {
		using S::S;
		virtual size_t getHash() const { return typeid(S).hash_code(); }
	};
private:
	struct TransitionBase { virtual ~TransitionBase(){} virtual StateBase* call(StateBase*, E) = 0; };
	template<typename FROM>
	using Func = std::function<StateBase*(std::unique_ptr<State<FROM>>, E)>;
	template<typename FROM>
	struct Transition : public TransitionBase {
		Transition(Func<FROM> func) : func_(func) {}
		StateBase* call(StateBase* from, E ev) override {
			return func_(std::unique_ptr<State<FROM>>(dynamic_cast<State<FROM>*>(from)), ev);
		};
		Func<FROM> func_;
	};
	using Transitions = std::map<std::pair<size_t, E>, std::unique_ptr<TransitionBase>>;
	Transitions transitions_;
public:
	template<typename S>
	StateMachine(S* initState) : state_(initState) {}
	virtual ~StateMachine() { delete state_; }
	bool inject(E ev) {
		typename Transitions::const_iterator ittr = transitions_.find(std::make_pair(state_->getHash(), ev));
		if (ittr == transitions_.end())
			return false;
		if (StateBase* newState = ittr->second->call(state_, ev))
			return state_ = newState;
		return false;
	}
	template<typename FROM>
	void addEvent(E ev, Func<FROM> func) {
		transitions_[std::make_pair(typeid(FROM).hash_code(), ev)] = std::make_unique<Transition<FROM>>(func);
	}
	template<typename B>
	B* getState() const { return state_->template getState<B>(); }
};
#endif
