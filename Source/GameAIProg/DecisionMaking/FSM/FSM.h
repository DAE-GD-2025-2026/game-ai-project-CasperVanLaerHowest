#pragma once

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "States/State.h"

class AAIController;

namespace GameAI::FSM
{
	struct Transition
	{
		State* To{nullptr};
		std::function<bool()> EvalFunc{};

		bool CanTrigger() const;
	};

	class FSM
	{
	public:
		State* AddState(std::unique_ptr<State>&& NewState);
		void AddTransition(State* From, State* To, std::function<bool()> EvalFunc);

		void Start(AAIController& Controller);
		void Stop(AAIController& Controller);
		void Update(AAIController& Controller, float DeltaTime);
		const State* GetCurrentState() const { return CurrentState; }

	private:
		std::vector<std::unique_ptr<State>> States{};
		std::unordered_map<State*, std::vector<Transition>> StateTransitions{};
		State* CurrentState{nullptr};
		bool bHasStarted{false};
	};
}
