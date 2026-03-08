
#include "CombinedSteeringBehaviors.h"
#include <algorithm>
#include "../SteeringAgent.h"

BlendedSteering::BlendedSteering(const std::vector<WeightedBehavior>& WeightedBehaviors)
	:WeightedBehaviors(WeightedBehaviors)
{};

//****************
//BLENDED STEERING
SteeringOutput BlendedSteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput BlendedOutput{};
	float TotalWeight = 0.f;

	for (const WeightedBehavior& WeightedBehavior : WeightedBehaviors)
	{
		if (!WeightedBehavior.pBehavior || WeightedBehavior.Weight <= 0.f)
			continue;

		const SteeringOutput BehaviorOutput = WeightedBehavior.pBehavior->CalculateSteering(DeltaT, Agent);
		BlendedOutput.LinearVelocity += BehaviorOutput.LinearVelocity * WeightedBehavior.Weight;
		BlendedOutput.AngularVelocity += BehaviorOutput.AngularVelocity * WeightedBehavior.Weight;
	}

	return BlendedOutput;
}

float* BlendedSteering::GetWeight(ISteeringBehavior* const SteeringBehavior)
{
	auto it = find_if(WeightedBehaviors.begin(),
		WeightedBehaviors.end(),
		[SteeringBehavior](const WeightedBehavior& Elem)
		{
			return Elem.pBehavior == SteeringBehavior;
		}
	);

	if(it!= WeightedBehaviors.end())
		return &it->Weight;
	
	return nullptr;
}

//*****************
//PRIORITY STEERING
SteeringOutput PrioritySteering::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
	SteeringOutput Steering{};
	Steering.IsValid = false;

	for (ISteeringBehavior* const pBehavior : m_PriorityBehaviors)
	{
		if (!pBehavior)
			continue;

		Steering = pBehavior->CalculateSteering(DeltaT, Agent);

		// Take the first behavior that actually moves the agent.
		if (Steering.LinearVelocity.SizeSquared() > KINDA_SMALL_NUMBER)
		{
			Steering.IsValid = true;
			break;
		}
	}

	//If non of the behavior return a valid output, last behavior is returned
	return Steering;
}
