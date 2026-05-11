#include "BT.h"

#include "AIController.h"

namespace GameAI::BT
{
	Node::Node(const char* InDebugName)
		: DebugName(InDebugName)
	{
	}

	void Node::Reset(AAIController& Controller)
	{
	}

	const char* Node::GetDebugName() const
	{
		return DebugName;
	}

	CompositeNode::CompositeNode(const char* InDebugName)
		: Node(InDebugName)
	{
	}

	void CompositeNode::AddChild(std::unique_ptr<Node>&& Child)
	{
		Children.emplace_back(std::move(Child));
	}

	void CompositeNode::Reset(AAIController& Controller)
	{
		for (const std::unique_ptr<Node>& Child : Children)
		{
			if (Child)
			{
				Child->Reset(Controller);
			}
		}
	}

	Sequence::Sequence(const char* InDebugName)
		: CompositeNode(InDebugName)
	{
	}

	ENodeResult Sequence::Tick(AAIController& Controller, float DeltaTime)
	{
		for (const std::unique_ptr<Node>& Child : Children)
		{
			if (!Child)
			{
				continue;
			}

			const ENodeResult Result = Child->Tick(Controller, DeltaTime);
			if (Result != ENodeResult::Succeeded)
			{
				return Result;
			}
		}

		return ENodeResult::Succeeded;
	}

	Selector::Selector(const char* InDebugName)
		: CompositeNode(InDebugName)
	{
	}

	ENodeResult Selector::Tick(AAIController& Controller, float DeltaTime)
	{
		for (const std::unique_ptr<Node>& Child : Children)
		{
			if (!Child)
			{
				continue;
			}

			const ENodeResult Result = Child->Tick(Controller, DeltaTime);
			if (Result != ENodeResult::Failed)
			{
				return Result;
			}
		}

		return ENodeResult::Failed;
	}

	SimpleParallel::SimpleParallel(std::unique_ptr<Node>&& InMainTask, std::unique_ptr<Node>&& InBackgroundTree,
		bool bInWaitForBackground, const char* InDebugName)
		: Node(InDebugName)
		, MainTask(std::move(InMainTask))
		, BackgroundTree(std::move(InBackgroundTree))
		, bWaitForBackground(bInWaitForBackground)
	{
	}

	ENodeResult SimpleParallel::Tick(AAIController& Controller, float DeltaTime)
	{
		const ENodeResult BackgroundResult = BackgroundTree
			? BackgroundTree->Tick(Controller, DeltaTime)
			: ENodeResult::Succeeded;

		if (!bMainFinished && MainTask)
		{
			MainResult = MainTask->Tick(Controller, DeltaTime);
			bMainFinished = MainResult != ENodeResult::Running;
		}

		if (!bMainFinished)
		{
			return ENodeResult::Running;
		}

		if (bWaitForBackground && BackgroundResult == ENodeResult::Running)
		{
			return ENodeResult::Running;
		}

		return MainResult;
	}

	void SimpleParallel::Reset(AAIController& Controller)
	{
		bMainFinished = false;
		MainResult = ENodeResult::Failed;

		if (MainTask)
		{
			MainTask->Reset(Controller);
		}
		if (BackgroundTree)
		{
			BackgroundTree->Reset(Controller);
		}
	}

	Action::Action(const char* InDebugName, TickFunction InTickFunction, ResetFunction InResetFunction)
		: Node(InDebugName)
		, TickFunctionInstance(std::move(InTickFunction))
		, ResetFunctionInstance(std::move(InResetFunction))
	{
	}

	ENodeResult Action::Tick(AAIController& Controller, float DeltaTime)
	{
		return TickFunctionInstance ? TickFunctionInstance(Controller, DeltaTime) : ENodeResult::Failed;
	}

	void Action::Reset(AAIController& Controller)
	{
		if (ResetFunctionInstance)
		{
			ResetFunctionInstance(Controller);
		}
	}

	void BehaviorTree::SetRoot(std::unique_ptr<Node>&& InRoot)
	{
		Root = std::move(InRoot);
		LastRunningNode = nullptr;
	}

	void BehaviorTree::Tick(AAIController& Controller, float DeltaTime)
	{
		if (!Root)
		{
			LastRunningNode = nullptr;
			return;
		}

		const ENodeResult Result = Root->Tick(Controller, DeltaTime);
		if (Result != ENodeResult::Running)
		{
			Root->Reset(Controller);
			LastRunningNode = nullptr;
		}
	}

	void BehaviorTree::Reset(AAIController& Controller)
	{
		if (Root)
		{
			Root->Reset(Controller);
		}

		LastRunningNode = nullptr;
	}

	const Node* BehaviorTree::GetRoot() const
	{
		return Root.get();
	}

	const Node* BehaviorTree::GetLastRunningNode() const
	{
		return LastRunningNode;
	}

	void BehaviorTree::SetLastRunningNode(const Node* Node)
	{
		LastRunningNode = Node;
	}
}
