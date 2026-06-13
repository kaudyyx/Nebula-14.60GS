#pragma once
#include "pch.h"

class FortGameModeAthena
{
public:
	DefHookOg(void, StartNewSafeZonePhase, AFortGameModeAthena* GameMode, int a2);
	static inline TArray<UFortAbilitySet*> AbilitySets;
	static bool ReadyToStartMatch(AFortGameModeAthena* GameMode);
	DefHookOg(APawn*, SpawnDefaultPawnFor, AGameModeBase*, AController*, AActor*);
	DefHookOg(void, OnAircraftExitedDropZone, AFortGameModeAthena* GM, AFortAthenaAircraft* Aircraft);
	static inline void (*HandleStartingNewPlayer)(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer);
	static void HandleStartingNewPlayerHook(AFortGameModeAthena* GameMode, AFortPlayerControllerAthena* NewPlayer);
	DefHookOg(bool, StartAircraftPhase, AFortGameModeAthena*, char);
	static void InitFortGameModeAthena();
};
