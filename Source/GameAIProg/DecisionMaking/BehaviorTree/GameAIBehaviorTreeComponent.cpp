#include "GameAIBehaviorTreeComponent.h"

#include "AIController.h"

UGameAIBehaviorTreeComponent::UGameAIBehaviorTreeComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

UGameAIBehaviorTreeComponent::~UGameAIBehaviorTreeComponent() = default;

void UGameAIBehaviorTreeComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bIsRunning || !Tree)
	{
		return;
	}

	if (AAIController* AIController = Cast<AAIController>(GetOwner()))
	{
		Tree->Tick(*AIController, DeltaTime);
	}
}

void UGameAIBehaviorTreeComponent::StartLogic()
{
	Super::StartLogic();

	if (bIsRunning)
	{
		return;
	}

	bIsRunning = true;
}

void UGameAIBehaviorTreeComponent::StopLogic(const FString& Reason)
{
	Super::StopLogic(Reason);

	if (!bIsRunning)
	{
		return;
	}

	if (Tree)
	{
		if (AAIController* AIController = Cast<AAIController>(GetOwner()))
		{
			Tree->Reset(*AIController);
		}
	}

	bIsRunning = false;
}

bool UGameAIBehaviorTreeComponent::IsRunning() const
{
	return bIsRunning;
}

void UGameAIBehaviorTreeComponent::SetTree(std::unique_ptr<GameAI::BT::BehaviorTree>&& InTree)
{
	Tree = std::move(InTree);
}

const GameAI::BT::Node* UGameAIBehaviorTreeComponent::GetRootNode() const
{
	return Tree ? Tree->GetRoot() : nullptr;
}

const GameAI::BT::Node* UGameAIBehaviorTreeComponent::GetLastRunningNode() const
{
	return Tree ? Tree->GetLastRunningNode() : nullptr;
}

void UGameAIBehaviorTreeComponent::SetLastRunningNode(const GameAI::BT::Node* Node)
{
	if (Tree)
	{
		Tree->SetLastRunningNode(Node);
	}
}
