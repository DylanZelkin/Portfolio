// Fill out your copyright notice in the Description page of Project Settings.


#include "EnvironmentObjectActor.h"
#include "Engine/StaticMeshActor.h" 

// Sets default values
AEnvironmentObjectActor::AEnvironmentObjectActor()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	initialized = false;
}

// Called when the game starts or when spawned
void AEnvironmentObjectActor::BeginPlay()
{
	Super::BeginPlay();
	
}

void GetMeshData(UWorld* World, UStaticMesh* Mesh)
{
    if (!World || !Mesh) return;

    // Check if the mesh has LOD data
    if (!(Mesh->GetRenderData() && Mesh->GetRenderData()->LODResources.Num() > 0)) {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Mesh has no LOD data!"));
        return;
    }

    const FStaticMeshLODResources& LODResource = Mesh->GetRenderData()->LODResources[0];
    const FPositionVertexBuffer& VertexBuffer = LODResource.VertexBuffers.PositionVertexBuffer;
    const FIndexArrayView Indices = LODResource.IndexBuffer.GetArrayView();

    TArray<TSharedPtr<FJsonValue>> VertexArray;
    TArray<TSharedPtr<FJsonValue>> TriangleArray;

    // Get vertex positions
    int32 VertexCount = VertexBuffer.GetNumVertices();
    for (int32 i = 0; i < VertexCount; i++)
    {
        FVector VertexPosition = FVector(VertexBuffer.VertexPosition(i));
        /*GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Cyan,
            FString::Printf(TEXT("Vertex %d: %s"), i, *VertexPosition.ToString()));*/

        // Convert vertex to JSON
        TSharedPtr<FJsonObject> VertexJson = MakeShareable(new FJsonObject());
        VertexJson->SetNumberField("X", VertexPosition.X);
        VertexJson->SetNumberField("Y", VertexPosition.Y);
        VertexJson->SetNumberField("Z", VertexPosition.Z);
        VertexArray.Add(MakeShareable(new FJsonValueObject(VertexJson)));
    }

    // Get triangles
    for (int32 i = 0; i < Indices.Num(); i += 3)
    {
        int32 Index0 = Indices[i];
        int32 Index1 = Indices[i + 1];
        int32 Index2 = Indices[i + 2];

        /*GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow,
            FString::Printf(TEXT("Triangle: %d, %d, %d"), Index0, Index1, Index2));*/

        // Convert triangle to JSON
        TSharedPtr<FJsonObject> TriangleJson = MakeShareable(new FJsonObject());
        TriangleJson->SetNumberField("V0", Index0);
        TriangleJson->SetNumberField("V1", Index1);
        TriangleJson->SetNumberField("V2", Index2);
        TriangleArray.Add(MakeShareable(new FJsonValueObject(TriangleJson)));
    }

    // Create JSON object
    TSharedPtr<FJsonObject> MeshDataJson = MakeShareable(new FJsonObject());
    MeshDataJson->SetArrayField("Vertices", VertexArray);
    MeshDataJson->SetArrayField("Triangles", TriangleArray);

    // Serialize to JSON string
    FString OutputString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&OutputString);
    FJsonSerializer::Serialize(MeshDataJson.ToSharedRef(), Writer);

    // Save JSON to file
    FString FilePath = FPaths::ProjectDir() + TEXT("MeshData.json");
    if (FFileHelper::SaveStringToFile(OutputString, *FilePath)) {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Mesh data saved to MeshData.json"));
    } else {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("Failed to save MeshData.json"));
    }
}

// Called every frame
void AEnvironmentObjectActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	if (!initialized) {
		initialized = true;

		//FString MeshPath = TEXT("/Game/OWD_Fruit_Vegetable/Meshes/SM_Apple.SM_Apple");
		//UStaticMesh* Mesh = Cast<UStaticMesh>(StaticLoadObject(UStaticMesh::StaticClass(), nullptr, *MeshPath));;
		//if (!Mesh) return;

		//FActorSpawnParameters SpawnParams;
		//AStaticMeshActor* MeshActor = GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), FVector(0, 0, 100), FRotator::ZeroRotator, SpawnParams);
		//if (!MeshActor) return;

		//UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
		//if (!MeshComp) return;

  //      MeshComp->SetMobility(EComponentMobility::Movable);
		//MeshComp->SetStaticMesh(Mesh);  // Assign the loaded mesh

  //      GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Green, TEXT("Mesh made!"));
	
  //      GetMeshData(GetWorld(), Mesh);
    }
}

