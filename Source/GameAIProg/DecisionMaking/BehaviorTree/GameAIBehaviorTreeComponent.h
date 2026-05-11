#pragma once

#include <memory>

#include "CoreMinimal.h"
#include "BrainComponent.h"
#include "BT.h"
#include "GameAIBehaviorTreeComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GAMEAIPROG_API UGameAIBehaviorTreeComponent : public UBrainComponent
{
	GENERATED_BODY()

public:
	UGameAIBehaviorTreeComponent();
	virtual ~UGameAIBehaviorTreeComponent() override;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	virtual void StartLogic() override;
	virtual void StopLogic(const FString& Reason) override;
	virtual bool IsRunning() const override;

	void SetTree(std::unique_ptr<GameAI::BT::BehaviorTree>&& InTree);
	const GameAI::BT::Node* GetRootNode() const;
	const GameAI::BT::Node* GetLastRunningNode() const;
	void SetLastRunningNode(const GameAI::BT::Node* Node);

private:
	std::unique_ptr<GameAI::BT::BehaviorTree> Tree;
	bool bIsRunning{false};
};
