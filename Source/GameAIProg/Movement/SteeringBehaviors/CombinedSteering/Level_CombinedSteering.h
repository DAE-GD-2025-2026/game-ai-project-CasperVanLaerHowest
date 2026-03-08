// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>
#include "CoreMinimal.h"
#include "CombinedSteeringBehaviors.h"
#include "GameAIProg/Shared/Level_Base.h"
#include "GameAIProg/Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"
#include "Level_CombinedSteering.generated.h"



UCLASS()
class GAMEAIPROG_API ALevel_CombinedSteering : public ALevel_Base
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	ALevel_CombinedSteering();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	virtual void BeginDestroy() override;

private:
	//Datamembers
	bool UseMouseTarget = false;
	bool CanDebugRender = false;
	ASteeringAgent* pCombinedAgent{nullptr}; // non-owning, world-owned actor
	ASteeringAgent* pPriorityAgent{nullptr}; // non-owning, world-owned actor
	std::unique_ptr<Seek> pSeekBehavior{nullptr};     // non-owning, owned by blended steering setup
	std::unique_ptr<Wander> pWanderBehavior{nullptr}; // non-owning, owned by blended steering setup
	std::unique_ptr<Evade> pEvadeBehavior{nullptr};   // non-owning, owned by blended steering setup (optional)
	std::unique_ptr<Seek> pPrioritySeekBehavior{nullptr};
	std::unique_ptr<Wander> pPriorityWanderBehavior{nullptr};
	std::unique_ptr<Evade> pPriorityEvadeBehavior{nullptr};

	enum class BehaviorTypes
	{
		Seek,
		Wander,
		Flee,
		Arrive,
		Evade,
		Pursuit,
		Face,
		// @ End
		Count
	};

	struct ImGui_Agent final
	{
		ASteeringAgent* Agent{nullptr};
		std::unique_ptr<ISteeringBehavior> Behavior{nullptr};
		int SelectedBehavior{static_cast<int>(BehaviorTypes::Seek)};
		int SelectedTarget = -1;
	};
	
	std::unique_ptr<BlendedSteering> pBlendedSteering{nullptr};
	std::unique_ptr<PrioritySteering> pPrioritySteering{nullptr};
	
};
