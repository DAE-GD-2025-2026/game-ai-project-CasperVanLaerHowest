#pragma once

#include <memory>

#include "State.h"

class Wander;
class Seek;

namespace GameAI::FSM
{
	class SearchState : public State
	{
	public:
		virtual ~SearchState();

		virtual const char* GetDebugName() const override { return "Search"; }
		virtual void Enter(AAIController& Controller) override;
		virtual void Exit(AAIController& Controller) override;
		virtual void Update(AAIController& Controller, float DeltaTime) override;

	private:
		std::unique_ptr<Seek> SeekBehavior{};
		std::unique_ptr<Wander> WanderBehavior{};
		bool bReachedLastKnownLocation{false};
	};
}
