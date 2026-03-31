#pragma once
#include "Movement/SteeringBehaviors/Steering/SteeringBehaviors.h"
class Flock;

//COHESION - FLOCKING
//*******************
class Cohesion final : public Seek
{
public:
	Cohesion(Flock* const pFlock) :pFlock(pFlock) {};

	//Cohesion Behavior
	SteeringOutput CalculateSteering(float deltaT, ASteeringAgent& pAgent) override;

private:
	Flock* pFlock = nullptr;
};

//SEPARATION - FLOCKING
//*********************
class Separation final
{
public:
	Separation(Flock* const pFlock) :pFlock(pFlock) {};
	
	//Separation Behavior
	SteeringOutput CalculateSteering(float deltaT, ASteeringAgent& pAgent);
private:
	Flock* pFlock = nullptr;
};

//VELOCITY MATCH - FLOCKING
//************************
class VelocityMatch final
{
public:
	VelocityMatch(Flock* const pFlock) :pFlock(pFlock) {};
	
	//Velocity Match Behavior
	SteeringOutput CalculateSteering(float deltaT, ASteeringAgent& pAgent);
private:
	Flock* pFlock = nullptr;
};