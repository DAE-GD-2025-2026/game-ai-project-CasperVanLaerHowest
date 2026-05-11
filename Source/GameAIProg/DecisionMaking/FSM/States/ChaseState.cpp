#include "ChaseState.h"

#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

GameAI::FSM::ChaseState::~ChaseState() = default;

void GameAI::FSM::ChaseState::Enter(AAIController& Controller)
{
	if (!SeekBehavior)
	{
		SeekBehavior = std::make_unique<Seek>();
	}

	if (ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn()))
	{
		Agent->SetSteeringBehavior(SeekBehavior.get());
	}

	UE_LOG(LogTemp, Log, TEXT("FSM: Enter ChaseState"));
}

void GameAI::FSM::ChaseState::Exit(AAIController& Controller)
{
	if (ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn()))
	{
		Agent->SetSteeringBehavior(nullptr);
	}

	UE_LOG(LogTemp, Log, TEXT("FSM: Exit ChaseState"));
}

void GameAI::FSM::ChaseState::Update(AAIController& Controller, float DeltaTime)
{
	AActor* TargetActor = nullptr;
	if (UBlackboardComponent* Blackboard = Controller.GetBlackboardComponent())
	{
		TargetActor = Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor")));
	}
	if (!IsValid(TargetActor))
	{
		APlayerController* PlayerController = Controller.GetWorld() ? Controller.GetWorld()->GetFirstPlayerController() : nullptr;
		TargetActor = PlayerController ? PlayerController->GetPawn() : nullptr;
	}

	ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn());
	if (!IsValid(TargetActor) || !IsValid(Agent) || !SeekBehavior)
	{
		return;
	}

	const FVector TargetLocation = TargetActor->GetActorLocation();
	SeekBehavior->SetTarget(FTargetData{FVector2D{TargetLocation.X, TargetLocation.Y}});
}
