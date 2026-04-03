#pragma once
#include <stack>
#include "Shared/Graph/Graph.h"

namespace GameAI
{
	enum class Eulerianity
	{
		notEulerian,
		semiEulerian,
		eulerian,
	};

	class EulerianPath final
	{
	public:
		EulerianPath(Graph* const pGraph);

		Eulerianity IsEulerian() const;
		std::vector<Node*> FindPath(Eulerianity& eulerianity) const;

	private:
		void VisitAllNodesDFS(const std::vector<Node*>& pNodes, std::vector<bool>& visited, int startIndex) const;
		bool IsConnected() const;

		Graph* m_pGraph;
	};

	inline EulerianPath::EulerianPath(Graph* const pGraph)
		: m_pGraph(pGraph)
	{
	}

	inline Eulerianity EulerianPath::IsEulerian() const
	{
		// If the graph is not connected, there can be no Eulerian Trail
		if (!IsConnected())
			return Eulerianity::notEulerian;
		if (m_pGraph->GetConnections().size() < 1)
			return Eulerianity::notEulerian;
		// TODO Count nodes with odd degree 
		int oddNodes = 0;
		for (auto& Node : m_pGraph->GetNodes())
		{
			if (Node->GetId() == Graphs::InvalidNodeId) continue;
			if (m_pGraph->FindConnectionsFrom(Node->GetId()).size() % 2 == 1)
			{
				oddNodes++;
			}
			
		}
		// TODO A connected graph with more than 2 nodes with an odd degree (an odd amount of connections) is not Eulerian
		if (oddNodes > 2)
		{
			return Eulerianity::notEulerian;
		}
		// TODO A connected graph with exactly 2 nodes with an odd degree is Semi-Eulerian (unless there are only 2 nodes)
		if (oddNodes == 2)
		{
			return Eulerianity::semiEulerian;
		}
		// TODO An Euler trail can be made, but only starting and ending in these 2 nodes
		// TODO A connected graph with no odd nodes is Eulerian
		if (oddNodes == 0)
		{
			return Eulerianity::eulerian;
		}
		
		return Eulerianity::notEulerian;
	}

	inline std::vector<Node*> EulerianPath::FindPath(Eulerianity& eulerianity) const
	{
		// Get a copy of the graph because this algorithm involves removing edges
		Graph graphCopy = m_pGraph->Clone();
		std::vector<Node*> Path = {};
		std::vector<Node*> Nodes = graphCopy.GetActiveNodes();
		int currentNodeId{ Graphs::InvalidNodeId };
		
		// TODO Check if there can be an Euler path
		// TODO If this graph is not eulerian, return the empty path
		//auto euler = IsEulerian();
		switch (eulerianity)
		{
		case Eulerianity::notEulerian :
			{
				return Path;
			}
		case Eulerianity::semiEulerian :
			{
				for (auto node : Nodes)
				{
					if (graphCopy.FindConnectionsFrom(node->GetId()).size() % 2 == 1)
					{
						currentNodeId = node->GetId();
						break;
					}
				}
				break;
			}
		case Eulerianity::eulerian :
			{
				for (auto node : Nodes)
				{
					if (!graphCopy.FindConnectionsFrom(node->GetId()).empty())
					{
						currentNodeId = node->GetId();
						break;
					}
				}
				break;
			}
		default :
			{
				return Path;
			}
		}
		if (currentNodeId == Graphs::InvalidNodeId)
			return Path;
		// TODO Start algorithm loop
		std::stack<int> nodeStack;
		std::vector<int> pathIds{};

		while (!nodeStack.empty() || !graphCopy.FindConnectionsFrom(currentNodeId).empty())
		{
			if (graphCopy.FindConnectionsFrom(currentNodeId).empty())
			{
				pathIds.emplace_back(currentNodeId);
				currentNodeId = nodeStack.top();
				nodeStack.pop();
			}
			else
			{
				nodeStack.push(currentNodeId);
				auto* connection = graphCopy.FindConnectionsFrom(currentNodeId)[0];
				const int nextNodeId = connection->GetToId();
				graphCopy.RemoveConnection(currentNodeId, nextNodeId);
				currentNodeId = nextNodeId;
			}
		}
		pathIds.emplace_back(currentNodeId);

		Path.reserve(pathIds.size());
		for (int nodeId : pathIds)
		{
			if (nodeId < 0 || nodeId >= static_cast<int>(m_pGraph->GetNodes().size()))
				continue;
			auto& nodePtr = m_pGraph->GetNode(nodeId);
			if (nodePtr && nodePtr->GetId() != Graphs::InvalidNodeId)
			{
				Path.emplace_back(nodePtr.get());
			}
		}

		return Path;
	}

	inline void EulerianPath::VisitAllNodesDFS(const std::vector<Node*>& Nodes, std::vector<bool>& visited, int startIndex ) const
	{
		// TODO Mark the visited node
		auto startNode = Nodes[startIndex];
		
		//if (!visited[startNode->GetId()])
		visited[startIndex] = true;
		// TODO Ask the graph for the connections from that node
		std::vector<Connection*> Connections = m_pGraph->FindConnectionsFrom(startNode->GetId());
		for (auto Connection : Connections)
		{
			if (!visited[Connection->GetToId()])
			{
				VisitAllNodesDFS(Nodes, visited, Connection->GetToId());
			}
		}
		// TODO recursively visit any valid connected nodes that were not visited before
		// TODO Tip: use an index-based for-loop to find the correct index
	}

	inline bool EulerianPath::IsConnected() const
	{
		std::vector<Node*> Nodes = m_pGraph->GetActiveNodes();
		if (Nodes.size() == 0)
			return false;

		// TODO choose a starting node
		Node* startNode{};
		for (int i = 0; i < Nodes.size(); ++i)
		{
			if (startNode == nullptr)
			{
				if (m_pGraph->FindConnectionsFrom(Nodes[i]->GetId()).size() > 0)
				{
					startNode = Nodes[i];
				}
			}
		}
		std::vector<bool> visited(Nodes.size(), false);
		// TODO start a depth-first-search traversal from the node that has at least one connection
		VisitAllNodesDFS(Nodes, visited, startNode->GetId());
		// TODO if a node was never visited, this graph is not connected
		for (auto node : Nodes)
		{
			if (!visited[node->GetId()])
			{
				return false;
			}
		}
		return true;
		
	}
}
