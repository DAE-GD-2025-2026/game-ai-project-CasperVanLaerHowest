// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_FSM.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "FSMComponent.h"
#include "DecisionMaking/GameAIController.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "States/ChaseState.h"
#include "States/PatrolState.h"
#include "States/SearchState.h"


// Sets default values
ALevel_FSM::ALevel_FSM()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_FSM::BeginPlay()
{
	Super::BeginPlay();
	
	GuardAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, 
	FVector{0,0,90}, FRotator::ZeroRotator);
	GuardAgent->SetDebugRenderingEnabled(false);

	ThiefAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass,
	FVector{650,0,90}, FRotator::ZeroRotator);
	ThiefAgent->SetDebugRenderingEnabled(false);
	ThiefSeek = std::make_unique<Seek>();
	ThiefAgent->SetSteeringBehavior(ThiefSeek.get());
	MouseTarget.Position = FVector2D{ThiefAgent->GetActorLocation()};
	ThiefSeek->SetTarget(MouseTarget);
	
	 
	if (AGameAIController* AIController = Cast<AGameAIController>(GuardAgent->GetController()))
	{
		if (UFSMComponent* FSM = Cast<UFSMComponent>(AIController->GetBrainComponent()))
		{
			GameAI::FSM::State* PatrolState = FSM->AddState(std::make_unique<GameAI::FSM::PatrolState>());
			GameAI::FSM::State* SearchState = FSM->AddState(std::make_unique<GameAI::FSM::SearchState>());
			GameAI::FSM::State* ChaseState = FSM->AddState(std::make_unique<GameAI::FSM::ChaseState>());

			constexpr float DetectionRadius = 1400.f;
			constexpr float MaxSearchDuration = 8.0f;

			UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
			if (BlackboardComp)
			{
				BlackboardComp->SetValueAsObject(TEXT("TargetActor"), ThiefAgent);
				BlackboardComp->SetValueAsVector(TEXT("LastKnownTargetLocation"), ThiefAgent->GetActorLocation());
			}

			auto TargetInfo = [AIController]() -> TOptional<TPair<float, bool>>
			{
				if (!AIController || !AIController->GetPawn() || !AIController->GetWorld())
				{
					return {};
				}

				UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
				AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
				if (!TargetActor)
				{
					return {};
				}

				const float Dist = FVector::Distance(AIController->GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
				const bool bHasLOS = AIController->LineOfSightTo(TargetActor);
				if (bHasLOS && Blackboard)
				{
					Blackboard->SetValueAsVector(TEXT("LastKnownTargetLocation"), TargetActor->GetActorLocation());
				}

				return TPair<float, bool>{Dist, bHasLOS};
			};

			FSM->AddTransition(PatrolState, ChaseState, [TargetInfo, DetectionRadius]()
			{
				const TOptional<TPair<float, bool>> Info = TargetInfo();
				return Info.IsSet() && Info->Value && Info->Key <= DetectionRadius;
			});

			FSM->AddTransition(SearchState, ChaseState, [TargetInfo, DetectionRadius]()
			{
				const TOptional<TPair<float, bool>> Info = TargetInfo();
				return Info.IsSet() && Info->Value && Info->Key <= DetectionRadius;
			});

			FSM->AddTransition(ChaseState, SearchState, [TargetInfo, DetectionRadius]()
			{
				const TOptional<TPair<float, bool>> Info = TargetInfo();
				return !Info.IsSet() || !Info->Value || Info->Key > DetectionRadius;
			});

			FSM->AddTransition(SearchState, PatrolState, [AIController, MaxSearchDuration]()
			{
				if (!AIController)
				{
					return true;
				}

				UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
				if (!Blackboard || !AIController->GetWorld())
				{
					return true;
				}

				const float SearchStartTime = Blackboard->GetValueAsFloat(TEXT("SearchStartTime"));
				const float Elapsed = AIController->GetWorld()->GetTimeSeconds() - SearchStartTime;
				return Elapsed >= MaxSearchDuration;
			});

			AIController->RunFiniteStateMachine();
		}
	}
	
}

// Called every frame
void ALevel_FSM::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void ALevel_FSM::BindLevelInputActions()
{
	Super::BindLevelInputActions();
	if (PlayerEnhancedInputComponent && SetThiefTargetAction)
	{
		PlayerEnhancedInputComponent->BindAction(SetThiefTargetAction, ETriggerEvent::Triggered, this,
			&ALevel_FSM::SetThiefTargetFromMouse);
	}
}

void ALevel_FSM::SetThiefTargetFromMouse()
{
	if (!ThiefSeek)
	{
		return;
	}

	MouseTarget.Position = FVector2D{LatestMouseWorldPos};
	ThiefSeek->SetTarget(MouseTarget);
}

