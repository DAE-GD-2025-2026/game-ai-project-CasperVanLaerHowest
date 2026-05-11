#pragma once
#include "CoreMinimal.h"

#include <memory>
#include <vector>

#include "State.h"

class Seek;

namespace GameAI::FSM
{
	class PatrolState : public State
	{
	public:
		virtual ~PatrolState();
		
		virtual const char* GetDebugName() const override { return "Patrol"; }
		virtual void Enter(AAIController& Controller) override;
		virtual void Exit(AAIController& Controller) override;
		virtual void Update(AAIController& Controller, float DeltaTime) override;

	private:
		std::unique_ptr<Seek> SeekBehavior{};
		std::vector<FVector2D> PatrolPoints{};
		int CurrentPatrolPointIndex{0};
	};
}

