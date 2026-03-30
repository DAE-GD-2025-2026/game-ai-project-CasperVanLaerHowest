#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "Shared/ImGuiHelpers.h"


Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld)
	: pWorld{pWorld}
	, FlockSize{ FlockSize }
	, pAgentToEvade{pAgentToEvade}
{
	Agents.SetNum(FlockSize);
	
	// 1. Create behaviors
	pSeekBehavior = std::make_unique<Seek>(  );
	pWanderBehavior = std::make_unique<Wander>();
	pEvadeBehavior = std::make_unique<Evade>();
	pEvadeBehavior->SetTargetAgent(pAgentToEvade);
	
	// 2. Create combined behaviors
	pBlendedSteering = std::make_unique<BlendedSteering>(std::vector<BlendedSteering::WeightedBehavior>{
		 {pSeekBehavior.get(), 0.3f},{pWanderBehavior.get(), 0.7f}});
	pPrioritySteering = std::make_unique<PrioritySteering>(std::vector<ISteeringBehavior*>{pEvadeBehavior.get(),
		pBlendedSteering.get()});

	// 3. Create agents in flock
	for (int i = 0; i < FlockSize; ++i)
	{
		const double PosRandX{static_cast<double>(FMath::FRandRange(-WorldSize, WorldSize))};
		const double PosRandY{static_cast<double>(FMath::FRandRange(-WorldSize, WorldSize))};
		
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		
		ASteeringAgent* Agent =
			pWorld->SpawnActor<ASteeringAgent>(AgentClass, 
				FVector{PosRandX, PosRandY, 90}, FRotator::ZeroRotator, Params);
		
		if (!Agent)
		{
			UE_LOG(LogTemp, Error, TEXT("Failed to spawn flock agent"));
			continue;
		}		
		
		Agent->SetSteeringBehavior(pPrioritySteering.get());
		
		Agents[i] = std::move(Agent);
	}
	
	// 4. Initialize pool
	Neighbors.SetNum(Agents.Num());
	NrOfNeighbors = 0;
}

Flock::~Flock()
{
	for (ASteeringAgent* Agent : Agents)
	{
		if (Agent && !Agent->IsPendingKillPending())
		{
			Agent->Destroy(  );
		}
	}
	Agents.Empty(  );
}

void Flock::Tick(float DeltaTime)
{
 // TODO: update the flock
 // TODO: for every agent:
  // TODO: register the neighbors for this agent (-> fill the memory pool with the neighbors for the currently evaluated agent)
  // TODO: update the agent (-> the steeringbehaviors use the neighbors in the memory pool)
  // TODO: trim the agent to the world
	
	for (ASteeringAgent* ag : Agents)
	{
		if (!ag)
			continue;
		// Register the neighbors for this agent (-> fill the memory pool with the neighbors for the currently evaluated agent)
		RegisterNeighbors( ag );
		
		// Update the agent -> the Steering Behaviors use the neighbors in the memory pool
		ag->Tick( DeltaTime );
		TrimAgentToWorld( ag );
	}
}

void Flock::RenderDebug()
{
 // TODO: Render all the agents in the flock
}

void Flock::ImGuiRender(ImVec2 const& WindowPos, ImVec2 const& WindowSize)
{
#ifdef PLATFORM_WINDOWS
#pragma region UI
	//UI
	{
		//Setup
		bool bWindowActive = true;
		ImGui::SetNextWindowPos(WindowPos);
		ImGui::SetNextWindowSize(WindowSize);
		ImGui::Begin("Gameplay Programming", &bWindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

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

		ImGui::Text("Flocking");
		ImGui::Spacing();

  // TODO: implement ImGUI checkboxes for debug rendering here

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();

  // TODO: implement ImGUI sliders for steering behavior weights here
		//End
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
 // TODO: Debugrender the neighbors for the first agent in the flock
}

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
 // TODO: Implement
}
#endif

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;

 // TODO: Implement
	
	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;

 // TODO: Implement

	return avgVelocity;
}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{
 // TODO: Implement
}

void Flock::TrimAgentToWorld(ASteeringAgent* Agent) const
{
	if (!bShouldTrimWorld || !Agent)
		return;
	
	FVector Pos{ Agent->GetActorLocation() };
	
	const float Min{ -TrimWorldSize };
	const float Max{ TrimWorldSize };
	
	if (Pos.X < Min) 
		Pos.X = Max;
	else if (Pos.X > Max) 
		Pos.X = Min;

	if (Pos.Y < Min) 
		Pos.Y = Max;
	else if (Pos.Y > Max) 
		Pos.Y = Min;

	Agent->SetActorLocation(Pos);
}

