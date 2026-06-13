#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>


class OnPossessedPawnDie
{
public:
	static inline void (*OnPossessedPawnDiedOG)(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AController* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);
	static void OnPossessedPawnDied(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum);

	static void InitOnPossessedPawnDie();
};