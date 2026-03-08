#pragma once

#include <Movement/SteeringBehaviors/SteeringHelpers.h>
#include "Kismet/KismetMathLibrary.h"

class ASteeringAgent;

// SteeringBehavior base, all steering behaviors should derive from this.
class ISteeringBehavior
{
public:
	ISteeringBehavior() = default;
	virtual ~ISteeringBehavior() = default;

	// Override to implement your own behavior
	virtual SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent & Agent) = 0;

	void SetTarget(const FTargetData& NewTarget) { Target = NewTarget; }
	const FTargetData& GetTarget() const { return Target; }
	template<class T, std::enable_if_t<std::is_base_of_v<ISteeringBehavior, T>>* = nullptr>
	T* As()
	{ return static_cast<T*>(this); }

protected:
	FTargetData Target;
};

// Your own SteeringBehaviors should follow here...

class Seek : public ISteeringBehavior
{
public:
    Seek() = default;
    virtual ~Seek() = default;

    SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Flee : public ISteeringBehavior
{
public:
	Flee() = default;
	virtual ~Flee() = default;
	const float GetRadius() const {return m_radius;}
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
private:
	float m_radius{ 300.f }; // radius within which the agent will attempt to flee from the target
};

class Arrive : public ISteeringBehavior
{
public:
	Arrive() = default;
	virtual ~Arrive() = default;
	const float GetRadiusFar() const {return m_radiusFar;}
	const float GetRadiusNear() const {return m_radiusNear;}
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
private:
	float m_radiusFar{ 300.f };
	float m_radiusNear{ 100.f };
};

class Face : public ISteeringBehavior
{
public:
	Face() = default;
	virtual ~Face() = default;

	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;
};

class Pursuit : public ISteeringBehavior
{
public:
	Pursuit() = default;
	virtual ~Pursuit() = default;
	void SetTargetAgent(ASteeringAgent* InTargetAgent) { TargetAgent = InTargetAgent; }
	ASteeringAgent* GetTargetAgent() const { return TargetAgent; }
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

private:
	ASteeringAgent* TargetAgent{nullptr}; // non-owning
};

class Evade : public ISteeringBehavior
{
public:
	Evade() = default;
	virtual ~Evade() = default;
	void SetTargetAgent(ASteeringAgent* InTargetAgent) { TargetAgent = InTargetAgent; }
	ASteeringAgent* GetTargetAgent() const { return TargetAgent; }
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

private:
	ASteeringAgent* TargetAgent{nullptr}; // non-owning
};

class Wander : public Seek
{
public:
	Wander() = default;
	virtual ~Wander() = default;

	//Wander Behavior
	SteeringOutput CalculateSteering(float DeltaT, ASteeringAgent& Agent) override;

	void SetWanderOffset(float offset) { m_OffsetDistance = offset; }
	void SetWanderRadius(float radius) { m_Radius = radius; }
	void SetMaxAngleChange(float rad) { m_MaxAngleChange = rad; }

protected:
	float m_OffsetDistance = 60.f; //Offset (Agent Direction)
	float m_Radius = 20.f; //WanderRadius
	float m_MaxAngleChange = FMath::DegreesToRadians(45); //Max random angle offset from forward (radians)
	float m_WanderAngle = 0.f; //Internal (radians)
	bool m_HasTarget = false;
	
	FVector2D m_Target{};
};
