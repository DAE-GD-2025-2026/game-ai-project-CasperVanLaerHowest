// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <functional>
#include <memory>

#include "CoreMinimal.h"
#include "BrainComponent.h"
#include "FSM.h"
#include "FSMComponent.generated.h"

namespace GameAI::FSM
{
	class State;
}

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEAIPROG_API UFSMComponent : public UBrainComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UFSMComponent();
	virtual ~UFSMComponent() override;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void StartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	
	virtual bool IsRunning() const override; 
	
	GameAI::FSM::State* AddState(std::unique_ptr<GameAI::FSM::State>&& NewState);
	void AddTransition(GameAI::FSM::State* From, GameAI::FSM::State* To, std::function<bool()> EvalFunc);
	const GameAI::FSM::State* GetCurrentState() const;
		
protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	std::unique_ptr<GameAI::FSM::FSM> FSMInstance;
	bool bIsRunning{false};
};
