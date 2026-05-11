#pragma once

#include <memory>

#include "State.h"

class Seek;

namespace GameAI::FSM
{
	class ChaseState : public State
	{
	public:
		virtual ~ChaseState();

		virtual const char* GetDebugName() const override { return "Chase"; }
		virtual void Enter(AAIController& Controller) override;
		virtual void Exit(AAIController& Controller) override;
		virtual void Update(AAIController& Controller, float DeltaTime) override;

	private:
		std::unique_ptr<Seek> SeekBehavior{};
	};
}
