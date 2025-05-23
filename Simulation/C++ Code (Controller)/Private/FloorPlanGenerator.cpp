// Fill out your copyright notice in the Description page of Project Settings.


#include "FloorPlanGenerator.h"
#include "StaticMeshAttributes.h"
#include "MeshDescription.h"
#include "StaticMeshDescription.h"
#include "MeshConversionModule.h"
#include "Engine/StaticMeshActor.h"
#include "DynamicMesh/MeshNormals.h"
#include "PhysicsEngine/BodySetup.h"


// Sets default values
AFloorPlanGenerator::AFloorPlanGenerator()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
    initialized = false;
}

void AFloorPlanGenerator::initialize()
{
    // Define the 2D vertices of the polygon
    TArray<FVector> PolygonVertices = {
        FVector(0.0f, 0.0f, 1000.0f),
        FVector(1000.0f, 0.0f, 1000.0f),
        FVector(1000.0f, 1000.0f, 1000.0f),
        FVector(0.0f, 1000.0f, 1000.0f)
    };

    TArray<FVector> PolygonVertices2 = {
        FVector(0.0f, 0.0f, 1000.0f),
        FVector(1000.0f, 0.0f, 1000.0f),
        FVector(1000.0f, 1000.0f, 1000.0f),
        FVector(0.0f, 1000.0f, 1000.0f)
    };

    //makeWall(PolygonVertices, 1000);
    makeWall(PolygonVertices, 400);
    makeDoorwayWall(PolygonVertices2, 1000, 700);
    //makeFloor(PolygonVertices);
}

AStaticMeshActor* AFloorPlanGenerator::makeStaticMesh(UE::Geometry::FDynamicMesh3 DynamicMesh) {
    // Step 1. Convert the dynamic mesh to a MeshDescription.
    FMeshDescription MeshDescription;
    FStaticMeshAttributes MeshAttributes(MeshDescription);
    MeshAttributes.Register();  // Register standard attributes (vertex positions, normals, UVs, etc.)

    // Use the built-in converter from GeometryProcessing.
    FDynamicMeshToMeshDescription Converter;
    Converter.Convert(&DynamicMesh, MeshDescription);

    // Step 2. Create a new UStaticMesh object.
    UStaticMesh* NewStaticMesh = NewObject<UStaticMesh>(GetTransientPackage(), UStaticMesh::StaticClass(), NAME_None, RF_Public | RF_Standalone);

    // Initialize the static mesh’s internal resources.
    NewStaticMesh->InitResources();

    // Build the static mesh from the MeshDescription. BuildFromMeshDescriptions() takes an array of FMeshDescription pointers,
    // which allows you to build a multi-section static mesh if desired.
    TArray<const FMeshDescription*> MeshDescriptions;
    MeshDescriptions.Add(&MeshDescription);
    NewStaticMesh->BuildFromMeshDescriptions(MeshDescriptions);

    // Enable collision for the new static mesh
    NewStaticMesh->CreateBodySetup();  // Ensure there's a BodySetup to handle collisions
    NewStaticMesh->GetBodySetup()->CollisionTraceFlag = ECollisionTraceFlag::CTF_UseComplexAsSimple;  // Use mesh geometry for collision

    // Update physics meshes for collision detection
    NewStaticMesh->GetBodySetup()->CreatePhysicsMeshes();

    // Perform post-build updates (this updates collision, LODs, etc.)
    NewStaticMesh->PostEditChange();

    FActorSpawnParameters SpawnParams;
    AStaticMeshActor* MeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
    if (MeshActor && MeshActor->GetStaticMeshComponent()) {
        MeshActor->GetStaticMeshComponent()->SetStaticMesh(NewStaticMesh);

        MeshActor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); // Enable both queries (traces) and physics
        MeshActor->GetStaticMeshComponent()->SetCollisionObjectType(ECollisionChannel::ECC_WorldStatic); // Treat as static world object
        MeshActor->GetStaticMeshComponent()->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Block); // Block everything

        // Load a default material from the engine's basic shapes
        UMaterial* DefaultMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("Material'/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial'")
        );

        // If the material was successfully loaded, assign it to the mesh component.
        if (DefaultMaterial)
        {
            MeshActor->GetStaticMeshComponent()->SetMaterial(0, DefaultMaterial);
        }
        else {
            GEngine->AddOnScreenDebugMessage(-1, 15.0f, FColor::Red, "Material Error");
        }
    }
    return MeshActor;
}

void AFloorPlanGenerator::makeWindowWall(TArray<FVector> PolygonVertices, double height, double windowHeight, double windowEndHeight)
{
}

void AFloorPlanGenerator::makeDoorwayWall(TArray<FVector> PolygonVertices, double height, double doorHeight)
{
    double new_height = height-doorHeight;
    TArray<FVector> adjustedVertices;
    for (const FVector& Vertex : PolygonVertices) {
        FVector ModifiedVertex = Vertex;
        ModifiedVertex.Z += height;
        adjustedVertices.Add(ModifiedVertex);
    }
    makeWall(adjustedVertices,new_height);
    
}

void AFloorPlanGenerator::makeWall(TArray<FVector> PolygonVertices, double height)
{
    int32 VerticeCount = PolygonVertices.Num();
    
    // Initialize the dynamic mesh
    UE::Geometry::FDynamicMesh3 DynamicMesh;
    DynamicMesh.EnableAttributes();

    // Add floor vertices to the mesh
    for (const FVector& Vertex : PolygonVertices) {
        DynamicMesh.AppendVertex(FVector3d(Vertex));
    }

    // ceiling vertices (doing it as 2 seperate loops here important for how i reference)
    for (const FVector& Vertex : PolygonVertices) {
        FVector ModifiedVertex = Vertex;
        ModifiedVertex.Z += height;
        DynamicMesh.AppendVertex(FVector3d(ModifiedVertex));
    }

    for (int32 i = 0; i < VerticeCount; i++) {
        //DynamicMesh.AppendTriangle(PolygonIndices[i], PolygonIndices[i + 1], PolygonIndices[i + 2]);
        if (i != VerticeCount - 1) {
            DynamicMesh.AppendTriangle(i, i + VerticeCount, i + 1);
            DynamicMesh.AppendTriangle(i + 1, i + VerticeCount, i + 1 + VerticeCount);
        } else {
            DynamicMesh.AppendTriangle(i, i + VerticeCount, 0);
            DynamicMesh.AppendTriangle(0, i + VerticeCount, 0 + VerticeCount);
        }
    }

    TArray<FVector> FirstNVertices;
    TArray<FVector> RemainingVertices;

    for (int32 i = 0; i < VerticeCount && i < PolygonVertices.Num(); ++i) {
        FirstNVertices.Add(PolygonVertices[i]);
    }
    
    for (int32 i = VerticeCount; i < DynamicMesh.VertexCount(); ++i) {
        FVector v= DynamicMesh.GetVertexRef(i);
        RemainingVertices.Add(v);
    }
    makeCeiling(FirstNVertices);
    makeFloor(RemainingVertices);

    StaticAssets.Add(makeStaticMesh(DynamicMesh));
}

void AFloorPlanGenerator::makeFloor(TArray<FVector> PolygonVertices)
{
    int32 VerticeCount = PolygonVertices.Num();

    // Initialize the dynamic mesh
    UE::Geometry::FDynamicMesh3 DynamicMesh;
    DynamicMesh.EnableAttributes();

    // Add floor vertices to the mesh
    for (const FVector& Vertex : PolygonVertices) {
        DynamicMesh.AppendVertex(FVector3d(Vertex));
    }

    for (int32 i = 0; i < VerticeCount; i+=2) {
        if(i + 2 < VerticeCount)
            DynamicMesh.AppendTriangle(i, i + 2, i + 1);
        else
            DynamicMesh.AppendTriangle(i, 0, i + 1);
    }

    StaticAssets.Add(makeStaticMesh(DynamicMesh));
}

void AFloorPlanGenerator::makeCeiling(TArray<FVector> PolygonVertices)
{
    int32 VerticeCount = PolygonVertices.Num();

    // Initialize the dynamic mesh
    UE::Geometry::FDynamicMesh3 DynamicMesh;
    DynamicMesh.EnableAttributes();

    // Add floor vertices to the mesh
    for (const FVector& Vertex : PolygonVertices) {
        DynamicMesh.AppendVertex(FVector3d(Vertex));
    }

    for (int32 i = 0; i < VerticeCount; i += 2) {
        if (i + 2 < VerticeCount)
            DynamicMesh.AppendTriangle(i, i + 1, i + 2);
        else
            DynamicMesh.AppendTriangle(i, i + 1, 0);
    }

    StaticAssets.Add(makeStaticMesh(DynamicMesh));
}

// Called when the game starts or when spawned
void AFloorPlanGenerator::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFloorPlanGenerator::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

