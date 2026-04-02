#include "Flock.h"
#include "FlockingSteeringBehaviors.h"
#include "Shared/ImGuiHelpers.h"
#include "../SpacePartitioning/SpacePartitioning.h"


Flock::Flock(
	UWorld* pWorld,
	TSubclassOf<ASteeringAgent> AgentClass,
	int FlockSize,
	float WorldSize,
	ASteeringAgent* const pAgentToEvade,
	bool bTrimWorld,
	bool bUseSpacePartitioning)
	: pWorld{pWorld}
	, FlockSize{ FlockSize }
	, pAgentToEvade{pAgentToEvade}
	, TrimWorldSize{WorldSize}
	, bShouldTrimWorld{bTrimWorld}
	, bUseSpacePartitioning{bUseSpacePartitioning}
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
		const double PosRandX{static_cast<double>(FMath::FRandRange(-WorldSize * 0.5f, WorldSize * 0.5f))};
		const double PosRandY{static_cast<double>(FMath::FRandRange(-WorldSize * 0.5f, WorldSize * 0.5f))};
		
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
		
		Agent->SetActorTickEnabled(false);
		
		Agents[i] = std::move(Agent);
	}
	
	// 4. Initialize pool
	
	pPartitionedSpace = std::make_unique<CellSpace>(pWorld, WorldSize, WorldSize, 10, 10, FlockSize);
	for (auto agent : Agents)
	{
		if (!agent)
			continue;
		pPartitionedSpace->AddAgent(*agent);
	}
	Neighbors.SetNum(Agents.Num());
	
	OldPositions.SetNum(Agents.Num());
	
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
	for (int i{}; i < Agents.Num(); ++i)
	{
		if (!Agents[i])
			continue;
		
		if (bUseSpacePartitioning && pPartitionedSpace)
		{
			pPartitionedSpace->UpdateAgentCell(*Agents[i], OldPositions[i]);
			OldPositions[i] = Agents[i]->GetPosition();
			pPartitionedSpace->RegisterNeighbors(*Agents[i], NeighborhoodRadius);
			Agents[i]->Tick(DeltaTime);
			TrimAgentToWorld(Agents[i]);
		}
		else
		{
			RegisterNeighbors( Agents[i] );
			Agents[i]->Tick( DeltaTime );
			TrimAgentToWorld( Agents[i] );
		}
		
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
		ImGui::Checkbox("Use Space Partitioning", &bUseSpacePartitioning);
		bool bEnableSpacePartitioning = bUseSpacePartitioning;
		/*if (ImGui::Checkbox("Use Space Partitioning", &bEnableSpacePartitioning))
		{
			SetUseSpacePartitioning(bEnableSpacePartitioning);
		}*/

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
		if (firstAgent)
		{
			if (bUseSpacePartitioning && pPartitionedSpace)
			{
				pPartitionedSpace->RegisterNeighbors(*firstAgent,NeighborhoodRadius);
				
				DrawDebugCircle(
						GWorld, firstAgent->GetActorLocation(),NeighborhoodRadius,32, FColor::Emerald,false, -1.f,0,
						3.f,FVector(1,0,0),FVector(0,1,0),false);
				
				DrawDebugBox(GWorld,firstAgent->GetActorLocation(),{NeighborhoodRadius,NeighborhoodRadius,0},FColor::Cyan,false,-1.f,0,2.f);
    	
				const auto& neighbors = pPartitionedSpace->GetNeighbors();
				const int neighborCount = FMath::Min(GetNrOfNeighbors(), neighbors.Num());
				for (int i = 0; i < neighborCount; ++i)
				{
					if (!neighbors[i])
						continue;
    
					DrawDebugSphere( GWorld, neighbors[i]->GetActorLocation(), 35.f, 
						8, FColor::Green,false,-1.f,0,2.f);
				}
			}
			else
			{
				RegisterNeighbors(firstAgent);
    	
    				DrawDebugCircle(
    					GWorld, firstAgent->GetActorLocation(),NeighborhoodRadius,32, FColor::Emerald,false, -1.f,0,
    					3.f,FVector(1,0,0),FVector(0,1,0),false);
    	
    				const auto& neighbors = GetNeighbors();
    				const int neighborCount = FMath::Min(GetNrOfNeighbors(), neighbors.Num());
    				for (int i = 0; i < neighborCount; ++i)
    				{
    					if (!neighbors[i])
    						continue;
    
    					DrawDebugSphere( GWorld, neighbors[i]->GetActorLocation(), 35.f, 
    						8, FColor::Green,false,-1.f,0,2.f);
    				}
			}
		}
	}
	if (DebugRenderPartitions)
	{
		if (bUseSpacePartitioning && pPartitionedSpace)
		{
			//TODO: Implement
			pPartitionedSpace->RenderCells();
		}
	}
	
}

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
			if (NrOfNeighbors >= Neighbors.Num())
				return;
			Neighbors[NrOfNeighbors] = agent;
			NrOfNeighbors++;
		}
	}
}

FVector2D Flock::GetAverageNeighborPos() const
{
	FVector2D avgPosition = FVector2D::ZeroVector;
	
	if (GetNrOfNeighbors() == 0)
		return avgPosition;

	const auto& neighbors = GetNeighbors();
	const int neighborCount = FMath::Min(GetNrOfNeighbors(), neighbors.Num());
	if (neighborCount == 0)
		return avgPosition;
	for (int i{}; i < neighborCount; i++ )
	{
		if (!neighbors[i])
			continue;
		avgPosition += neighbors[i]->GetPosition();
	}
	
	
	avgPosition /= neighborCount;
	
	return avgPosition;
}

FVector2D Flock::GetAverageNeighborVelocity() const
{
	FVector2D avgVelocity = FVector2D::ZeroVector;

	if (GetNrOfNeighbors() == 0)
		return avgVelocity;
	const auto& neighbors = GetNeighbors();
	const int neighborCount = FMath::Min(GetNrOfNeighbors(), neighbors.Num());
	if (neighborCount == 0)
		return avgVelocity;
	for (int i{}; i < neighborCount; i++ )
	{
		if (!neighbors[i])
			continue;
		avgVelocity += neighbors[i]->GetLinearVelocity();
	}
	

	avgVelocity /= neighborCount;
	
	return avgVelocity;
}

void Flock::SetTarget_Seek(FSteeringParams const& Target)
{

	pSeekBehavior->SetTarget(Target);
}

void Flock::SetUseSpacePartitioning(bool bEnable)
{
	if (bUseSpacePartitioning == bEnable)
		return;

	bUseSpacePartitioning = bEnable;

	if (bUseSpacePartitioning)
	{
		pPartitionedSpace = std::make_unique<CellSpace>(pWorld, TrimWorldSize, TrimWorldSize, 10, 10, FlockSize);
		for (auto agent : Agents)
		{
			if (!agent)
				continue;
			pPartitionedSpace->AddAgent(*agent);
		}
	}
	else
	{
		pPartitionedSpace.reset();
		Neighbors.SetNum(Agents.Num());
		NrOfNeighbors = 0;
	}
}

int Flock::GetNrOfNeighbors() const
{
	if (bUseSpacePartitioning && pPartitionedSpace)
		return pPartitionedSpace->GetNrOfNeighbors();
	return NrOfNeighbors;
}

const TArray<ASteeringAgent*>& Flock::GetNeighbors() const
{
	if (bUseSpacePartitioning && pPartitionedSpace)
		return pPartitionedSpace->GetNeighbors();
	return Neighbors;
}

void Flock::TrimAgentToWorld(ASteeringAgent* Agent) const
{
	if (!bShouldTrimWorld || !Agent)
		return;
	
	FVector Pos{ Agent->GetActorLocation() };
	
	const float Min{ -TrimWorldSize * 0.5f };
	const float Max{ TrimWorldSize * 0.5f };
	
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
