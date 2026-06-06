// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Shared/Level_Base.h"
#include "Shared/Graph/Graph.h"
#include "Shared/Graph/GraphEditorComponent.h"
#include "Shared/Graph/GraphNodeFactory.h"
#include "Level_GraphColoring.generated.h"

UCLASS()
class GAMEAIPROG_API ALevel_GraphColoring : public ALevel_Base
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GraphEditor")
	TSubclassOf<UGraphEditorComponent> GraphEditorClass;

	ALevel_GraphColoring();

	virtual void Tick(float DeltaTime) override;

protected:
	virtual void BeginPlay() override;

private:
	GameAI::Graph Graph{false};
	GameAI::GraphNodeFactory<GameAI::Node> NodeFactory{};
	TArray<FColor> Palette{};
	TArray<int> NodeColors{};
	int AmountOfColorsUsed{};
	bool bHasValidColoring{};

	UPROPERTY()
	UGraphEditorComponent* PlayerGraphEditor{};

	void BuildAssignmentGraph();
	void SolveGraphColoring();
	bool TryColorWithAmount(int AmountOfColors, TArray<int>& OutColors) const;
	bool BacktrackColorNode(int NodeOrderIndex, int AmountOfColors, TArray<int>& Colors, const TArray<int>& NodeOrder) const;
	bool CanUseColor(int NodeId, int ColorIndex, const TArray<int>& Colors) const;
	bool IsValidColoring() const;
	void RenderAssignmentGraph() const;
	void RenderAssignmentUi() const;
};
