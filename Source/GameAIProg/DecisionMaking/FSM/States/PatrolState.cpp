#include "PatrolState.h"

#include "AIController.h"
#include "GameFramework/Pawn.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

GameAI::FSM::PatrolState::~PatrolState() = default;

void GameAI::FSM::PatrolState::Enter(AAIController& Controller)
{
	if (!SeekBehavior)
	{
		SeekBehavior = std::make_unique<Seek>();
	}

	if (ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn()))
	{
		const FVector2D Center = Agent->GetPosition();
		PatrolPoints = {
			Center + FVector2D{-700.f, -700.f},
			Center + FVector2D{700.f, -700.f},
			Center + FVector2D{700.f, 700.f},
			Center + FVector2D{-700.f, 700.f}
		};
		CurrentPatrolPointIndex = 0;
		SeekBehavior->SetTarget(FTargetData{PatrolPoints[CurrentPatrolPointIndex]});
		Agent->SetSteeringBehavior(SeekBehavior.get());
	}

	UE_LOG(LogTemp, Log, TEXT("FSM: Enter PatrolState"));
}

void GameAI::FSM::PatrolState::Exit(AAIController& Controller)
{
	if (ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn()))
	{
		Agent->SetSteeringBehavior(nullptr);
	}

	UE_LOG(LogTemp, Log, TEXT("FSM: Exit PatrolState"));
}

void GameAI::FSM::PatrolState::Update(AAIController& Controller, float DeltaTime)
{
	ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn());
	if (!Agent || !SeekBehavior || PatrolPoints.empty())
	{
		return;
	}

	const FVector2D ToPoint = PatrolPoints[CurrentPatrolPointIndex] - Agent->GetPosition();
	const float ReachDistance = FMath::Max(Agent->GetCapsuleRadius() * 1.5f, 80.f);
	if (ToPoint.SizeSquared() <= ReachDistance * ReachDistance)
	{
		CurrentPatrolPointIndex = (CurrentPatrolPointIndex + 1) % static_cast<int>(PatrolPoints.size());
		SeekBehavior->SetTarget(FTargetData{PatrolPoints[CurrentPatrolPointIndex]});
	}
}
