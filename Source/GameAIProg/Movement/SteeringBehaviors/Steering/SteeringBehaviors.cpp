#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput steering{};
    steering.LinearVelocity = Target.Position - Agent.GetPosition();
    steering.LinearVelocity.Normalize();
    steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
    return steering;
}