// Fill out your copyright notice in the Description page of Project Settings.


#include "FSMComponent.h"

#include "AIController.h"
#include "FSM.h"
#include "States/State.h"

// Sets default values for this component's properties
UFSMComponent::UFSMComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	FSMInstance = std::make_unique<GameAI::FSM::FSM>();
}

UFSMComponent::~UFSMComponent() = default;


GameAI::FSM::State* UFSMComponent::AddState(std::unique_ptr<GameAI::FSM::State>&& NewState)
{
	if (FSMInstance)
	{
		return FSMInstance->AddState(std::move(NewState));
	}

	return nullptr;
}

void UFSMComponent::AddTransition(GameAI::FSM::State* From, GameAI::FSM::State* To, std::function<bool()> EvalFunc)
{
	if (FSMInstance)
	{
		FSMInstance->AddTransition(From, To, std::move(EvalFunc));
	}
}

// Called when the game starts
void UFSMComponent::BeginPlay()
{
	Super::BeginPlay();
}


// Called every frame
void UFSMComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRunning || !FSMInstance)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		FSMInstance->Update(*AIController, DeltaTime);
	}
}

void UFSMComponent::StartLogic()
{
	Super::StartLogic();

	if (bIsRunning || !FSMInstance)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		FSMInstance->Start(*AIController);
		bIsRunning = true;
	}
}

void UFSMComponent::StopLogic(const FString& Reason)
{
	Super::StopLogic(Reason);

	if (!bIsRunning || !FSMInstance)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		FSMInstance->Stop(*AIController);
	}

	bIsRunning = false;
}

bool UFSMComponent::IsRunning() const
{
	return bIsRunning;
}

const GameAI::FSM::State* UFSMComponent::GetCurrentState() const
{
	return FSMInstance ? FSMInstance->GetCurrentState() : nullptr;
}

