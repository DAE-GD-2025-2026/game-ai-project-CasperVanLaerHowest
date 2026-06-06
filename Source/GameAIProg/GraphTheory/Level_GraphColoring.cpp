// Fill out your copyright notice in the Description page of Project Settings.

#include "Level_GraphColoring.h"

#include "DrawDebugHelpers.h"
#include "Shared/GameAISpectator.h"

ALevel_GraphColoring::ALevel_GraphColoring()
{
	PrimaryActorTick.bCanEverTick = true;
}

void ALevel_GraphColoring::BeginPlay()
{
	Super::BeginPlay();

	if (PlayerController = Cast<APlayerController>(GetWorld()->GetFirstLocalPlayerFromController()->PlayerController);
		GraphEditorClass && PlayerController)
	{
		PlayerGraphEditor = NewObject<UGraphEditorComponent>(PlayerController->GetPawn(), GraphEditorClass);
		PlayerGraphEditor->RegisterComponent();
		PlayerGraphEditor->SetEditedGraph(&Graph);
		PlayerGraphEditor->SetNodeFactory(&NodeFactory);
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("Unable to create graph coloring editor. Check PlayerController and GraphEditorClass."));
	}

	if (PlayerController)
	{
		if (AGameAISpectator* Player = Cast<AGameAISpectator>(PlayerController->GetPawnOrSpectator()); Player)
		{
			Player->SetCameraProjection(ECameraProjectionMode::Orthographic);
		}
	}

	Palette.Reset();
	Palette.Add(FColor{40, 70, 255});
	Palette.Add(FColor{240, 55, 45});
	Palette.Add(FColor{235, 235, 55});
	Palette.Add(FColor{90, 230, 65});
	Palette.Add(FColor{245, 120, 35});
	Palette.Add(FColor{190, 70, 230});

	BuildAssignmentGraph();
	SolveGraphColoring();
}

void ALevel_GraphColoring::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (PlayerGraphEditor && PlayerGraphEditor->HasGraphUpdated())
	{
		SolveGraphColoring();
	}

	RenderAssignmentUi();
	RenderAssignmentGraph();
}

void ALevel_GraphColoring::BuildAssignmentGraph()
{
	auto AddNode = [this](float X, float Y)
	{
		Graph.AddNode(std::make_unique<GameAI::Node>(FVector2D{X, Y}));
	};

	AddNode(120.f, 0.f);
	AddNode(-80.f, -115.f);
	AddNode(-185.f, 90.f);
	AddNode(-40.f, 160.f);
	AddNode(170.f, 185.f);
	AddNode(310.f, 70.f);
	AddNode(300.f, -115.f);
	AddNode(125.f, -235.f);
	AddNode(-95.f, -205.f);
	AddNode(-250.f, -115.f);
	AddNode(-245.f, 230.f);
	AddNode(-165.f, 350.f);
	AddNode(100.f, 400.f);
	AddNode(320.f, 260.f);
	AddNode(410.f, -10.f);
	AddNode(295.f, -265.f);
	AddNode(70.f, -370.f);
	AddNode(-225.f, -290.f);
	AddNode(-415.f, -110.f);
	AddNode(-400.f, 190.f);

	auto AddEdge = [this](int FromLabel, int ToLabel)
	{
		Graph.AddConnection(FromLabel - 1, ToLabel - 1);
	};

	AddEdge(1, 2);
	AddEdge(1, 4);
	AddEdge(1, 5);
	AddEdge(1, 6);
	AddEdge(1, 7);
	AddEdge(1, 8);
	AddEdge(2, 3);
	AddEdge(2, 9);
	AddEdge(2, 10);
	AddEdge(3, 4);
	AddEdge(3, 10);
	AddEdge(3, 11);
	AddEdge(4, 5);
	AddEdge(4, 11);
	AddEdge(4, 12);
	AddEdge(5, 6);
	AddEdge(5, 7);
	AddEdge(5, 13);
	AddEdge(5, 14);
	AddEdge(6, 7);
	AddEdge(6, 14);
	AddEdge(6, 15);
	AddEdge(6, 16);
	AddEdge(7, 8);
	AddEdge(7, 15);
	AddEdge(7, 16);
	AddEdge(8, 9);
	AddEdge(8, 16);
	AddEdge(8, 17);
	AddEdge(9, 10);
	AddEdge(9, 18);
	AddEdge(10, 18);
	AddEdge(10, 19);
	AddEdge(11, 12);
	AddEdge(11, 20);
	AddEdge(12, 13);
	AddEdge(12, 20);
	AddEdge(13, 14);
	AddEdge(16, 17);
	AddEdge(17, 18);
	AddEdge(18, 19);
	AddEdge(19, 20);
}

void ALevel_GraphColoring::SolveGraphColoring()
{
	AmountOfColorsUsed = 0;
	bHasValidColoring = false;

	TArray<int> Colors;
	Colors.Init(-1, Graph.GetNodes().size());

	for (int AmountOfColors = 1; AmountOfColors <= Palette.Num(); ++AmountOfColors)
	{
		if (TryColorWithAmount(AmountOfColors, Colors))
		{
			AmountOfColorsUsed = AmountOfColors;
			NodeColors = Colors;
			bHasValidColoring = IsValidColoring();
			return;
		}
	}

	NodeColors = Colors;
}

bool ALevel_GraphColoring::TryColorWithAmount(int AmountOfColors, TArray<int>& OutColors) const
{
	OutColors.Init(-1, Graph.GetNodes().size());

	TArray<int> NodeOrder;
	NodeOrder.Reserve(Graph.GetNodeCount());
	for (const std::unique_ptr<GameAI::Node>& Node : Graph.GetNodes())
	{
		if (Node && Node->GetId() != GameAI::Graphs::InvalidNodeId)
		{
			NodeOrder.Add(Node->GetId());
		}
	}

	NodeOrder.Sort([this](int LeftId, int RightId)
	{
		int LeftDegree{};
		int RightDegree{};
		for (const std::unique_ptr<GameAI::Connection>& Edge : Graph.GetConnections())
		{
			if (Edge->GetFromId() == LeftId || Edge->GetToId() == LeftId)
			{
				++LeftDegree;
			}
			if (Edge->GetFromId() == RightId || Edge->GetToId() == RightId)
			{
				++RightDegree;
			}
		}
		return LeftDegree > RightDegree;
	});

	return BacktrackColorNode(0, AmountOfColors, OutColors, NodeOrder);
}

bool ALevel_GraphColoring::BacktrackColorNode(
	int NodeOrderIndex,
	int AmountOfColors,
	TArray<int>& Colors,
	const TArray<int>& NodeOrder) const
{
	if (NodeOrderIndex >= NodeOrder.Num())
	{
		return true;
	}

	const int CurrentNodeId = NodeOrder[NodeOrderIndex];
	for (int ColorIndex = 0; ColorIndex < AmountOfColors; ++ColorIndex)
	{
		if (!CanUseColor(CurrentNodeId, ColorIndex, Colors))
		{
			continue;
		}

		Colors[CurrentNodeId] = ColorIndex;
		if (BacktrackColorNode(NodeOrderIndex + 1, AmountOfColors, Colors, NodeOrder))
		{
			return true;
		}
		Colors[CurrentNodeId] = -1;
	}

	return false;
}

bool ALevel_GraphColoring::CanUseColor(int NodeId, int ColorIndex, const TArray<int>& Colors) const
{
	for (const std::unique_ptr<GameAI::Connection>& Edge : Graph.GetConnections())
	{
		const bool bTouchesFrom = Edge->GetFromId() == NodeId;
		const bool bTouchesTo = Edge->GetToId() == NodeId;
		if (!bTouchesFrom && !bTouchesTo)
		{
			continue;
		}

		const int OtherNodeId = bTouchesFrom ? Edge->GetToId() : Edge->GetFromId();
		if (Colors.IsValidIndex(OtherNodeId) && Colors[OtherNodeId] == ColorIndex)
		{
			return false;
		}
	}

	return true;
}

bool ALevel_GraphColoring::IsValidColoring() const
{
	for (const std::unique_ptr<GameAI::Connection>& Edge : Graph.GetConnections())
	{
		const int FromId = Edge->GetFromId();
		const int ToId = Edge->GetToId();
		if (!NodeColors.IsValidIndex(FromId) || !NodeColors.IsValidIndex(ToId))
		{
			return false;
		}

		if (NodeColors[FromId] < 0 || NodeColors[FromId] == NodeColors[ToId])
		{
			return false;
		}
	}

	return true;
}

void ALevel_GraphColoring::RenderAssignmentGraph() const
{
	constexpr float DrawHeight = 35.f;
	constexpr float NodeRadius = 34.f;
	const FColor EdgeColor{150, 170, 200};

	for (const std::unique_ptr<GameAI::Connection>& Edge : Graph.GetConnections())
	{
		if (!Graph.GetIsDirectional() && Edge->GetFromId() > Edge->GetToId())
		{
			continue;
		}

		const FVector Start{Graph.GetNode(Edge->GetFromId())->GetPosition(), DrawHeight};
		const FVector End{Graph.GetNode(Edge->GetToId())->GetPosition(), DrawHeight};
		DrawDebugLine(GetWorld(), Start, End, EdgeColor, false, -1.f, 0, 5.f);
	}

	for (const std::unique_ptr<GameAI::Node>& Node : Graph.GetNodes())
	{
		if (!Node || Node->GetId() == GameAI::Graphs::InvalidNodeId)
		{
			continue;
		}

		const int ColorIndex = NodeColors.IsValidIndex(Node->GetId()) ? NodeColors[Node->GetId()] : -1;
		const FColor NodeColor = Palette.IsValidIndex(ColorIndex)
			? Palette[ColorIndex]
			: FColor{180, 180, 180};
		const FVector Location{Node->GetPosition(), DrawHeight + 8.f};

		DrawDebugSphere(GetWorld(), Location, NodeRadius, 24, NodeColor, false, -1.f, 0, 5.f);
		DrawDebugString(
			GetWorld(),
			Location + FVector{14.f, 0.f, 26.f},
			FString::Printf(TEXT("%d"), Node->GetId() + 1),
			nullptr,
			FColor::White,
			0.f,
			false,
			1.35f);
	}
}

void ALevel_GraphColoring::RenderAssignmentUi() const
{
	bool WindowActive = true;
	ImGui::SetNextWindowPos(WindowPos);
	ImGui::SetNextWindowSize(WindowSize);
	ImGui::Begin("Gameplay Programming", &WindowActive, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
	ImGui::SetWindowFocus();
	ImGui::PushItemWidth(70);

	ImGui::Text("Graph Coloring");
	ImGui::Spacing();
	ImGui::TextWrapped("Color a graph using as few colors as possible while connected nodes never have the same color.");

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("RESULT");
	ImGui::Indent();
	ImGui::Text("Nodes: %d", Graph.GetNodeCount());
	ImGui::Text("Edges: %d", static_cast<int>(Graph.GetConnections().size() / (Graph.GetIsDirectional() ? 1 : 2)));
	ImGui::Text("Colors used: %d", AmountOfColorsUsed);
	ImGui::Text("Valid: %s", bHasValidColoring ? "yes" : "no");
	ImGui::Unindent();

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	ImGui::Text("STATS");
	ImGui::Indent();
	ImGui::Text("%.3f ms/frame", 1000.0f / ImGui::GetIO().Framerate);
	ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
	ImGui::Unindent();

	ImGui::End();
}
