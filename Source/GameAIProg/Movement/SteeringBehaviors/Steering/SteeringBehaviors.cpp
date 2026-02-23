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
    steering.LinearVelocity = Target.Position - Agent.GetPosition();
    steering.LinearVelocity *= -1;
    steering.LinearVelocity.Normalize();
    steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
    return steering;
}

SteeringOutput Arrive::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput steering{};
    const FVector2D toTarget = Target.Position - Agent.GetPosition();
    const float distance = toTarget.Size();

    if (distance < 1.f)
        return steering;

    const FVector2D direction = toTarget.GetSafeNormal();
    float desiredSpeed = Agent.GetMaxLinearSpeed();

    if (distance < m_radiusNear)
    {
        desiredSpeed = 0.f;
    }
    else if (distance < m_radiusFar)
    {
        const float slowRange = m_radiusFar - m_radiusNear;
        const float speedFactor = slowRange > 0.f ? (distance - m_radiusNear) / slowRange : 0.f;
        desiredSpeed = Agent.GetMaxLinearSpeed() * FMath::Clamp(speedFactor, 0.f, 1.f);
    }

    steering.LinearVelocity = direction * desiredSpeed;
    return steering;
}
