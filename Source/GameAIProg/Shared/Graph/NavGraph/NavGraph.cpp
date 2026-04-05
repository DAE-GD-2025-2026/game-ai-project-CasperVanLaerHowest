#include "NavGraph.h"

#include "NavGraphNode.h"

GameAI::NavGraph::NavGraph(std::unique_ptr<TriPolygon> && NavPoly)
	: Graph{false}
	, pNavPoly{std::move(NavPoly)}
{
	CreateNavigationGraph();
}

GameAI::NavGraph::NavGraph(const NavGraph& Other)
	: Graph(false)
	, pNavPoly{Other.pNavPoly ? std::make_unique<TriPolygon>(*Other.pNavPoly) : nullptr}
{
	Nodes.reserve(Other.Nodes.size());
	for (std::unique_ptr<Node> const & OtherNode : Other.Nodes)
	{
		Nodes.push_back(std::make_unique<NavGraphNode>(*dynamic_cast<NavGraphNode*>(OtherNode.get())));
	}
        
	Connections.reserve(Other.Connections.size());
	for (std::unique_ptr<Connection> const & OtherConnection : Other.Connections)
	{
		Connections.push_back(std::make_unique<Connection>(*OtherConnection.get()));
	}
}

std::unique_ptr<GameAI::NavGraph> GameAI::NavGraph::Clone() const
{
	return std::make_unique<NavGraph>(*this);
}

int GameAI::NavGraph::GetNodeIdFromEdgeIndex(int EdgeIdx) const
{
	if (EdgeIdx >= 0)
	{
		for (auto const & pNode : Nodes)
		{
			if (reinterpret_cast<NavGraphNode*>(pNode.get())->GetEdgeIdx() == EdgeIdx)
			{
				return pNode->GetId();
			}
		}
	}
	
	return Graphs::InvalidNodeId;
}

void GameAI::NavGraph::CreateNavigationGraph()
{
	//1. Go over all the edges of the navigation mesh and create nodes
	for (int index = 0; index < pNavPoly->GetEdges().size(); ++index)
	{
		auto const& edge = pNavPoly->GetEdges()[index];
		
		int count{};
		for (auto const& triangle : pNavPoly->GetTriangles())
		{
			if (triangle.HasEdge(edge))
			{
				count++;
			}
		}
		
		if (count < 2)
			continue;
		
		auto const midpoint = (edge.GetP1(*pNavPoly) + edge.GetP2(*pNavPoly)) * 0.5;
		AddNode(std::make_unique<NavGraphNode>(FVector2D{midpoint}, index));
	}
			// Create node here
	

	//2. Create connections now that every node is created	
		//2 valid nodes -> 1 connection
		//3 valid nodes -> 3 connections
	for (auto triangle : pNavPoly->GetTriangles())
	{
		std::vector<int> nodeIds{};

		for (auto edge : triangle.GetEdges())
		{
			int const edgeId = pNavPoly->FindEdgeIndex(edge).value_or(Graphs::InvalidNodeId);
			int const nodeId = GetNodeIdFromEdgeIndex(edgeId);
			if (nodeId == Graphs::InvalidNodeId)
				continue;
			
			nodeIds.emplace_back(nodeId);
		}
		
		if (nodeIds.size() < 2)
			continue;
		
		if (nodeIds.size() == 2)
		{
			AddConnection(nodeIds[0], nodeIds[1]);
		}
		
		if (nodeIds.size() == 3)
		{
			AddConnection(nodeIds[0], nodeIds[1]);
			AddConnection(nodeIds[1], nodeIds[2]);
			AddConnection(nodeIds[2], nodeIds[0]);
		}
	}
		
	//3. Set the connections cost to the actual distance
	SetConnectionCostsToDistances();
}
