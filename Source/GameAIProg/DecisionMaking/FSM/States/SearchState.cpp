#include "SearchState.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Pawn.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

GameAI::FSM::SearchState::~SearchState() = default;

void GameAI::FSM::SearchState::Enter(AAIController& Controller)
{
	bReachedLastKnownLocation = false;

	if (!SeekBehavior)
	{
		SeekBehavior = std::make_unique<Seek>();
	}

	if (!WanderBehavior)
	{
		WanderBehavior = std::make_unique<Wander>();
		WanderBehavior->SetWanderRadius(90.f);
		WanderBehavior->SetWanderOffset(120.f);
	}

	if (ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn()))
	{
		Agent->SetSteeringBehavior(SeekBehavior.get());
	}

	UBlackboardComponent* Blackboard = Controller.GetBlackboardComponent();
	if (Blackboard)
	{
		Blackboard->SetValueAsFloat(TEXT("SearchStartTime"), Controller.GetWorld()->GetTimeSeconds());
		Blackboard->SetValueAsBool(TEXT("SearchReachedLastKnownLocation"), false);
		const FVector LastKnown = Blackboard->GetValueAsVector(TEXT("LastKnownTargetLocation"));
		SeekBehavior->SetTarget(FTargetData{FVector2D{LastKnown}});
	}

	UE_LOG(LogTemp, Log, TEXT("FSM: Enter SearchState"));
}

void GameAI::FSM::SearchState::Exit(AAIController& Controller)
{
	if (ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn()))
	{
		Agent->SetSteeringBehavior(nullptr);
	}

	UE_LOG(LogTemp, Log, TEXT("FSM: Exit SearchState"));
}

void GameAI::FSM::SearchState::Update(AAIController& Controller, float DeltaTime)
{
	ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn());
	if (!Agent || !SeekBehavior || !WanderBehavior)
	{
		return;
	}

	if (!bReachedLastKnownLocation)
	{
		UBlackboardComponent* Blackboard = Controller.GetBlackboardComponent();
		const FVector LastKnown = Blackboard ? Blackboard->GetValueAsVector(TEXT("LastKnownTargetLocation")) : FVector::ZeroVector;
		const FVector2D ToLastKnown = FVector2D{LastKnown} - Agent->GetPosition();
		const float ReachDistance = FMath::Max(Agent->GetCapsuleRadius() * 1.5f, 100.f);
		if (ToLastKnown.SizeSquared() <= ReachDistance * ReachDistance)
		{
			bReachedLastKnownLocation = true;
			if (Blackboard)
			{
				Blackboard->SetValueAsBool(TEXT("SearchReachedLastKnownLocation"), true);
			}
			Agent->SetSteeringBehavior(WanderBehavior.get());
		}
	}
}
