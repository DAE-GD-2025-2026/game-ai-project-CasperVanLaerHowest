#include "SpacePartitioning.h"
#include "DrawDebugHelpers.h"

// --- Cell ---
// ------------
Cell::Cell(float Left, float Bottom, float Width, float Height)
{
	BoundingBox.Min = { Left, Bottom };
	BoundingBox.Max = { BoundingBox.Min.X + Width, BoundingBox.Min.Y + Height };
}

std::vector<FVector2D> Cell::GetRectPoints() const
{
	const float left = BoundingBox.Min.X;
	const float bottom = BoundingBox.Min.Y;
	const float width = BoundingBox.Max.X - BoundingBox.Min.X;
	const float height = BoundingBox.Max.Y - BoundingBox.Min.Y;

	std::vector<FVector2D> rectPoints =
	{
		{ left , bottom  },
		{ left , bottom + height  },
		{ left + width , bottom + height },
		{ left + width , bottom  },
	};

	return rectPoints;
}

// --- Partitioned Space ---
// -------------------------
CellSpace::CellSpace(UWorld* pWorld, float Width, float Height, int Rows, int Cols, int MaxEntities)
	: pWorld{pWorld}
	, SpaceWidth{Width}
	, SpaceHeight{Height}
	, NrOfRows{Rows}
	, NrOfCols{Cols}
	, NrOfNeighbors{0}
{
	Neighbors.SetNum(MaxEntities);
	
	//calculate bounds of a cell
	CellWidth = Width / Cols;
	CellHeight = Height / Rows;

	// TODO create the cells
	for (int i{}; i < Rows; ++i)
	{
		for (int o{}; o < Cols; ++o)
		{
			Cells.emplace_back(((o * CellWidth) - (Width * 0.5f)), ((i * CellHeight) - (Height * 0.5f)), CellWidth, CellHeight);
		}
	}
}

void CellSpace::AddAgent(ASteeringAgent& Agent)
{
	// TODO Add the agent to the correct cell
	auto pos = Agent.GetPosition();
	auto index = PositionToIndex(pos);
	
	if (index == -1)
		return; // fail
	
	Cells[index].Agents.push_back(&Agent);
}

void CellSpace::UpdateAgentCell(ASteeringAgent& Agent, const FVector2D& OldPos)
{
	//TODO Check if the agent needs to be moved to another cell.
	//TODO Use the calculated index for oldPos and currentPos for this
	
	const FVector2D newPos = Agent.GetPosition();
	const int oldIndex = PositionToIndex(OldPos);
	const int newIndex = PositionToIndex(newPos);
	
	
	if (oldIndex != newIndex)
		UE_LOG(LogTemp, Warning, TEXT("Agent moved : %d -> %d"), oldIndex, newIndex);
	
	
	if (oldIndex == newIndex)
		return;
	
	if (oldIndex != -1)
		Cells[oldIndex].Agents.remove(&Agent);
	
	if (newIndex != -1)
		Cells[newIndex].Agents.push_back(&Agent);
	
	
	
}

void CellSpace::RegisterNeighbors(ASteeringAgent& Agent, float QueryRadius)
{
	// TODO Register the neighbors for the provided agent
	// TODO Only check the cells that are within the radius of the neighborhood
	
	NrOfNeighbors = 0;
	
	const FRect agentBox{
		{ Agent.GetPosition().X - QueryRadius, Agent.GetPosition().Y - QueryRadius },
		{ Agent.GetPosition().X + QueryRadius, Agent.GetPosition().Y + QueryRadius }
	};

	for (auto cell : Cells)
	{
		if (DoRectsOverlap(agentBox, cell.BoundingBox))
		{
			for (auto agent : cell.Agents)
			{
				const float distance = FVector::DistSquared(agent->GetActorLocation(), Agent.GetActorLocation());
		
				if (distance <= QueryRadius * QueryRadius)
				{
					if (NrOfNeighbors >= Neighbors.Num())
						return;
					Neighbors[NrOfNeighbors] = agent;
					NrOfNeighbors++;
				}
			}
		}
	}
}

void CellSpace::EmptyCells()
{
	for (Cell& c : Cells)
		c.Agents.clear();
}

void CellSpace::RenderCells() const
{
	// TODO Render the cells with the number of agents inside of it
	for (const Cell& c : Cells)
	{
		if (!pWorld)
			continue;

		const std::vector<FVector2D> rectPoints = c.GetRectPoints();
		if (rectPoints.size() < 4)
			continue;

		for (int i = 0; i < 4; ++i)
		{
			const FVector2D& start2D = rectPoints[i];
			const FVector2D& end2D = rectPoints[(i + 1) % 4];
			const FVector start{ start2D.X, start2D.Y, 0.f };
			const FVector end{ end2D.X, end2D.Y, 0.f };

			DrawDebugLine(pWorld, start, end, FColor::Green, false, -1.f, 0, 1.f);
		}

		const FVector2D center2D{
			(c.BoundingBox.Min.X + c.BoundingBox.Max.X) * 0.5f,
			(c.BoundingBox.Min.Y + c.BoundingBox.Max.Y) * 0.5f
		};
		const FVector textLocation{ center2D.X, center2D.Y, 10.f };
		const FString agentCountText = FString::Printf(TEXT("%d"), static_cast<int>(c.Agents.size()));
		DrawDebugString(pWorld, textLocation, agentCountText, nullptr, FColor::White, 0.f, false, 1.f);
	}
}

int CellSpace::PositionToIndex(FVector2D const & Pos) const
{
	// TODO Calculate the index of the cell based on the position
	
	for (int i{}; i < static_cast<int>(Cells.size()); ++i)
	{
		const Cell& c = Cells[i];
		if (Pos.X >= c.BoundingBox.Min.X && Pos.X <= c.BoundingBox.Max.X &&
			Pos.Y >= c.BoundingBox.Min.Y && Pos.Y <= c.BoundingBox.Max.Y)
		{
			return i;
		}
	}

	return -1;
}

bool CellSpace::DoRectsOverlap(FRect const & RectA, FRect const & RectB)
{
	// Check if the rectangles are separated on either axis
	if (RectA.Max.X < RectB.Min.X || RectA.Min.X > RectB.Max.X) return false;
	if (RectA.Max.Y < RectB.Min.Y || RectA.Min.Y > RectB.Max.Y) return false;
    
	// If they are not separated, they must overlap
	return true;
}
