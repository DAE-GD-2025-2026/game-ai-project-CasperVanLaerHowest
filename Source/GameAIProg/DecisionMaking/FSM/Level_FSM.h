// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "InputAction.h"
#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
#include "Shared/Level_Base.h"
#include "Level_FSM.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_FSM : public ALevel_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="FSM|Input")
	UInputAction* SetThiefTargetAction{};

	// Sets default values for this actor's properties
	ALevel_FSM();

	// Called every frame
	virtual void Tick(float DeltaTime) override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	virtual void BindLevelInputActions() override;
	void UpdateImGui();

private:
	UPROPERTY(EditAnywhere, Category="FSM|Debug")
	float GuardDetectionRadius{1400.f};

	UPROPERTY()
	ASteeringAgent* GuardAgent{nullptr};
	UPROPERTY()
	ASteeringAgent* ThiefAgent{nullptr};

	std::unique_ptr<Seek> ThiefSeek{};
	void SetThiefTargetFromMouse();
	bool bWasLeftMouseDown{false};
};
