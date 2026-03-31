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
	pCohesionBehavior = std::make_unique<Cohesion>(this);
	pSeparationBehavior = std::make_unique<Separation>(this);
	pVelMatchBehavior = std::make_unique<VelocityMatch>(this);
	pEvadeBehavior = std::make_unique<Evade>();
	pEvadeBehavior->SetTargetAgent(pAgentToEvade);
	
	// 2. Create combined behaviors
	pBlendedSteering = std::make_unique<BlendedSteering>(std::vector<BlendedSteering::WeightedBehavior>{
		 {pSeekBehavior.get(), 0.1f},{pWanderBehavior.get(), 0.4f},
		{pCohesionBehavior.get(), 0.2f},{pSeparationBehavior.get(), 0.1f},
		{pVelMatchBehavior.get(), 0.2f}});
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
	for (ASteeringAgent* agent : Agents)
	{
		if (!agent)
			continue;
		
		RegisterNeighbors( agent );
		
		agent->Tick( DeltaTime );
		TrimAgentToWorld( agent );
	}
}

void Flock::RenderDebug()
{
 // TODO: Render all the agents in the flock
	RenderNeighborhood();
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
		
		ImGui::Checkbox("Show Neighborhood Debug", &DebugRenderNeighborhood);
		ImGui::Checkbox("Show Render Partitions", &DebugRenderPartitions);
		ImGui::Checkbox("Show Steering", &DebugRenderSteering);

		ImGui::Text("Behavior Weights");
		ImGui::Spacing();
		
		auto& Weights = pBlendedSteering->GetWeightedBehaviorsRef();
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Seek", Weights[0].Weight, 0.f, 1.f,
			[&](float w)
		{
				Weights[0].Weight = w;
		},"%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Wander", Weights[1].Weight, 0.f, 1.f,
			[&](float w)
		{
				Weights[1].Weight = w;
		},"%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Cohesion", Weights[2].Weight, 0.f, 1.f,
			[&](float w)
		{
				Weights[2].Weight = w;
		},"%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Separation", Weights[3].Weight, 0.f, 1.f,
			[&](float w)
		{
				Weights[3].Weight = w;
		},"%.2f");
		
		ImGuiHelpers::ImGuiSliderFloatWithSetter("Velocity match", Weights[4].Weight, 0.f, 1.f,
			[&](float w)
		{
				Weights[4].Weight = w;
		},"%.2f");
		
		ImGui::End();
	}
#pragma endregion
#endif
}

void Flock::RenderNeighborhood()
{
	if (DebugRenderSteering)
	{
		DrawDebugCircle(
				GWorld,
				pAgentToEvade->GetActorLocation(),
				pEvadeBehavior->GetEvadeRadius(),  
				32, FColor::Purple, false, -1.f, 0,   
				2.f,  FVector(1, 0, 0), FVector(0, 1, 0), true 
			);
	}
	if (DebugRenderNeighborhood)
	{
		if (Agents.Num() == 0)
			return;

		ASteeringAgent* firstAgent{ Agents[0] };
		if (!firstAgent)
		{
			RegisterNeighbors(firstAgent);
	
			DrawDebugCircle(
				pWorld, firstAgent->GetActorLocation(),NeighborhoodRadius,24, FColor::Yellow,false, -1.f,0,
				3.f,FVector(1,0,0),FVector(0,1,0),false);
	
			for (int i = 0; i < NrOfNeighbors; ++i)
			{
				if (!Neighbors[i])
					continue;

				DrawDebugSphere( pWorld,Neighbors[i]->GetActorLocation(), 35.f, 
					8, FColor::Green,false,-1.f,0,2.f);
			}
		}
	}
	if (DebugRenderPartitions)
	{
		//TODO: Implement
	}
	
}

#ifndef GAMEAI_USE_SPACE_PARTITIONING
void Flock::RegisterNeighbors(ASteeringAgent* const pAgent)
{
	NrOfNeighbors = 0;
	
	for (auto agent : Agents)
	{
		if (!agent)
	 		continue;
		
		if (agent == pAgent)
			continue;
		
		const float distance = FVector::DistSquared(agent->GetActorLocation(), pAgent->GetActorLocation());
		
		if (distance <= NeighborhoodRadius * NeighborhoodRadius)
		{
			Neighbors[NrOfNeighbors] = agent;
			NrOfNeighbors++;
		}
	}
}
#endif

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;
	
	if (NrOfNeighbors == 0)
		return avgPosition;
	
	for (int i{}; i < NrOfNeighbors; i++ )
	{
		avgPosition += Neighbors[i]->GetPosition();
	}
	
	avgPosition /= NrOfNeighbors;
	
	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;

	if (NrOfNeighbors == 0)
		return avgVelocity;
	
	for (int i{}; i < NrOfNeighbors; i++ )
	{
		avgVelocity += Neighbors[i]->GetLinearVelocity();
	}

	avgVelocity /= NrOfNeighbors;
	
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

