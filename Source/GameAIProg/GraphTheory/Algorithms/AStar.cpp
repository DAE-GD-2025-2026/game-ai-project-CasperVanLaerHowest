#include "AStar.h"

#include <algorithm>

using namespace GameAI;

AStar::AStar(Graph* const pGraph, HeuristicFunctions::Heuristic hFunction)
	: pGraph(pGraph)
	, HeuristicFunction(hFunction)
{
}

std::vector<Node*>AStar::FindPath(Node* const pStartNode, Node* const pGoalNode)
{
	std::vector<Node*> path{};
	if (!pStartNode || !pGoalNode || !pGraph)
	{
		return path;
	}

	std::vector<NodeRecord> openList{};
	std::vector<NodeRecord> closedList{};

	NodeRecord startRecord{};
	startRecord.pNode = pStartNode;
	startRecord.pConnection = nullptr;
	startRecord.costSoFar = 0.0f;
	startRecord.estimatedTotalCost = GetHeuristicCost(pStartNode, pGoalNode);
	openList.push_back(startRecord);

	NodeRecord currentRecord{};
	bool foundGoal = false;

	while (!openList.empty())
	{
		auto currentIt = std::min_element(openList.begin(), openList.end());
		currentRecord = *currentIt;
		openList.erase(currentIt);

		if (currentRecord.pNode == pGoalNode)
		{
			foundGoal = true;
			closedList.push_back(currentRecord);
			break;
		}

		auto const connections = pGraph->FindConnectionsFrom(currentRecord.pNode->GetId());
		for (Connection* const pConnection : connections)
		{
			Node* const pNextNode = pGraph->GetNode(pConnection->GetToId()).get();
			float const gCost = currentRecord.costSoFar + pConnection->GetWeight();

			auto closedIt = std::find_if(closedList.begin(), closedList.end(),
				[pNextNode](NodeRecord const& record) { return record.pNode == pNextNode; });
			if (closedIt != closedList.end())
			{
				if (closedIt->costSoFar <= gCost)
				{
					continue;
				}
				closedList.erase(closedIt);
			}

			auto openIt = std::find_if(openList.begin(), openList.end(),
				[pNextNode](NodeRecord const& record) { return record.pNode == pNextNode; });
			if (openIt != openList.end())
			{
				if (openIt->costSoFar <= gCost)
				{
					continue;
				}
				openList.erase(openIt);
			}

			NodeRecord nextRecord{};
			nextRecord.pNode = pNextNode;
			nextRecord.pConnection = pConnection;
			nextRecord.costSoFar = gCost;
			nextRecord.estimatedTotalCost = gCost + GetHeuristicCost(pNextNode, pGoalNode);
			openList.push_back(nextRecord);
		}

		closedList.push_back(currentRecord);
	}

	if (!foundGoal)
	{
		return path;
	}

	NodeRecord backtrackRecord = currentRecord;
	while (backtrackRecord.pNode != pStartNode)
	{
		path.push_back(backtrackRecord.pNode);
		if (!backtrackRecord.pConnection)
		{
			break;
		}

		int const fromId = backtrackRecord.pConnection->GetFromId();
		auto prevIt = std::find_if(closedList.begin(), closedList.end(),
			[fromId](NodeRecord const& record) { return record.pNode->GetId() == fromId; });
		if (prevIt == closedList.end())
		{
			break;
		}
		backtrackRecord = *prevIt;
	}

	path.push_back(pStartNode);
	std::reverse(path.begin(), path.end());
	return path;
}

float AStar::GetHeuristicCost(Node* const pStartNode, Node* const pEndNode) const
{
	FVector2D toDestination = pGraph->GetNode(pEndNode->GetId())->GetPosition() - pGraph->GetNode(pStartNode->GetId())->GetPosition();
	return HeuristicFunction(abs(toDestination.X), abs(toDestination.Y));
}
