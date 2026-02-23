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

SteeringOutput Face::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput steering{};

    const FVector2D toTarget = Target.Position - Agent.GetPosition();
    if (toTarget.Size() < 1.f)
        return steering;

    const FVector2D desiredDirection = toTarget.GetSafeNormal();
    const float desiredYaw = FMath::RadiansToDegrees(FMath::Atan2(desiredDirection.Y, desiredDirection.X));
    const float currentYaw = Agent.GetRotation();
    const float deltaYaw = FMath::FindDeltaAngleDegrees(currentYaw, desiredYaw);

    const float safeDeltaT = FMath::Max(DeltaT, KINDA_SMALL_NUMBER);
    steering.AngularVelocity = FMath::Clamp(deltaYaw / safeDeltaT, -Agent.GetMaxAngularSpeed(), Agent.GetMaxAngularSpeed());
    return steering;
}

SteeringOutput Pursuit::CalculateSteering(float DeltaT, ASteeringAgent& Agent)
{
    SteeringOutput steering{};

    FTargetData targetData = Target;
    if (TargetAgent)
    {
        targetData.Position = TargetAgent->GetPosition();
        targetData.Orientation = TargetAgent->GetRotation();
        targetData.LinearVelocity = TargetAgent->GetLinearVelocity();
        targetData.AngularVelocity = TargetAgent->GetAngularVelocity();
    }

    const FVector2D toTarget = targetData.Position - Agent.GetPosition();
    const float distance = toTarget.Size();
    if (distance < 1.f)
        return steering;

    const float ownSpeed = FMath::Max(Agent.GetMaxLinearSpeed(), KINDA_SMALL_NUMBER);
    const float predictionTime = distance / ownSpeed;
    const FVector2D predictedPos = targetData.Position + (targetData.LinearVelocity * predictionTime);

    steering.LinearVelocity = predictedPos - Agent.GetPosition();
    if (steering.LinearVelocity.SizeSquared() < KINDA_SMALL_NUMBER)
        return steering;

    steering.LinearVelocity.Normalize();
    steering.LinearVelocity *= Agent.GetMaxLinearSpeed();
    return steering;
}
