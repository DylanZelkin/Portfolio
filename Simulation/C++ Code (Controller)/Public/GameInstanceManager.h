// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "CharacterManager.h"
#include "EnvironmentManagerActor.h"
#include "ConnectionManagerActor.h"
#include "GameInstanceManager.generated.h"

/**
 * 
 */
UCLASS()
class GAMEANIMATIONSAMPLE_API UGameInstanceManager : public UGameInstance
{
	GENERATED_BODY()

public:
	virtual void Init() override;

	//ACharacterManager* manager;
	AEnvironmentManagerActor* EnvironmentManager;
	AConnectionManagerActor* ConnectionManager;

};
