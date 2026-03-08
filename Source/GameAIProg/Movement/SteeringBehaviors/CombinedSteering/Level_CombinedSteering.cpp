#include "Level_CombinedSteering.h"
#include "CombinedSteeringBehaviors.h"
#include "imgui.h"
#include "DrawDebugHelpers.h"


// Sets default values
ALevel_CombinedSteering::ALevel_CombinedSteering()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ALevel_CombinedSteering::BeginPlay()
{
	Super::BeginPlay();

	pSeekBehavior    = std::make_unique<Seek>();
	pWanderBehavior = std::make_unique<Wander>();
	pEvadeBehavior   = std::make_unique<Evade>();
	pPrioritySeekBehavior = std::make_unique<Seek>();
	pPriorityWanderBehavior = std::make_unique<Wander>();
	pPriorityEvadeBehavior = std::make_unique<Evade>();
	
	
	pBlendedSteering = std::make_unique<BlendedSteering>(std::vector<BlendedSteering::WeightedBehavior>{
		{pSeekBehavior.get(),    0.5f},
		{pWanderBehavior.get(),  0.5f}
	});
	pPrioritySteering = std::make_unique<PrioritySteering>(std::vector<ISteeringBehavior*>{
		pPriorityEvadeBehavior.get(),
		pPriorityWanderBehavior.get()
	});

	pCombinedAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{0, 0, 90}, FRotator::ZeroRotator);
	if (IsValid(pCombinedAgent))
	{
		pCombinedAgent->SetSteeringBehavior(pBlendedSteering.get());
		pCombinedAgent->SetDebugRenderingEnabled(CanDebugRender);

		// Wander should not follow the mouse target. Seed it with the agent state
		// so Wander can generate/maintain its own internal wander target.
		if (pWanderBehavior)
		{
			FTargetData WanderSeed{};
			WanderSeed.Position = pCombinedAgent->GetPosition();
			WanderSeed.Orientation = pCombinedAgent->GetRotation();
			WanderSeed.LinearVelocity = pCombinedAgent->GetLinearVelocity();
			WanderSeed.AngularVelocity = pCombinedAgent->GetAngularVelocity();
			pWanderBehavior->SetTarget(WanderSeed);
		}
	}

	pPriorityAgent = GetWorld()->SpawnActor<ASteeringAgent>(SteeringAgentClass, FVector{300, 0, 90}, FRotator::ZeroRotator);
	if (IsValid(pPriorityAgent))
	{
		pPriorityAgent->SetSteeringBehavior(pPrioritySteering.get());
		pPriorityAgent->SetDebugRenderingEnabled(CanDebugRender);

		if (pPriorityWanderBehavior)
		{
			FTargetData WanderSeed{};
			WanderSeed.Position = pPriorityAgent->GetPosition();
			WanderSeed.Orientation = pPriorityAgent->GetRotation();
			WanderSeed.LinearVelocity = pPriorityAgent->GetLinearVelocity();
			WanderSeed.AngularVelocity = pPriorityAgent->GetAngularVelocity();
			pPriorityWanderBehavior->SetTarget(WanderSeed);
		}
	}
}

void ALevel_CombinedSteering::BeginDestroy()
{
	Super::BeginDestroy();

}

// Called every frame
void ALevel_CombinedSteering::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
#pragma region UI
	//UI
	{
		//Setup
		bool windowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Game AI", &windowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	
		//Elements
		ImGui::Text("CONTROLS");
		ImGui::Indent();
		ImGui::Text("LMB: place target");
		ImGui::Text("RMB: move cam.");
		ImGui::Text("Scrollwheel: zoom cam.");
		ImGui::Unindent();
	
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();
	
		ImGui::Text("STATS");
		ImGui::Indent();
		ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
		ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
		ImGui::Unindent();
	
		ImGui::Spacing();
		ImGui::Separator();
		ImGui::Spacing();
		ImGui::Spacing();
	
		ImGui::Text("Flocking");
		ImGui::Spacing();
		ImGui::Spacing();
	
		if (ImGui::Checkbox("Debug Rendering", &CanDebugRender))
		{
			if (IsValid(pCombinedAgent))
			{
				pCombinedAgent->SetDebugRenderingEnabled(CanDebugRender);
			}
			if (IsValid(pPriorityAgent))
			{
				pPriorityAgent->SetDebugRenderingEnabled(CanDebugRender);
			}
		}
		ImGui::Checkbox("Trim World", &TrimWorld->bShouldTrimWorld);
		if (TrimWorld->bShouldTrimWorld)
		{
			ImGuiHelpers::ImGuiSliderFloatWithSetter("Trim Size",
				TrimWorld->GetTrimWorldSize(), 1000.f, 3000.f,
				[this](float InVal) { TrimWorld->SetTrimWorldSize(InVal); });
		}
		
		ImGui::Spacing();
		ImGui::Spacing();
		ImGui::Spacing();
	
		ImGui::Text("Behavior Weights");
		ImGui::Spacing();


		ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek",
		 	pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight, 0.f, 1.f,
			[this](float InVal) { pBlendedSteering->GetWeightedBehaviorsRef()[0].Weight = InVal; }, "%.2f");
		
		 ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander",
		 pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight, 0.f, 1.f,
		 [this](float InVal) { pBlendedSteering->GetWeightedBehaviorsRef()[1].Weight = InVal; }, "%.2f");
	
		//End
		ImGui::End();
	}
#pragma endregion
	// Combined Steering Update
	if (pBlendedSteering && IsValid(pCombinedAgent))
	{
		// Mouse clicks update MouseTarget in BP; only Seek should use it.
		if (pSeekBehavior)
		{
			pSeekBehavior->SetTarget(MouseTarget);
		}
		
		if (pCombinedAgent->GetDebugRenderingEnabled() && pSeekBehavior)
		{
			const FVector AgentLocation = pCombinedAgent->GetActorLocation();
	
			const auto SeekTarget2D = pSeekBehavior->GetTarget().Position;
			const FVector SeekTarget{
				SeekTarget2D.X,
				SeekTarget2D.Y,
				AgentLocation.Z
			};
	
			// Seek target debug
			DrawDebugLine(GetWorld(), AgentLocation, SeekTarget, FColor::Cyan);
			DrawDebugPoint(GetWorld(), SeekTarget, 10.f, FColor::Cyan);
	
			// Forward direction debug
			const FVector ForwardEnd = AgentLocation + (pCombinedAgent->GetActorForwardVector() * 100.f);
			DrawDebugLine(GetWorld(), AgentLocation, ForwardEnd, FColor::Yellow);
	
			// Current movement direction debug (where the agent is going)
			const auto LinearVelocity = pCombinedAgent->GetLinearVelocity();
			FVector MovementDirection{LinearVelocity.X, LinearVelocity.Y, 0.f};
			if (!MovementDirection.IsNearlyZero())
			{
				MovementDirection.Normalize();
				const FVector MovementEnd = AgentLocation + (MovementDirection * 120.f);
				DrawDebugLine(GetWorld(), AgentLocation, MovementEnd, FColor::Green);
			}
		}

		pCombinedAgent->SetSteeringBehavior(pBlendedSteering.get());
	}
	
	// Priority Steering Update (second agent)
	if (pPrioritySteering && IsValid(pPriorityAgent))
	{
		if (pPrioritySeekBehavior)
		{
			pPrioritySeekBehavior->SetTarget(MouseTarget);
		}

		if (pPriorityEvadeBehavior && IsValid(pCombinedAgent))
		{
			FTargetData CombinedTarget{};
			CombinedTarget.Position = pCombinedAgent->GetPosition();
			CombinedTarget.Orientation = pCombinedAgent->GetRotation();
			CombinedTarget.LinearVelocity = pCombinedAgent->GetLinearVelocity();
			CombinedTarget.AngularVelocity = pCombinedAgent->GetAngularVelocity();
			pPriorityEvadeBehavior->SetTargetAgent(pCombinedAgent);
			pPriorityEvadeBehavior->SetTarget(CombinedTarget);
		}

		if (pPriorityAgent->GetDebugRenderingEnabled() && pPrioritySeekBehavior)
		{
			const FVector AgentLocation = pPriorityAgent->GetActorLocation();
			const auto SeekTarget2D = pPrioritySeekBehavior->GetTarget().Position;
			const FVector SeekTarget{
				SeekTarget2D.X,
				SeekTarget2D.Y,
				AgentLocation.Z
			};

			// Priority seek target debug
			DrawDebugLine(GetWorld(), AgentLocation, SeekTarget, FColor::Blue);
			DrawDebugPoint(GetWorld(), SeekTarget, 10.f, FColor::Blue);

			// Priority forward direction debug
			const FVector ForwardEnd = AgentLocation + (pPriorityAgent->GetActorForwardVector() * 100.f);
			DrawDebugLine(GetWorld(), AgentLocation, ForwardEnd, FColor::Yellow);

			// Priority movement direction debug
			const auto LinearVelocity = pPriorityAgent->GetLinearVelocity();
			FVector MovementDirection{LinearVelocity.X, LinearVelocity.Y, 0.f};
			if (!MovementDirection.IsNearlyZero())
			{
				MovementDirection.Normalize();
				const FVector MovementEnd = AgentLocation + (MovementDirection * 120.f);
				DrawDebugLine(GetWorld(), AgentLocation, MovementEnd, FColor::Green);
			}
		}

		pPriorityAgent->SetSteeringBehavior(pPrioritySteering.get());
	}
}


