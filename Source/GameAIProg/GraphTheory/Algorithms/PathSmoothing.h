#pragma once
#include <algorithm>
#include <vector>

#include "Movement/Pathfinding/Navmesh/TriPolygon.h"
#include "NavGraphPathfinding.h"
#include "Shared/Graph/Graph.h"
#include "Shared/Graph/NavGraph/NavGraphNode.h"

namespace GameAI
{
class SSFA final
{
public:
	//=== SSFA Functions ===
	//--- References ---
	//http://digestingduck.blogspot.be/2010/03/simple-stupid-funnel-algorithm.html
	//https://gamedev.stackexchange.com/questions/68302/how-does-the-simple-stupid-funnel-algorithm-work
	static std::vector<NavLine> FindPortals(std::vector<Node*> const& Path, TriPolygon const& NavPoly)
	{
		std::vector<NavLine> portals{};
		if (Path.empty())
		{
			return portals;
		}

		portals.reserve(Path.size() + 2);

		FVector2D const startPosition = Path.front() ? Path.front()->GetPosition() : FVector2D{};
		portals.push_back({startPosition, startPosition});

		for (int pathIndex = 0; pathIndex < static_cast<int>(Path.size()); ++pathIndex)
		{
			Node const* const pNode = Path[pathIndex];
			if (!pNode)
			{
				continue;
			}

			auto const* const pNavNode = dynamic_cast<NavGraphNode const*>(pNode);
			if (!pNavNode)
			{
				continue;
			}

			int const edgeIdx = pNavNode->GetEdgeIdx();
			if (edgeIdx < 0 || edgeIdx >= static_cast<int>(NavPoly.GetEdges().size()))
			{
				continue;
			}

			TriPolygon::Edge const& edge = NavPoly.GetEdges()[edgeIdx];
			FVector2D p1{edge.GetP1(NavPoly)};
			FVector2D p2{edge.GetP2(NavPoly)};

			FVector2D forward{};
			if (pathIndex + 1 < static_cast<int>(Path.size()) && Path[pathIndex + 1])
			{
				forward = Path[pathIndex + 1]->GetPosition() - pNode->GetPosition();
			}
			else if (pathIndex > 0 && Path[pathIndex - 1])
			{
				forward = pNode->GetPosition() - Path[pathIndex - 1]->GetPosition();
			}

			if (!forward.IsNearlyZero())
			{
				float const p1Side = FVector2D::CrossProduct(forward, p1 - pNode->GetPosition());
				float const p2Side = FVector2D::CrossProduct(forward, p2 - pNode->GetPosition());
				if (p1Side > p2Side)
				{
					std::swap(p1, p2);
				}
			}

			portals.push_back({p1, p2});
		}

		FVector2D const endPosition = Path.back() ? Path.back()->GetPosition() : startPosition;
		portals.push_back({endPosition, endPosition});

		return portals;
	}

	static std::vector<FVector2D> OptimizePortals(std::vector<NavLine> const& Portals, TriPolygon const& NavPoly)
	{
		std::vector<FVector2D> path{};
		if (Portals.empty())
		{
			return path;
		}

		(void)NavPoly;

		int const portalCount = static_cast<int>(Portals.size());
		int leftLegIndex = 0;
		int rightLegIndex = 0;

		FVector2D apex = Portals[0].P1;
		FVector2D leftLeg = Portals[0].P2 - apex;
		FVector2D rightLeg = Portals[0].P1 - apex;

		path.reserve(Portals.size());
		path.push_back(apex);

		int portalIdx = 1;
		while (portalIdx < portalCount)
		{
			NavLine const& portal = Portals[portalIdx];

			// --- RIGHT CHECK ---
			FVector2D const newRightLeg = portal.P1 - apex;
			if (FVector2D::CrossProduct(rightLeg, newRightLeg) >= 0.f) // inwards (CCW)
			{
				if (FVector2D::CrossProduct(leftLeg, newRightLeg) < 0.f) // no crossing over left leg
				{
					rightLeg = newRightLeg;
					rightLegIndex = portalIdx;
				}
				else // crossing over left leg -> left leg becomes new apex
				{
					apex += leftLeg;
					portalIdx = leftLegIndex + 1;
					leftLegIndex = portalIdx;
					rightLegIndex = portalIdx;
					path.push_back(apex);

					if (portalIdx < portalCount)
					{
						rightLeg = Portals[rightLegIndex].P1 - apex;
						leftLeg = Portals[leftLegIndex].P2 - apex;
					}
					continue;
				}
			}

			// --- LEFT CHECK ---
			FVector2D const newLeftLeg = portal.P2 - apex;
			if (FVector2D::CrossProduct(leftLeg, newLeftLeg) <= 0.f) // inwards (CW)
			{
				if (FVector2D::CrossProduct(rightLeg, newLeftLeg) > 0.f) // no crossing over right leg
				{
					leftLeg = newLeftLeg;
					leftLegIndex = portalIdx;
				}
				else // crossing over right leg -> right leg becomes new apex
				{
					apex += rightLeg;
					portalIdx = rightLegIndex + 1;
					leftLegIndex = portalIdx;
					rightLegIndex = portalIdx;
					path.push_back(apex);

					if (portalIdx < portalCount)
					{
						rightLeg = Portals[rightLegIndex].P1 - apex;
						leftLeg = Portals[leftLegIndex].P2 - apex;
					}
					continue;
				}
			}

			++portalIdx;
		}

		if (path.empty() || !path.back().Equals(Portals.back().P1))
		{
			path.push_back(Portals.back().P1);
		}

		return path;
	}

private:
	SSFA() = default;
	~SSFA() = default;
};
}
