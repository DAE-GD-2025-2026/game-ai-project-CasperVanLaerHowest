// Fill out your copyright notice in the Description page of Project Settings.


#include "Level_FSM.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "DrawDebugHelpers.h"
#include "FSMComponent.h"
#include "DecisionMaking/GameAIController.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "Misc/CString.h"
#include "imgui.h"
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

	const FVector SpawnCenter = GetNavMeshBoundsCenter(90.0f).value_or(FVector{0, 0, 90});
	
	GuardAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, 
	SpawnCenter, FRotator::ZeroRotator);
	if (!IsValid(GuardAgent))
	{
		UE_LOG(LogTemp, Error, TEXT("FSM: Failed to spawn GuardAgent"));
		return;
	}
	GuardAgent->SetDebugRenderingEnabled(true);
	GuardAgent->AIControllerClass = AGameAIController::StaticClass();
	GuardAgent->SpawnDefaultController();

	ThiefAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass,
	SpawnCenter + FVector{650,0,0}, FRotator::ZeroRotator);
	if (!IsValid(ThiefAgent))
	{
		UE_LOG(LogTemp, Error, TEXT("FSM: Failed to spawn ThiefAgent"));
		return;
	}
	ThiefAgent->SetDebugRenderingEnabled(true);
	ThiefAgent->SpawnDefaultController();
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

			UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
			UE_LOG(LogTemp, Warning, TEXT("FSM: BlackboardComp valid: %s"), (BlackboardComp != nullptr ? TEXT("true") : TEXT("false")));
			UE_LOG(LogTemp, Warning, TEXT("FSM: ThiefAgent valid: %s"), (IsValid(ThiefAgent) ? TEXT("true") : TEXT("false")));

			if (BlackboardComp)
			{
				BlackboardComp->SetValueAsObject(TEXT("TargetActor"), ThiefAgent);
				BlackboardComp->SetValueAsVector(TEXT("LastKnownTargetLocation"), ThiefAgent->GetActorLocation());
			}

			auto GetTargetInfo = [AIController]() -> TOptional<TPair<float, bool>>
			{
				if (!AIController)
				{
					UE_LOG(LogTemp, Warning, TEXT("TargetInfo: AIController is invalid."));
					return {};
				}
				if (!AIController->GetPawn())
				{
					UE_LOG(LogTemp, Warning, TEXT("TargetInfo: AIController->GetPawn() is invalid."));
					return {};
				}
				if (!AIController->GetWorld())
				{
					UE_LOG(LogTemp, Warning, TEXT("TargetInfo: AIController->GetWorld() is invalid."));
					return {};
				}

				UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
				AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
				if (!IsValid(TargetActor))
				{
					UE_LOG(LogTemp, Warning, TEXT("TargetInfo: TargetActor from Blackboard is invalid."));
					return {};
				}

				const float Dist = FVector::Distance(AIController->GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
				const bool bHasLOS = AIController->LineOfSightTo(TargetActor);

				return TPair<float, bool>{Dist, bHasLOS};
			};

			auto StoreTargetLocation = [AIController]()
			{
				if (!AIController)
				{
					return;
				}

				UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
				AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
				if (Blackboard && IsValid(TargetActor))
				{
					Blackboard->SetValueAsVector(TEXT("LastKnownTargetLocation"), TargetActor->GetActorLocation());
				}
			};

			FSM->AddTransition(PatrolState, ChaseState, [GetTargetInfo, StoreTargetLocation, this]()
			{
				const TOptional<TPair<float, bool>> Info = GetTargetInfo();
				UE_LOG(LogTemp, Warning, TEXT("Patrol to Chase Transition: Info.IsSet()=%s, Distance=%f, DetectionRadius=%f"),
					Info.IsSet() ? TEXT("true") : TEXT("false"),
					Info.IsSet() ? Info->Key : -1.0f,
					GuardDetectionRadius);
				const bool bCanChase = Info.IsSet() && Info->Key <= GuardDetectionRadius;
				if (bCanChase)
				{
					StoreTargetLocation();
				}
				return bCanChase;
			});

			FSM->AddTransition(SearchState, ChaseState, [GetTargetInfo, StoreTargetLocation, this]()
			{
				const TOptional<TPair<float, bool>> Info = GetTargetInfo();
				const bool bCanChase = Info.IsSet() && Info->Key <= GuardDetectionRadius;
				if (bCanChase)
				{
					StoreTargetLocation();
				}
				return bCanChase;
			});

			FSM->AddTransition(ChaseState, SearchState, [GetTargetInfo, StoreTargetLocation, this]()
			{
				const TOptional<TPair<float, bool>> Info = GetTargetInfo();
				const bool bShouldSearch = !Info.IsSet() || Info->Key > GuardDetectionRadius;
				if (bShouldSearch)
				{
					StoreTargetLocation();
				}
				return bShouldSearch;
			});

			FSM->AddTransition(SearchState, PatrolState, [AIController]()
			{
				if (!AIController)
				{
					return true;
				}

				UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
				ASteeringAgent* Agent = Cast<ASteeringAgent>(AIController->GetPawn());
				if (!Blackboard || !IsValid(Agent))
				{
					return true;
				}

				const FVector LastKnown = Blackboard->GetValueAsVector(TEXT("LastKnownTargetLocation"));
				const FVector2D ToLastKnown = FVector2D{LastKnown} - Agent->GetPosition();
				const float ReachDistance = FMath::Max(Agent->GetCapsuleRadius() * 1.5f, 100.f);
				return ToLastKnown.SizeSquared() <= ReachDistance * ReachDistance;
			});

			AIController->RunFiniteStateMachine();
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("FSM: GuardAgent controller is not AGameAIController. Check AIControllerClass on SteeringAgent BP/class."));
	}
	
}

// Called every frame
void ALevel_FSM::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PlayerController)
	{
		const bool bLeftMouseDown = PlayerController->IsInputKeyDown(EKeys::LeftMouseButton);
		if (bLeftMouseDown && !bWasLeftMouseDown && !ImGui::GetIO().WantCaptureMouse)
		{
			SetThiefTargetFromMouse();
		}
		bWasLeftMouseDown = bLeftMouseDown;
	}

	if (IsValid(ThiefAgent))
	{
		const FVector ThiefTarget3D{MouseTarget.Position.X, MouseTarget.Position.Y, ThiefAgent->GetActorLocation().Z};
		DrawDebugLine(GetWorld(), ThiefAgent->GetActorLocation(), ThiefTarget3D, FColor::Cyan, false, -1.f, 0, 2.f);
		DrawDebugPoint(GetWorld(), ThiefTarget3D, 12.f, FColor::Cyan, false, -1.f, 0);
	}

	if (IsValid(GuardAgent) && IsValid(ThiefAgent))
	{
		const float DistanceToThief = FVector::Distance(GuardAgent->GetActorLocation(), ThiefAgent->GetActorLocation());
		const bool bHasLOS = GuardAgent->GetController()
			? GuardAgent->GetController()->LineOfSightTo(ThiefAgent)
			: false;
		const bool bWithinDetectionRadius = DistanceToThief <= GuardDetectionRadius;
		const FColor DetectionColor = bWithinDetectionRadius
			? FColor::Green
			: FColor::Red;

		DrawDebugCircle(
			GetWorld(),
			GuardAgent->GetActorLocation(),
			GuardDetectionRadius,
			48,
			DetectionColor,
			false,
			-1.f,
			0,
			3.f,
			FVector(1, 0, 0),
			FVector(0, 1, 0),
			false);

		DrawDebugLine(GetWorld(), GuardAgent->GetActorLocation(), ThiefAgent->GetActorLocation(), FColor::Yellow, false, -1.f, 0, 1.f);
	}

	if (AGameAIController* GuardController = GuardAgent ? Cast<AGameAIController>(GuardAgent->GetController()) : nullptr)
	{
		const UFSMComponent* GuardFSM = Cast<UFSMComponent>(GuardController->GetBrainComponent());
		const GameAI::FSM::State* GuardState = GuardFSM ? GuardFSM->GetCurrentState() : nullptr;
		UBlackboardComponent* Blackboard = GuardController->GetBlackboardComponent();
		if (GuardState && Blackboard && FCStringAnsi::Strcmp(GuardState->GetDebugName(), "Search") == 0)
		{
			const FVector LastKnown = Blackboard->GetValueAsVector(TEXT("LastKnownTargetLocation"));
			const float DrawZ = IsValid(GuardAgent) ? GuardAgent->GetActorLocation().Z : LastKnown.Z;
			const FVector LastKnownDrawLocation{LastKnown.X, LastKnown.Y, DrawZ + 10.f};

			DrawDebugSphere(GetWorld(), LastKnownDrawLocation, 80.f, 16, FColor::Orange, false, -1.f, 0, 3.f);
			DrawDebugPoint(GetWorld(), LastKnownDrawLocation, 18.f, FColor::Orange, false, -1.f, 0);
			if (IsValid(GuardAgent))
			{
				DrawDebugLine(GetWorld(), GuardAgent->GetActorLocation(), LastKnownDrawLocation, FColor::Orange, false, -1.f, 0, 2.f);
			}
		}
	}

	UpdateImGui();
}

void ALevel_FSM::BindLevelInputActions()
{
	Super::BindLevelInputActions();
	if (!PlayerEnhancedInputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("FSM: Enhanced input component missing, using direct LMB fallback."));
		return;
	}

	if (SetThiefTargetAction)
	{
		PlayerEnhancedInputComponent->BindAction(SetThiefTargetAction, ETriggerEvent::Started, this,
			&ALevel_FSM::SetThiefTargetFromMouse);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("FSM: SetThiefTargetAction not assigned, using direct LMB fallback."));
	}
}

void ALevel_FSM::SetThiefTargetFromMouse()
{
	if (!ThiefSeek || !IsValid(ThiefAgent))
	{
		return;
	}

	if (const auto MouseWorldPos = GetMouseWorldPos(); MouseWorldPos.has_value())
	{
		LatestMouseWorldPos = MouseWorldPos.value();
	}

	MouseTarget.Position = FVector2D{LatestMouseWorldPos};
	ThiefSeek->SetTarget(MouseTarget);
}

void ALevel_FSM::UpdateImGui()
{
	ImGui::SetNextWindowPos(WindowPos);
	ImGui::SetNextWindowSize(WindowSize);
	ImGui::Begin("Gameplay Programming", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	ImGui::Text("CONTROLS");
	ImGui::Indent();
	ImGui::Text("LMB: set thief target");
	ImGui::Unindent();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("STATS");
	ImGui::Indent();
	ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
	ImGui::Unindent();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("FSM Debug");
	ImGui::Indent();
	ImGui::Text("Guard valid: %s", IsValid(GuardAgent) ? "Yes" : "No");
	ImGui::Text("Thief valid: %s", IsValid(ThiefAgent) ? "Yes" : "No");
	ImGui::Text("Guard controller present: %s", GuardAgent && GuardAgent->GetController() ? "Yes" : "No");
	ImGui::Text("Thief controller present: %s", ThiefAgent && ThiefAgent->GetController() ? "Yes" : "No");
	const AGameAIController* GuardController = GuardAgent ? Cast<AGameAIController>(GuardAgent->GetController()) : nullptr;
	const UFSMComponent* GuardFSM = GuardController ? Cast<UFSMComponent>(GuardController->GetBrainComponent()) : nullptr;
	const GameAI::FSM::State* GuardState = GuardFSM ? GuardFSM->GetCurrentState() : nullptr;
	ImGui::Text("Guard state: %s", GuardState ? GuardState->GetDebugName() : "None");
	ImGui::Text("Mouse target: (%.1f, %.1f)", MouseTarget.Position.X, MouseTarget.Position.Y);
	ImGui::SliderFloat("Guard detect radius", &GuardDetectionRadius, 100.f, 4000.f, "%.1f");

	if (IsValid(GuardAgent) && IsValid(ThiefAgent))
	{
		float GuardMaxSpeed = GuardAgent->GetMaxLinearSpeed();
		if (ImGui::SliderFloat("Guard max speed", &GuardMaxSpeed, 50.f, 2000.f, "%.1f"))
		{
			GuardAgent->SetMaxLinearSpeed(GuardMaxSpeed);
		}

		float ThiefMaxSpeed = ThiefAgent->GetMaxLinearSpeed();
		if (ImGui::SliderFloat("Thief max speed", &ThiefMaxSpeed, 50.f, 2000.f, "%.1f"))
		{
			ThiefAgent->SetMaxLinearSpeed(ThiefMaxSpeed);
		}

		const float Distance = FVector::Distance(GuardAgent->GetActorLocation(), ThiefAgent->GetActorLocation());
		const bool bHasLOS = GuardAgent->GetController()
			? GuardAgent->GetController()->LineOfSightTo(ThiefAgent)
			: false;
		const float ThiefToTargetDistance = FVector2D::Distance(ThiefAgent->GetPosition(), MouseTarget.Position);
		const bool bWithinDetectionRadius = Distance <= GuardDetectionRadius;
		ImGui::Text("Guard->Thief dist: %.1f", Distance);
		ImGui::Text("Within detect radius: %s", bWithinDetectionRadius ? "Yes" : "No");
		ImGui::Text("Guard LOS Thief: %s", bHasLOS ? "Yes" : "No");
		ImGui::Text("Thief->Target dist: %.1f", ThiefToTargetDistance);
		ImGui::Text("Guard speed: %.1f", GuardAgent->GetVelocity().Size2D());
		ImGui::Text("Thief speed: %.1f", ThiefAgent->GetVelocity().Size2D());
	}
	ImGui::Unindent();

	ImGui::End();
}
