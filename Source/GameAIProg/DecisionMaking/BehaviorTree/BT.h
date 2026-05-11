#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "CoreMinimal.h"

class AAIController;

namespace GameAI::BT
{
	enum class ENodeResult : uint8
	{
		Succeeded,
		Failed,
		Running
	};

	class Node
	{
	public:
		explicit Node(const char* InDebugName);
		virtual ~Node() = default;

		virtual ENodeResult Tick(AAIController& Controller, float DeltaTime) = 0;
		virtual void Reset(AAIController& Controller);
		const char* GetDebugName() const;

	private:
		const char* DebugName;
	};

	class CompositeNode : public Node
	{
	public:
		explicit CompositeNode(const char* InDebugName);

		void AddChild(std::unique_ptr<Node>&& Child);
		virtual void Reset(AAIController& Controller) override;

	protected:
		std::vector<std::unique_ptr<Node>> Children;
	};

	class Sequence final : public CompositeNode
	{
	public:
		explicit Sequence(const char* InDebugName = "Sequence");
		virtual ENodeResult Tick(AAIController& Controller, float DeltaTime) override;
	};

	class Selector final : public CompositeNode
	{
	public:
		explicit Selector(const char* InDebugName = "Selector");
		virtual ENodeResult Tick(AAIController& Controller, float DeltaTime) override;
	};

	class SimpleParallel final : public Node
	{
	public:
		SimpleParallel(std::unique_ptr<Node>&& InMainTask, std::unique_ptr<Node>&& InBackgroundTree,
			bool bInWaitForBackground = false, const char* InDebugName = "SimpleParallel");

		virtual ENodeResult Tick(AAIController& Controller, float DeltaTime) override;
		virtual void Reset(AAIController& Controller) override;

	private:
		std::unique_ptr<Node> MainTask;
		std::unique_ptr<Node> BackgroundTree;
		bool bWaitForBackground;
		bool bMainFinished{false};
		ENodeResult MainResult{ENodeResult::Failed};
	};

	class Action final : public Node
	{
	public:
		using TickFunction = std::function<ENodeResult(AAIController&, float)>;
		using ResetFunction = std::function<void(AAIController&)>;

		Action(const char* InDebugName, TickFunction InTickFunction, ResetFunction InResetFunction = {});

		virtual ENodeResult Tick(AAIController& Controller, float DeltaTime) override;
		virtual void Reset(AAIController& Controller) override;

	private:
		TickFunction TickFunctionInstance;
		ResetFunction ResetFunctionInstance;
	};

	class BehaviorTree
	{
	public:
		void SetRoot(std::unique_ptr<Node>&& InRoot);
		void Tick(AAIController& Controller, float DeltaTime);
		void Reset(AAIController& Controller);
		const Node* GetRoot() const;
		const Node* GetLastRunningNode() const;
		void SetLastRunningNode(const Node* Node);

	private:
		std::unique_ptr<Node> Root;
		const Node* LastRunningNode{nullptr};
	};
}
