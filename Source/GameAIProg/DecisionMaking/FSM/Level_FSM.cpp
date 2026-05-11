#include "Level_FSM.h"

#include <memory>

#include "BehaviorTree/BlackboardComponent.h"
#include "DecisionMaking/BehaviorTree/BT.h"
#include "DecisionMaking/BehaviorTree/GameAIBehaviorTreeComponent.h"
#include "DecisionMaking/GameAIController.h"
#include "DrawDebugHelpers.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "InputCoreTypes.h"
#include "imgui.h"

ALevel_FSM::ALevel_FSM()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_FSM::BeginPlay()
{
	Super::BeginPlay();

	const FVector SpawnCenter = GetNavMeshBoundsCenter(90.0f).value_or(FVector{0, 0, 90});

	GuardAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, SpawnCenter, FRotator::ZeroRotator);
	if (!IsValid(GuardAgent))
	{
		UE_LOG(LogTemp, Error, TEXT("BehaviorTree: Failed to spawn GuardAgent"));
		return;
	}
	GuardAgent->SetDebugRenderingEnabled(true);
	GuardAgent->AIControllerClass = AGameAIController::StaticClass();
	GuardAgent->SpawnDefaultController();

	ThiefAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass,
		SpawnCenter + FVector{650, 0, 0}, FRotator::ZeroRotator);
	if (!IsValid(ThiefAgent))
	{
		UE_LOG(LogTemp, Error, TEXT("BehaviorTree: Failed to spawn ThiefAgent"));
		return;
	}
	ThiefAgent->SetDebugRenderingEnabled(true);
	ThiefAgent->SpawnDefaultController();
	ThiefSeek = std::make_unique<Seek>();
	ThiefAgent->SetSteeringBehavior(ThiefSeek.get());
	MouseTarget.Position = FVector2D{ThiefAgent->GetActorLocation()};
	ThiefSeek->SetTarget(MouseTarget);

	AGameAIController* AIController = Cast<AGameAIController>(GuardAgent->GetController());
	if (!AIController)
	{
		UE_LOG(LogTemp, Error, TEXT("BehaviorTree: GuardAgent controller is not AGameAIController."));
		return;
	}

	UGameAIBehaviorTreeComponent* BehaviorTreeComp = AIController->FindComponentByClass<UGameAIBehaviorTreeComponent>();
	UBlackboardComponent* BlackboardComp = AIController->GetBlackboardComponent();
	if (!BehaviorTreeComp || !BlackboardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("BehaviorTree: Guard controller is missing a behavior tree component or blackboard."));
		return;
	}

	BlackboardComp->SetValueAsObject(TEXT("TargetActor"), ThiefAgent);
	BlackboardComp->SetValueAsVector(TEXT("LastKnownTargetLocation"), ThiefAgent->GetActorLocation());
	BlackboardComp->SetValueAsBool(TEXT("HasLastKnownTarget"), false);
	BlackboardComp->SetValueAsBool(TEXT("SearchReachedLastKnownLocation"), false);

	auto GetTargetInfo = [AIController]() -> TOptional<TPair<float, bool>>
	{
		if (!AIController || !AIController->GetPawn() || !AIController->GetWorld())
		{
			return {};
		}

		UBlackboardComponent* Blackboard = AIController->GetBlackboardComponent();
		AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
		if (!IsValid(TargetActor))
		{
			return {};
		}

		const float Distance = FVector::Distance(AIController->GetPawn()->GetActorLocation(), TargetActor->GetActorLocation());
		const bool bHasLOS = AIController->LineOfSightTo(TargetActor);
		return TPair<float, bool>{Distance, bHasLOS};
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
			Blackboard->SetValueAsBool(TEXT("HasLastKnownTarget"), true);
			Blackboard->SetValueAsBool(TEXT("SearchReachedLastKnownLocation"), false);
		}
	};

	auto ChaseSeek = std::make_shared<Seek>();
	auto SearchSeek = std::make_shared<Seek>();
	auto SearchWander = std::make_shared<Wander>();
	SearchWander->SetWanderRadius(90.f);
	SearchWander->SetWanderOffset(120.f);
	auto PatrolSeek = std::make_shared<Seek>();
	auto PatrolPoints = std::make_shared<TArray<FVector2D>>();
	auto PatrolPointIndex = std::make_shared<int32>(0);
	auto bReachedSearchPoint = std::make_shared<bool>(false);

	auto Tree = std::make_unique<GameAI::BT::BehaviorTree>();
	auto Root = std::make_unique<GameAI::BT::Selector>("Root Selector");

	auto ChaseSequence = std::make_unique<GameAI::BT::Sequence>("Can See -> Chase");
	ChaseSequence->AddChild(std::make_unique<GameAI::BT::Action>("Can See Thief",
		[GetTargetInfo, StoreTargetLocation, this](AAIController&, float)
		{
			const TOptional<TPair<float, bool>> Info = GetTargetInfo();
			const bool bCanSeeTarget = Info.IsSet() && Info->Key <= GuardDetectionRadius && Info->Value;
			if (!bCanSeeTarget)
			{
				return GameAI::BT::ENodeResult::Failed;
			}

			StoreTargetLocation();
			return GameAI::BT::ENodeResult::Succeeded;
		}));
	ChaseSequence->AddChild(std::make_unique<GameAI::BT::SimpleParallel>(
		std::make_unique<GameAI::BT::Action>("Chase Thief",
			[ChaseSeek, BehaviorTreeComp](AAIController& Controller, float)
			{
				UBlackboardComponent* Blackboard = Controller.GetBlackboardComponent();
				AActor* TargetActor = Blackboard ? Cast<AActor>(Blackboard->GetValueAsObject(TEXT("TargetActor"))) : nullptr;
				ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn());
				if (!IsValid(TargetActor) || !IsValid(Agent))
				{
					return GameAI::BT::ENodeResult::Failed;
				}

				ChaseSeek->SetTarget(FTargetData{FVector2D{TargetActor->GetActorLocation()}});
				Agent->SetSteeringBehavior(ChaseSeek.get());
				BehaviorTreeComp->SetLastRunningNode(nullptr);
				return GameAI::BT::ENodeResult::Running;
			}),
		std::make_unique<GameAI::BT::Action>("Update Last Seen",
			[StoreTargetLocation](AAIController&, float)
			{
				StoreTargetLocation();
				return GameAI::BT::ENodeResult::Running;
			}),
		false,
		"Chase + Update Memory"));
	Root->AddChild(std::move(ChaseSequence));

	auto SearchSequence = std::make_unique<GameAI::BT::Sequence>("Has Memory -> Search");
	SearchSequence->AddChild(std::make_unique<GameAI::BT::Action>("Has Last Seen Location",
		[](AAIController& Controller, float)
		{
			UBlackboardComponent* Blackboard = Controller.GetBlackboardComponent();
			return Blackboard && Blackboard->GetValueAsBool(TEXT("HasLastKnownTarget"))
				? GameAI::BT::ENodeResult::Succeeded
				: GameAI::BT::ENodeResult::Failed;
		}));
	SearchSequence->AddChild(std::make_unique<GameAI::BT::Action>("Search Last Seen Location",
		[SearchSeek, SearchWander, bReachedSearchPoint, BehaviorTreeComp](AAIController& Controller, float)
		{
			ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn());
			UBlackboardComponent* Blackboard = Controller.GetBlackboardComponent();
			if (!IsValid(Agent) || !Blackboard)
			{
				return GameAI::BT::ENodeResult::Failed;
			}

			if (!Blackboard->GetValueAsBool(TEXT("SearchReachedLastKnownLocation")))
			{
				*bReachedSearchPoint = false;
			}

			const FVector LastKnown = Blackboard->GetValueAsVector(TEXT("LastKnownTargetLocation"));
			const FVector2D ToLastKnown = FVector2D{LastKnown} - Agent->GetPosition();
			const float ReachDistance = FMath::Max(Agent->GetCapsuleRadius() * 1.5f, 100.f);
			if (!*bReachedSearchPoint)
			{
				SearchSeek->SetTarget(FTargetData{FVector2D{LastKnown}});
				Agent->SetSteeringBehavior(SearchSeek.get());

				if (ToLastKnown.SizeSquared() <= ReachDistance * ReachDistance)
				{
					*bReachedSearchPoint = true;
					Blackboard->SetValueAsBool(TEXT("SearchReachedLastKnownLocation"), true);
					Blackboard->SetValueAsFloat(TEXT("SearchStartTime"), Controller.GetWorld()->GetTimeSeconds());
					Agent->SetSteeringBehavior(SearchWander.get());
				}

				BehaviorTreeComp->SetLastRunningNode(nullptr);
				return GameAI::BT::ENodeResult::Running;
			}

			Agent->SetSteeringBehavior(SearchWander.get());
			const float SearchStartTime = Blackboard->GetValueAsFloat(TEXT("SearchStartTime"));
			if (Controller.GetWorld() && Controller.GetWorld()->GetTimeSeconds() - SearchStartTime >= 2.5f)
			{
				Blackboard->SetValueAsBool(TEXT("HasLastKnownTarget"), false);
				Blackboard->SetValueAsBool(TEXT("SearchReachedLastKnownLocation"), false);
				*bReachedSearchPoint = false;
				return GameAI::BT::ENodeResult::Failed;
			}

			BehaviorTreeComp->SetLastRunningNode(nullptr);
			return GameAI::BT::ENodeResult::Running;
		},
		[bReachedSearchPoint](AAIController&)
		{
			*bReachedSearchPoint = false;
		}));
	Root->AddChild(std::move(SearchSequence));

	Root->AddChild(std::make_unique<GameAI::BT::Action>("Patrol",
		[PatrolSeek, PatrolPoints, PatrolPointIndex, BehaviorTreeComp](AAIController& Controller, float)
		{
			ASteeringAgent* Agent = Cast<ASteeringAgent>(Controller.GetPawn());
			if (!IsValid(Agent))
			{
				return GameAI::BT::ENodeResult::Failed;
			}

			if (PatrolPoints->IsEmpty())
			{
				const FVector2D Center = Agent->GetPosition();
				PatrolPoints->Add(Center + FVector2D{-700.f, -700.f});
				PatrolPoints->Add(Center + FVector2D{700.f, -700.f});
				PatrolPoints->Add(Center + FVector2D{700.f, 700.f});
				PatrolPoints->Add(Center + FVector2D{-700.f, 700.f});
				*PatrolPointIndex = 0;
			}

			const FVector2D CurrentTarget = (*PatrolPoints)[*PatrolPointIndex];
			const FVector2D ToPoint = CurrentTarget - Agent->GetPosition();
			const float ReachDistance = FMath::Max(Agent->GetCapsuleRadius() * 1.5f, 80.f);
			if (ToPoint.SizeSquared() <= ReachDistance * ReachDistance)
			{
				*PatrolPointIndex = (*PatrolPointIndex + 1) % PatrolPoints->Num();
			}

			PatrolSeek->SetTarget(FTargetData{(*PatrolPoints)[*PatrolPointIndex]});
			Agent->SetSteeringBehavior(PatrolSeek.get());
			BehaviorTreeComp->SetLastRunningNode(nullptr);
			return GameAI::BT::ENodeResult::Running;
		}));

	Tree->SetRoot(std::move(Root));
	BehaviorTreeComp->SetTree(std::move(Tree));
	AIController->RunBehaviorTreeLogic();
}

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
		const bool bCanSeeThief = DistanceToThief <= GuardDetectionRadius && bHasLOS;
		const FColor DetectionColor = bCanSeeThief ? FColor::Green : FColor::Red;

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
		UBlackboardComponent* Blackboard = GuardController->GetBlackboardComponent();
		if (Blackboard && Blackboard->GetValueAsBool(TEXT("HasLastKnownTarget")))
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
		UE_LOG(LogTemp, Warning, TEXT("BehaviorTree: Enhanced input component missing, using direct LMB fallback."));
		return;
	}

	if (SetThiefTargetAction)
	{
		PlayerEnhancedInputComponent->BindAction(SetThiefTargetAction, ETriggerEvent::Started, this,
			&ALevel_FSM::SetThiefTargetFromMouse);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("BehaviorTree: SetThiefTargetAction not assigned, using direct LMB fallback."));
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

	ImGui::Text("Behavior Tree Debug");
	ImGui::Indent();
	ImGui::Text("Guard valid: %s", IsValid(GuardAgent) ? "Yes" : "No");
	ImGui::Text("Thief valid: %s", IsValid(ThiefAgent) ? "Yes" : "No");
	ImGui::Text("Guard controller present: %s", GuardAgent && GuardAgent->GetController() ? "Yes" : "No");
	ImGui::Text("Thief controller present: %s", ThiefAgent && ThiefAgent->GetController() ? "Yes" : "No");

	const AGameAIController* GuardController = GuardAgent ? Cast<AGameAIController>(GuardAgent->GetController()) : nullptr;
	const UBlackboardComponent* Blackboard = GuardController ? GuardController->GetBlackboardComponent() : nullptr;
	const bool bHasMemory = Blackboard && Blackboard->GetValueAsBool(TEXT("HasLastKnownTarget"));
	const bool bReachedSearchPoint = Blackboard && Blackboard->GetValueAsBool(TEXT("SearchReachedLastKnownLocation"));
	ImGui::Text("Root: Selector");
	ImGui::Text("Priority: Chase > Search > Patrol");
	ImGui::Text("Has last seen location: %s", bHasMemory ? "Yes" : "No");
	ImGui::Text("Reached search point: %s", bReachedSearchPoint ? "Yes" : "No");
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
