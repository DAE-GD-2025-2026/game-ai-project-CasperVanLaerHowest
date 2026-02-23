#include "SteeringBehaviors.h"
#include "GameAIProg/Movement/SteeringBehaviors/SteeringAgent.h"

SteeringOutput Seek::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput steering{};
    if ((Target.Position - Agent.GetPosition()).Size() < 1)
        return steering;
    steering.LinearVelocity = Target.Position - Agent.GetPosition();
    steering.LinearVelocity.Normalize();
    steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
    return steering;
}

SteeringOutput Flee::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput steering{};
    if ((Target.Position - Agent.GetPosition()).Size() > m_radius)
        return steering;
    steering.LinearVelocity = Target.Position + Agent.GetPosition();
    steering.LinearVelocity.Normalize();
    steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
    return steering;
}