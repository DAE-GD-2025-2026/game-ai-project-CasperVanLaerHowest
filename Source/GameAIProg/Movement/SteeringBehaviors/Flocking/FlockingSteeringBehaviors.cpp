#include "FlockingSteeringBehaviors.h"
#include "Flock.h"
#include "../SteeringAgent.h"
#include "../SteeringHelpers.h"


//*******************
//COHESION (FLOCKING)
SteeringOutput Cohesion::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput steering{};
	if (!pFlock || pFlock->GetNrOfNeighbors() == 0)
	{
		steering.IsValid = false;
		return steering;
	}

	FTargetData target = GetTarget();
	target.Position = pFlock->GetAverageNeighborPos();
	SetTarget(target);
	return Seek::CalculateSteering(deltaT, pAgent);
}

//*********************
//SEPARATION (FLOCKING)
SteeringOutput Separation::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput steering{};
	
	if (!pFlock || pFlock->GetNrOfNeighbors() == 0)
	{
		steering.IsValid = false;
		return steering;
	}
	
	FVector2D force{ 0.f };
	const auto& Neighbors{pFlock->GetNeighbors()};
	const int neighborCount = FMath::Min(pFlock->GetNrOfNeighbors(), Neighbors.Num());
	for (int i{}; i < neighborCount; i++)
	{
		const auto neighbourToAgent = pAgent.GetPosition() - Neighbors[i]->GetPosition();
		
		auto distanceSqr = neighbourToAgent.SizeSquared();
		
		if (distanceSqr > 1.f)
			force += neighbourToAgent / distanceSqr;
	}
	
	force.Normalize();
	force *= pAgent.GetMaxLinearSpeed();
	
	steering.LinearVelocity = force;
	return steering;
}

//*************************
//VELOCITY MATCH (FLOCKING)
SteeringOutput VelocityMatch::CalculateSteering(float deltaT, ASteeringAgent& pAgent)
{
	SteeringOutput steering{};
	
	steering.LinearVelocity = pFlock->GetAverageNeighborVelocity();
	
	return steering;
}
