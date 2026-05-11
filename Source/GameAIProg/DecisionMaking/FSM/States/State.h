#pragma once

class AAIController;

namespace GameAI::FSM
{
	class State
	{
	public:
		virtual ~State() = default;

		virtual const char* GetDebugName() const { return "Unknown"; }
		virtual void Enter(AAIController& Controller) {}
		virtual void Update(AAIController& Controller, float DeltaTime) = 0;
		virtual void Exit(AAIController& Controller) {}
	};
}
