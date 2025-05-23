// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "RemoteManagerActor.h"
#include "SimGameInstance.generated.h"
/**
 * 
 */
UCLASS()
class THESISSIMULATOR_API USimGameInstance : public UGameInstance
{
	GENERATED_BODY()
	
public:
	virtual void Init() override;
	virtual void Shutdown() override;

	ARemoteManagerActor* manager;
	

};
