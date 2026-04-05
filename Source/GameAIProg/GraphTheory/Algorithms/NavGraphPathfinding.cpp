#include "NavGraphPathfinding.h"

#include "AStar.h"
#include "PathSmoothing.h"
#include "VectorTypes.h"
#include "Shared/Graph/NavGraph/NavGraph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

using namespace GameAI;

void ConnectNodeToTriangle(NavGraph* const pNavGraph, TriPolygon::Triangle const* pTriangle, int const nodeId)
{
	if (!pNavGraph || !pTriangle)
	{
		return;
	}
	
	TriPolygon const* const pNavPolygon = pNavGraph->GetNavPolygon();
	if (!pNavPolygon)
	{
		return;
	}

	for (TriPolygon::Edge const& edge : pTriangle->GetEdges())
	{
		auto const edgeIdx = pNavPolygon->FindEdgeIndex(edge);
		if (!edgeIdx.has_value())
		{
			continue;
		}

		int const edgeNodeId = pNavGraph->GetNodeIdFromEdgeIndex(edgeIdx.value());
		if (edgeNodeId == Graphs::InvalidNodeId || edgeNodeId == nodeId)
		{
			continue;
		}

		pNavGraph->AddConnection(nodeId, edgeNodeId);

		float const cost = (pNavGraph->GetNode(nodeId)->GetPosition() - pNavGraph->GetNode(edgeNodeId)->GetPosition()).Length();
		if (Connection* const pForward = pNavGraph->FindConnection(nodeId, edgeNodeId))
		{
			pForward->SetWeight(cost);
		}
		if (Connection* const pBackward = pNavGraph->FindConnection(edgeNodeId, nodeId))
		{
			pBackward->SetWeight(cost);
		}
	}
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos,
	NavGraph* const pNavGraph, std::vector<FVector2D>& debugNodePositions, std::vector<NavLine>& debugPortals) 
{
	//Create the path to return
	std::vector<FVector2D> finalPath{};
	if (!pNavGraph)
	{
		return finalPath;
	}

	//Get the start and endTriangle
	auto navPolygon = pNavGraph->GetNavPolygon();
	
	if (!navPolygon)
	{
		return finalPath;
	}
	
	auto startTriangle = navPolygon->GetTriangleAtPosition(startPos,true);
	auto endTriangle = navPolygon->GetTriangleAtPosition(endPos, true);
	
	FVector2D outStartPos{ startPos };
	FVector2D outEndPos{ endPos };
	
	if (!startTriangle)
	{
		startTriangle = navPolygon->GetClosestTriangleToPosition(startPos, outStartPos);
	}
	
	if (!endTriangle)
	{
		endTriangle = navPolygon->GetClosestTriangleToPosition(endPos, outEndPos);
	}
	
	if (startTriangle && endTriangle && startTriangle == endTriangle)
	{
		finalPath.push_back(outStartPos);
		finalPath.push_back(outEndPos);
		debugNodePositions = finalPath;
		return finalPath;
	}
	
	std::unique_ptr<NavGraph> GraphCopy = pNavGraph->Clone();
	
	int const startNodeId = GraphCopy->AddNode(std::make_unique<NavGraphNode>(outStartPos, -1));
	int const endNodeId = GraphCopy->AddNode(std::make_unique<NavGraphNode>(outEndPos, -1));

	ConnectNodeToTriangle(GraphCopy.get(), startTriangle, startNodeId);
	ConnectNodeToTriangle(GraphCopy.get(), endTriangle, endNodeId);

	//Run A* on the augmented graph
	AStar astar{ GraphCopy.get(), HeuristicFunctions::Euclidean };
	std::vector<Node*> const pathNodes = astar.FindPath(GraphCopy->GetNode(startNodeId).get(), GraphCopy->GetNode(endNodeId).get());

	//Debug Visualisation
	debugNodePositions.clear();
	debugNodePositions.reserve(pathNodes.size());
	finalPath.reserve(pathNodes.size());
	for (Node const* pNode : pathNodes)
	{
		if (!pNode)
		{
			continue;
		}

		FVector2D const position = pNode->GetPosition();
		debugNodePositions.push_back(position);
		finalPath.push_back(position);
	}

	// Extra: Run optimiser on new graph (First check if everything works without SSFA!)
	 debugPortals = SSFA::FindPortals(pathNodes, *pNavGraph->GetNavPolygon());
	 finalPath = SSFA::OptimizePortals(debugPortals, *pNavGraph->GetNavPolygon());
	
	return finalPath;
}

std::vector<FVector2D> NavMeshPathfinding::FindPath(const FVector2D& startPos, const FVector2D& endPos, NavGraph* const pNavGraph)
{
	std::vector<FVector2D> debugNodePositions{};
	std::vector<NavLine> debugPortals{};

	return FindPath(startPos, endPos, pNavGraph, debugNodePositions, debugPortals);
}
