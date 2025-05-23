// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include <DynamicMeshToMeshDescription.h>
#include "FloorPlanGenerator.generated.h"

UCLASS()
class GAMEANIMATIONSAMPLE_API AFloorPlanGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFloorPlanGenerator();

	bool initialized;
	void initialize();

	TArray<AStaticMeshActor*> StaticAssets;
	AStaticMeshActor* makeStaticMesh(UE::Geometry::FDynamicMesh3 DynamicMesh);

	void makeWindowWall(TArray<FVector> PolygonVertices, double height, double windowHeight, double windowEndHeight);
	void makeDoorwayWall(TArray<FVector> PolygonVertices, double height, double doorHeight);
	void makeWall(TArray<FVector> PolygonVertices, double height);
	void makeFloor(TArray<FVector> PolygonVertices);
	void makeCeiling(TArray<FVector> PolygonVertices);

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;
};
