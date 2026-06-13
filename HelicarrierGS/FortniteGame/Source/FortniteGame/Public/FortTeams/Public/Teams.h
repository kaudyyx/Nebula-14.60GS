#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class Teams
{
public:
	static void RebootingDelegate(ABuildingGameplayActorSpawnMachine* RebootVan);
	static void ServerReviveFromDBNO(AFortPlayerPawnAthena* Pawn, AFortPlayerControllerAthena* EventInstigator, AFortAthenaAIBotController* BotController);
	static void FinishResurrection(ABuildingGameplayActorSpawnMachine* SpawnMachine, int SquadId);
	static void InitTeams();
};