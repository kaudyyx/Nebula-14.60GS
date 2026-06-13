#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class Creative {
public:
    static void OnStopCallback(AFortProjectileBase* Base, FHitResult& Hit);
    DefHookOg(void, ServerCreativeSetFlightSpeedIndex, UObject*, FFrame&);
    static void EndGame(AFortMinigame* Minigame, AFortPlayerController* Controller, EFortMinigameEnd EndMethod);
    static void BeginPlay(AFortMinigameSettingsBuilding* Minigame);
    static void ClientBroadcastOnUpdateCreativePlotName(AFortPlayerControllerAthena* Controller, bool bSuccess, FText& Reason);
    static void ServerTeleportToPlaygroundLobbyIsland(AFortPlayerControllerAthena* Controller);
    static void TeleportPlayerToLinkedVolume(AFortAthenaCreativePortal* Portal, FFrame& Frame);
    static void MakeNewCreativePlot(UObject* Context, FFrame& StacK);
    static void UpdateCreativePlotData(AFortPlayerControllerAthena* Controler, AFortVolume* VolumeToPublish, FCreativeIslandInfo& MyInfo);
    static void UpdateCreativePlotName(UObject* Context, FFrame& Stack);
    static void UpdateCreativeIslandDescriptionTags(AFortPlayerControllerAthena* Controler, FString& IslandId, FString& Locale, TArray<class FString>& DescriptionTags);
    static void ServerLoadPlotForPortal(AFortPlayerControllerAthena* Controler, AFortAthenaCreativePortal* Portal, FString& PlotItemId);
    static void DestroyCreativePlot(AFortPlayerControllerAthena* Controller, FString& IslandId);
    static void DuplicateCreativePlot(AFortPlayerControllerAthena* Controller, FString& IslandId, FString& Locale, FString& Title);
    static void RestoreCreativePlot(AFortPlayerControllerAthena* Controller, FString& IslandId);

    static void LoadIslands(AFortPlayerControllerAthena* Controller, FString Title);
    static void ShowPlayset(UFortPlaysetItemDefinition* Playset, AFortVolume* Volume);
    static void MakeNewIsland(AFortPlayerControllerAthena* Controller, FString Title, bool Deleted);
    DefHookOg(void, ServerEndMinigame, AFortPlayerControllerAthena* PlayerController, FFrame& Stack);
    DefHookOg(void, ServerStartMinigame, AFortPlayerControllerAthena* PlayerController, FFrame& Stack);
    static void ServerUpdateGameplayOptions(UObject* Context, FFrame& Stack);
    static void ServerSetAllowGravity(AFortCreativeMoveTool* Tool, bool bAllow);
    static void ComputeSelectionSetTransformAndBounds(AFortCreativeMoveTool* Tool, FTransform& OutTransform, FBox& OutBounds);
    static void ServerStartInteracting(AFortCreativeMoveTool* Tool, TArray<AActor*>& Actors, FTransform DragStart);
    static void ServerDuplicateStartInteracting(UObject* Context, FFrame& Stack);
    static void ServerSpawnActorWithTransform(UObject* Context, FFrame& Stack);
    static void InitCreativeHooks();
};