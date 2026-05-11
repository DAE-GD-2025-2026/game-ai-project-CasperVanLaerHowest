#include "FSM.h"

#include "AIController.h"
#include "States/State.h"

bool GameAI::FSM::Transition::CanTrigger() const
{
	return EvalFunc && EvalFunc();
}

GameAI::FSM::State* GameAI::FSM::FSM::AddState(std::unique_ptr<State>&& NewState)
{
	if (!NewState)
	{
		return nullptr;
	}

	State* RawState = NewState.get();
	States.emplace_back(std::move(NewState));
	if (!CurrentState)
	{
		CurrentState = RawState;
	}

	return RawState;
}

void GameAI::FSM::FSM::AddTransition(State* From, State* To, std::function<bool()> EvalFunc)
{
	if (!From || !To || !EvalFunc)
	{
		return;
	}

	StateTransitions[From].push_back(Transition{To, std::move(EvalFunc)});
}

void GameAI::FSM::FSM::Start(AAIController& Controller)
{
	if (bHasStarted || !CurrentState)
	{
		return;
	}

	CurrentState->Enter(Controller);
	bHasStarted = true;
}

void GameAI::FSM::FSM::Stop(AAIController& Controller)
{
	if (!bHasStarted || !CurrentState)
	{
		return;
	}

	CurrentState->Exit(Controller);
	bHasStarted = false;
}

void GameAI::FSM::FSM::Update(AAIController& Controller, float DeltaTime)
{
	if (!bHasStarted || !CurrentState)
	{
		return;
	}

	if (const auto TransitionIt = StateTransitions.find(CurrentState); TransitionIt != StateTransitions.end())
	{
		for (const Transition& Transition : TransitionIt->second)
		{
			if (!Transition.CanTrigger())
			{
				continue;
			}

			CurrentState->Exit(Controller);
			CurrentState = Transition.To;
			CurrentState->Enter(Controller);
			break;
		}
	}

	CurrentState->Update(Controller, DeltaTime);
}
