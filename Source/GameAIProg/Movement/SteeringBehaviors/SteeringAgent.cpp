// Fill out your copyright notice in the Description page of Project Settings.

#include "SteeringAgent.h"


// Sets default values
ASteeringAgent::ASteeringAgent()
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASteeringAgent::BeginPlay()
{
	Super::BeginPlay();
}

void ASteeringAgent::BeginDestroy()
{
	Super::BeginDestroy();
}

// Called every frame
void ASteeringAgent::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SteeringBehavior)
	{
		SteeringOutput output = SteeringBehavior->CalculateSteering(DeltaTime, *this);
		
		const float maxSpeed = GetMaxLinearSpeed();
		const float speed = output.LinearVelocity.Size();
		const float speedScale = maxSpeed > 0.f ? FMath::Clamp(speed / maxSpeed, 0.f, 1.f) : 0.f;
		const FVector2D direction2D = output.LinearVelocity.GetSafeNormal();
		AddMovementInput(FVector{direction2D, 0.f}, speedScale);
		AddActorWorldRotation(FRotator{0.f, output.AngularVelocity * DeltaTime, 0.f});
	
	}
}

// Called to bind functionality to input
void ASteeringAgent::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

void ASteeringAgent::SetSteeringBehavior(ISteeringBehavior* NewSteeringBehavior)
{
	SteeringBehavior = NewSteeringBehavior;
}

