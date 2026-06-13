#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/Creative/Public/Creative.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>
static std::vector<std::string> Islands;
void Creative::MakeNewCreativePlot(UObject* Context, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    UFortCreativeRealEstatePlotItemDefinition PlotType;
    FString Locale;
    FString Title;

    Stack.StepCompiledIn(&PlotType);
    Stack.StepCompiledIn(&Locale);
    Stack.StepCompiledIn(&Title);
    Stack.IncrementCode();

    auto Controller = (AFortPlayerControllerAthena*)Context;
    if (!Controller) return;
    Controller->ClientBroadcastOnMakeNewCreativePlotFinished(true, UKismetTextLibrary::Conv_StringToText(L""));
}

void Creative::DestroyCreativePlot(AFortPlayerControllerAthena* Controller, FString& IslandId)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
    auto CreativeIslands = Controller->CreativeIslands;

    CreativeIslands[0].bIsDeleted = true;
    Controller->ClientBroadcastOnDestroyCreativePlotFinished(true, UKismetTextLibrary::Conv_StringToText(L""));
}

void Creative::DuplicateCreativePlot(AFortPlayerControllerAthena* Controller, FString& IslandId, FString& Locale, FString& Title)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
    //auto CreativeIslands = Controller->CreativeIslands;
    //auto Name = CreativeIslands[0].IslandName;

    //MakeNewIsland(Controller, UKismetTextLibrary::Conv_TextToString(Name), false);
    Controller->ClientBroadcastOnDuplicateCreativePlotFinished(true, UKismetTextLibrary::Conv_StringToText(L""));
}

void Creative::RestoreCreativePlot(AFortPlayerControllerAthena* Controller, FString& IslandId)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
    auto CreativeIslands = Controller->CreativeIslands;
    CreativeIslands[0].bIsDeleted = false; // scuffed....
    Controller->ClientBroadcastOnRestoreCreativePlotFinished(true, UKismetTextLibrary::Conv_StringToText(L""));
}

static void(*OnStopCallbackOG)(AFortProjectileBase* Base, FHitResult& Hit);
void Creative::OnStopCallback(AFortProjectileBase* Base, FHitResult& Hit)
{
    if (!Globals::bCreative) return;
    if (!Base->IsA(AB_Prj_Athena_PlaysetGrenade_C::StaticClass()))
        return;
    AB_Prj_Athena_PlaysetGrenade_C* PG = Utils::Cast<AB_Prj_Athena_PlaysetGrenade_C>(Base);
    if (!PG)
        return;
    auto Playset = PG->CachedPlayset;
    if (!Playset)
        return;
    FVector SpawnLoc = PG->K2_GetActorLocation();
    FRotator SpawnRot = PG->K2_GetActorRotation();
    UClass* ClassToSpawn = Playset->ActorClassCount[0].First.LoadAsset();
    if (ClassToSpawn)
    {
        Utils::SpawnActor<AActor>(SpawnLoc, SpawnRot, ClassToSpawn);// bad andreu!
    }

    if (Base->IsA<AB_Prj_Athena_PlaysetGrenade_C>())
    {
        auto* PlaysetGrenade = (AB_Prj_Athena_PlaysetGrenade_C*)Base;
        if (UFortPlaysetItemDefinition* Playset = PlaysetGrenade->CachedPlayset)
        {
            if (Playset->Class)
            {
                std::string PathName = UKismetSystemLibrary::GetPathName(Playset).ToString();

                size_t LastDot = PathName.find_last_of('.');
                if (LastDot != std::string::npos)
                {
                    std::string AssetName = PathName.substr(LastDot + 1);

                    if (AssetName.rfind("PID_", 0) == 0)
                    {
                        size_t pgPos = AssetName.find("PG_");
                        size_t cpPos = AssetName.find("CP_");
                        if (pgPos != std::string::npos)
                            AssetName = AssetName.substr(pgPos);
                        else if (cpPos != std::string::npos)
                            AssetName = AssetName.substr(cpPos);
                    }

                    if (AssetName.rfind("CP_", 0) == 0 || AssetName.rfind("PG_", 0) == 0)
                    {
                        std::vector<std::string> parts;
                        size_t start = 0, pos = 0;
                        while ((pos = AssetName.find('_', start)) != std::string::npos)
                        {
                            if (pos > start) parts.push_back(AssetName.substr(start, pos - start));
                            start = pos + 1;
                        }
                        if (start < AssetName.length())
                            parts.push_back(AssetName.substr(start));

                        if (parts.size() >= 3)
                        {
                            std::string BasePath;
                            if (parts[1] == "Devices")
                                BasePath = "/Game/Creative/Maps/Devices/";
                            else if (AssetName.rfind("CP_", 0) == 0)
                                BasePath = "/Game/Creative/Maps/Prefabs/";
                            else
                                BasePath = "/Game/Playgrounds/Maps/Playsets/";

                            std::string AssetFullName = AssetName + "." + AssetName;

                            std::string ConstructedPath;
                            UWorld* World = nullptr;

                            if (AssetName.find("Gallery") != std::string::npos)
                            {
                                ConstructedPath = BasePath + parts[1] + "/Gallery/" + AssetFullName;
                                World = Utils::StaticLoadObject<UWorld>(ConstructedPath);
                            }

                            if (!World)
                            {
                                ConstructedPath = BasePath + parts[1] + "/" + AssetFullName;
                                World = Utils::StaticLoadObject<UWorld>(ConstructedPath);
                            }

                            if (!World)
                            {
                                ConstructedPath = BasePath + AssetFullName;
                                World = Utils::StaticLoadObject<UWorld>(ConstructedPath);
                            }

                            if (!World)
                            {
                                std::string SizeFolder;
                                if (parts[2] == "S") SizeFolder = "Small/";
                                else if (parts[2] == "M") SizeFolder = "Medium/";
                                else if (parts[2] == "L") SizeFolder = "Large/";
                                else if (parts[2] == "XL") SizeFolder = "XLarge/";

                                ConstructedPath = BasePath + parts[1] + "/" + SizeFolder + AssetFullName;
                                World = Utils::StaticLoadObject<UWorld>(ConstructedPath);
                            }

                            if (World)
                            {
                                FVector ProjectileLocation = Base->K2_GetActorLocation();
                                FRotator ProjectileRotation = Base->K2_GetActorRotation();

                                if (ULevel* Level = World->PersistentLevel)
                                {
                                    auto& Actors = *reinterpret_cast<TArray<class AActor*>*>(
                                        reinterpret_cast<uintptr_t>(Level) + 0x98);
                                    for (AActor* Actor : Actors)
                                    {
                                        USceneComponent* Root = Actor ? Actor->K2_GetRootComponent() : nullptr;
                                        if (!Root)
                                            continue;

                                        FVector RelativeLocation = Root->RelativeLocation;
                                        FRotator RelativeRotation = Root->RelativeRotation;

                                        float yawRad = (ProjectileRotation.Yaw + 180.0f) * 3.14159265359f / 180.0f;
                                        float cosYaw = std::cos(yawRad);
                                        float sinYaw = std::sin(yawRad);

                                        FVector RotatedLocation = FVector(
                                            RelativeLocation.X * cosYaw - RelativeLocation.Y * sinYaw,
                                            RelativeLocation.X * sinYaw + RelativeLocation.Y * cosYaw,
                                            RelativeLocation.Z
                                        );
                                        FVector SpawnLocation = ProjectileLocation + RotatedLocation;

                                        FRotator FinalRotation = FRotator(
                                            RelativeRotation.Pitch,
                                            RelativeRotation.Yaw + ProjectileRotation.Yaw + 180.0f,
                                            RelativeRotation.Roll
                                        );

                                        Utils::SpawnActor<AActor>(SpawnLocation, FinalRotation, Actor->Class);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }

        Base->K2_DestroyActor();
    }


    return OnStopCallbackOG(Base, Hit);
}

void Creative::UpdateCreativePlotData(AFortPlayerControllerAthena* Controller, AFortVolume* VolumeToPublish, FCreativeIslandInfo& MyInfo)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;

    FCreativeIslandData Data{};
    Data.IslandName = UKismetTextLibrary::Conv_StringToText(MyInfo.IslandTitle);
    Data.bIsDeleted = false;

    Controller->CreativeIslands.Add(Data);
    Controller->OnRep_CreativeIslands();
}

extern inline __int64 (*ShowPlaysetOG)(class UPlaysetLevelStreamComponent*) = decltype(ShowPlaysetOG)(InSDKUtils::GetImageBase() + 0x2c03580);
void Creative::ShowPlayset(UFortPlaysetItemDefinition* Playset, AFortVolume* Volume)
{
    if (!Globals::bCreative) return;
    auto LevelStreamComponent = (UPlaysetLevelStreamComponent*)Volume->GetComponentByClass(UPlaysetLevelStreamComponent::StaticClass());
    if (!LevelStreamComponent) return;
    LevelStreamComponent->SetPlayset(Playset);

    ShowPlaysetOG(LevelStreamComponent);
}

void Creative::ClientBroadcastOnUpdateCreativePlotName(AFortPlayerControllerAthena* Controller, bool bSuccess, FText& Reason)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
}

void Creative::TeleportPlayerToLinkedVolume(AFortAthenaCreativePortal* Portal, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    AFortPlayerPawn* PlayerPawn = nullptr;
    bool bUseSpawnTags;

    Stack.StepCompiledIn(&PlayerPawn);
    Stack.StepCompiledIn(&bUseSpawnTags);
    Stack.IncrementCode();

    if (!PlayerPawn) return;
    if (!Portal->LinkedVolume) return;

    auto Controller = (AFortPlayerControllerAthena*)PlayerPawn->Controller;
    if (!Controller) return;

    // Teleporta para Z=10000 acima do volume destino
    // Sem BeginSkydiving — causava bug de input em todos os players
    // O glider aparece naturalmente por estar em queda livre
    auto Location = Portal->LinkedVolume->K2_GetActorLocation();
    Location.Z = 10000.f;
    PlayerPawn->K2_TeleportTo(Location, FRotator());

    auto Phone = Utils::StaticLoadObject<UFortItemDefinition>(
        "/Game/Athena/Items/Weapons/Prototype/WID_CreativeTool.WID_CreativeTool"
    );

    bool bHasPhone = false;
    for (auto& Entry : Controller->WorldInventory->Inventory.ReplicatedEntries)
    {
        if (Entry.ItemDefinition == Phone)
        {
            bHasPhone = true;
            break;
        }
    }

    if (!bHasPhone)
        Inventory::GiveItem(Controller, Phone);

    auto PlayerName = Controller->PlayerState->GetPlayerName().ToString();
    if (std::find(Islands.begin(), Islands.end(), PlayerName) == Islands.end())
        Islands.push_back(PlayerName);

    Controller->bBuildFree = true;
}

void Creative::UpdateCreativePlotName(UObject* Context, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    FString Locale;
    FString Title;
    FString IslandId;

    Stack.StepCompiledIn(&IslandId);
    Stack.StepCompiledIn(&Locale);
    Stack.StepCompiledIn(&Title);
    Stack.IncrementCode();

    auto Controller = (AFortPlayerControllerAthena*)Context;
    if (!Controller) return;

    Controller->CreativeIslands[0].IslandName = UKismetTextLibrary::Conv_StringToText(Title);
    Controller->ClientBroadcastOnUpdateCreativePlotName(true, UKismetTextLibrary::Conv_StringToText(L""));
}

void Creative::UpdateCreativeIslandDescriptionTags(AFortPlayerControllerAthena* Controller, FString& IslandId, FString& Locale, TArray<class FString>& DescriptionTags)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
}

void Creative::ServerLoadPlotForPortal(AFortPlayerControllerAthena* Controller, AFortAthenaCreativePortal* Portal, FString& PlotItemId)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;

    auto GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    if (!GameState) return;
}


void Creative::LoadIslands(AFortPlayerControllerAthena* Controller, FString Title)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
    MakeNewIsland(Controller, Title, false);
}

void Creative::MakeNewIsland(AFortPlayerControllerAthena* Controller, FString Title, bool Deleted)
{
    if (!Globals::bCreative) return;
    if (!Controller) return;
    FCreativeIslandData Data;
    Data.bIsDeleted = Deleted;
    Data.PublishedIslandVersion = 1;
    Data.PublishedIslandCode = Utils::generateIslandCodeThing();
    Data.IslandName = UKismetTextLibrary::Conv_StringToText(Title);
    Data.McpId = L"creative";
    Data.LastLoadedDate = UKismetMathLibrary::UtcNow();

    Controller->CreativeIslands.Add(Data);
    Controller->OnRep_CreativeIslands();
}

void Creative::BeginPlay(AFortMinigameSettingsBuilding* Minigame)
{
    if (!Globals::bCreative) return;
    Minigame->SettingsVolume = (AFortVolume*)Minigame->GetOwner();//ayo this was so easy? "AFortMinigameSettingsBuilding::BeginPlay: FullName[%s] or "&AFortMinigameSettingsBuilding::OnLinkedVolumeSet"
}

void Creative::EndGame(AFortMinigame* Minigame, AFortPlayerController* Controller, EFortMinigameEnd EndMethod)
{
    if (!Globals::bCreative) return;
    static bool IsMatchpointReached = false;
    if (!Controller) return;

    Minigame->TotalRounds = 5;

    bool bIsMatchpoint = false;
    if (EndMethod == EFortMinigameEnd::EndRound)
    {
        bIsMatchpoint = (Minigame->CurrentRound >= Minigame->TotalRounds);
    }

    Minigame->RoundWinnerDisplayTime = 5.f;
    Minigame->RoundScoreDisplayTime = 1.0f;

    Minigame->CurrentState = EFortMinigameState::PostGameEnd;
    Minigame->QueueAllAIForDespawn();
}

void ServerGiveCreativeItemHook(AFortPlayerControllerAthena* Controller, FFortItemEntry CreativeItem)
{
    if (!Globals::bCreative) return;
    if (!CreativeItem.ItemDefinition)
        return;

    bool bShouldUpdate = false;

    Inventory::GiveItemStack(Controller, CreativeItem.ItemDefinition, CreativeItem.Count, CreativeItem.LoadedAmmo);

    //if (bShouldUpdate)
       // Inventory::Update(Controller);
}

void Creative::ServerEndMinigame(AFortPlayerControllerAthena* Controller, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    Stack.IncrementCode();
    ServerEndMinigameOG(Controller, Stack);

    AFortMinigame* Minigame = Controller->GetMinigame();
    if (!Minigame) return;

    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    if (!GameMode) return;

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    if (!GameState) return;

    auto GamePhase = GameState->GamePhase;

    Minigame->TotalRounds = 5;

    if (Minigame->TotalRounds >= 0) {
        Minigame->CurrentState = EFortMinigameState::PostRoundEnd;
        Minigame->CurrentRound++;

        Minigame->RoundWinnerDisplayTime = 5.f;
        Minigame->RoundScoreDisplayTime = 1.0f;
    }

    GamePhase = EAthenaGamePhase::EndGame;

    std::thread([Minigame]() {
        while (Minigame->CurrentState == EFortMinigameState::PostGameAbandon) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        Minigame->CurrentState = EFortMinigameState::PreGame;
        Minigame->OnRep_CurrentState();

        }).detach();
}

static bool bTextChanged = false;
void Creative::ServerStartMinigame(AFortPlayerControllerAthena* Controller, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    Stack.IncrementCode();

    ServerStartMinigameOG(Controller, Stack);

    AFortMinigame* Minigame = Controller->GetMinigame();
    if (!Minigame) return;

    struct FortMinigame_GetParticipatingPlayers final
    {
    public:
        TArray<class AFortPlayerState*> OutPlayers;
    } ret{};

    Minigame->GetParticipatingPlayers(&ret.OutPlayers);
    TArray<class AFortPlayerState*> Players = ret.OutPlayers;

    AFortPlayerControllerAthena* PlayerController = nullptr;

    for (int i = 0; i < Players.Num(); i++) {
        auto Player = (AFortPlayerStateAthena*)Players[i];
        PlayerController = (AFortPlayerControllerAthena*)Player->GetOwner();

        PlayerController->MyFortPawn->ForceKill(FGameplayTag(UKismetStringLibrary::Conv_StringToName(L"DeathCause.BanHammer")), PlayerController, nullptr);
    }

    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    if (!GameMode) return;

    auto GameState = (AFortGameStateAthena*)GameMode->GameState;
    if (!GameState) return;

    auto GamePhase = GameState->GamePhase;

    if (GamePhase == EAthenaGamePhase::Warmup) {
        GameMode->SafeZonePhase++;
        GamePhase = EAthenaGamePhase::SafeZones;
        Minigame->OnGamePhaseChanged(GamePhase);
    }

    std::thread([Minigame]() {
        std::this_thread::sleep_for(std::chrono::seconds(3));
        Minigame->AdvanceState();
        Minigame->HandleMinigameStarted();
        Minigame->HandleVolumeEditModeChange(false);
        }).detach();
}

void Creative::ServerUpdateGameplayOptions(UObject* Context, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    TArray<FString> UserOptionsKeys;
    TArray<FString> UserOptionsValues;

    Stack.StepCompiledIn(&UserOptionsKeys);
    Stack.StepCompiledIn(&UserOptionsValues);
    Stack.IncrementCode();

    auto Controller = (AFortPlayerControllerAthena*)Context;
    if (!Controller) return;

    auto PlayerState = (AFortPlayerStateAthena*)Controller->PlayerState;
    if (UserOptionsKeys.Num() != UserOptionsValues.Num()) return;

    UFortLevelSaveComponent* Save = (UFortLevelSaveComponent*)Controller->CreativePlotLinkedVolume->GetComponentByClass(UFortLevelSaveComponent::StaticClass());

    if (AFortMinigame* Minigame = Controller->GetMinigame())
    {
        if (Minigame->CurrentState >= EFortMinigameState::Setup && Minigame->CurrentState <= EFortMinigameState::PostGameAbandon) return;
    }

    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

    UFortMutatorListComponent* MutatorList = GameMode->MutatorListComponent;
    UPlaylistUserOptions* UserOptions = nullptr;
    if (MutatorList && MutatorList->UserOptions)
    {
        UserOptions = MutatorList->UserOptions;
    }
    else
    {
        return;
    }

    TMap<FString, FString> NewOptions;
    for (int32 i = 0; i < UserOptionsKeys.Num(); ++i) NewOptions.Add(UserOptionsKeys[i], UserOptionsKeys[i]);

    if (MutatorList)
    {
        MutatorList->SetPropertyOverrides(NewOptions);
    }
}

void Creative::ServerSetAllowGravity(AFortCreativeMoveTool* Tool, bool bAllow)
{
    if (!Globals::bCreative) return;
    Tool->bAllowGravityOnPlace = bAllow;
    Tool->OnRep_AllowGravityOnPlace();
}

void Creative::ComputeSelectionSetTransformAndBounds(AFortCreativeMoveTool* Tool, FTransform& OutTransform, FBox& OutBounds)
{
    if (!Globals::bCreative) return;
    if (!Tool->SelectedActors.Num()) return;

    auto SelectedActor = Tool->SelectedActors[0].Actor;

    if (SelectedActor)
    {
        OutTransform = SelectedActor->GetTransform();
    }
}

void Creative::ServerStartInteracting(AFortCreativeMoveTool* Tool, TArray<AActor*>& Actors, FTransform DragStart)
{
    if (!Globals::bCreative) return;
    Tool->SelectedActors.Free();
    Tool->NewlyPlacedActors.Free();

    if (!Tool->ActiveMovementMode) {
        Tool->ActiveMovementMode = Tool->InteractionBehaviors[0];
    }

    for (auto& Actor : Actors)
    {
        FCreativeSelectedActorInfo SelectedActorInfo{};
        SelectedActorInfo.Actor = Actor;
        SelectedActorInfo.UnscaledActorToSelectionAtDragStart = DragStart;

        Tool->SelectedActors.Add(SelectedActorInfo);
    }

    FTransform Transform;
    FBox Bounds;

    ComputeSelectionSetTransformAndBounds(Tool, Transform, Bounds);
    Tool->ClientStartInteracting(Tool->ActiveMovementMode, Tool->SelectedActors, Transform, Bounds);

    Tool->ActiveMovementMode->StartCreativeInteractionOnServer();
}

void Creative::ServerDuplicateStartInteracting(UObject* Context, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    TArray<AActor*> Actors;
    FTransform DragStart;

    Stack.StepCompiledIn(&Actors);
    Stack.StepCompiledIn(&DragStart);
    Stack.IncrementCode();

    AFortCreativeMoveTool* Tool = (AFortCreativeMoveTool*)Context;
    TArray<AActor*> InteractionActors;

    for (auto& Actor : Actors)
    {
        if (!Actor) return;

        if (Actor->IsA<ABuildingActor>())
        {
            auto SpawnedActor = Utils::SpawnActorCreative<ABuildingActor>(Actor->GetTransform(), Actor->Class);

            if (!SpawnedActor) return;

            SpawnedActor->InitializeKismetSpawnedBuildingActor(SpawnedActor, NULL, false);

            InteractionActors.Add(SpawnedActor);
        }
    }

    ServerStartInteracting(Tool, InteractionActors, DragStart);
}

void Creative::ServerSpawnActorWithTransform(UObject* Context, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    AActor* ActorToSpawn;
    FTransform TargetTransform;
    bool bAllowOverlap;
    bool bAllowGravity;
    bool bIgnoreStructuralIssues;
    bool bForPreviewing;

    Stack.StepCompiledIn(&ActorToSpawn);
    Stack.StepCompiledIn(&TargetTransform);
    Stack.StepCompiledIn(&bAllowOverlap);
    Stack.StepCompiledIn(&bAllowGravity);
    Stack.StepCompiledIn(&bIgnoreStructuralIssues);
    Stack.StepCompiledIn(&bForPreviewing);
    Stack.IncrementCode();

    AFortCreativeMoveTool* Tool = (AFortCreativeMoveTool*)Context;
    for (auto& NewlyPlacedActor : Tool->NewlyPlacedActors)
    {
        if (NewlyPlacedActor.OriginalActor == ActorToSpawn) return;
    }

    if (ActorToSpawn->IsA<ABuildingActor>()) {
        auto SpawnedActor = Utils::SpawnActorCreative<ABuildingActor>(TargetTransform, ActorToSpawn->Class);
        Tool->bClientNeedsToProcessNewlyPlacedActors = true;
        Tool->NewlyPlacedActors.Add(FOriginalAndSpawnedPair{ ActorToSpawn, SpawnedActor });
        Tool->OnRep_NewlyPlacedActors();
    }
}

void Creative::ServerCreativeSetFlightSpeedIndex(UObject* Context, FFrame& Stack)
{
    if (!Globals::bCreative) return;
    int Index;
    Stack.StepCompiledIn(&Index);
    Stack.IncrementCode();

    auto PlayerController = (AFortPlayerControllerAthena*)Context;
    PlayerController->FlyingModifierIndex = Index;
    PlayerController->OnRep_FlyingModifierIndex();

    return callOG(PlayerController, "/Script/FortniteGame.FortPlayerControllerGameplay", ServerCreativeSetFlightSpeedIndex, Index);
}

void Creative::ServerTeleportToPlaygroundLobbyIsland(AFortPlayerControllerAthena* Controller)
{
    if (!Controller || !Globals::bCreative) return;

    auto GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;

    if (Controller->WarmupPlayerStart)
        Controller->GetPlayerPawn()->K2_TeleportTo(Controller->WarmupPlayerStart->K2_GetActorLocation(), Controller->WarmupPlayerStart->K2_GetActorRotation());
    else
    {
        AActor* Actor = GameMode->ChoosePlayerStart(Controller);
        if (Actor)
            Controller->GetPlayerPawn()->K2_TeleportTo(Actor->K2_GetActorLocation(), FRotator());
    }

    Controller->bBuildFree = true;
}

void Creative::InitCreativeHooks()
{
    if (!Globals::bCreative) return;
    Utils::ExecHook("/Script/FortniteGame.FortAthenaCreativePortal.TeleportPlayerToLinkedVolume", TeleportPlayerToLinkedVolume);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.ServerTeleportToPlaygroundLobbyIsland", ServerTeleportToPlaygroundLobbyIsland);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.MakeNewCreativePlot", MakeNewCreativePlot);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.UpdateCreativePlotName", UpdateCreativePlotName);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.DestroyCreativePlot", DestroyCreativePlot);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.DuplicateCreativePlot", DuplicateCreativePlot);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.RestoreCreativePlot", RestoreCreativePlot);
    Utils::ExecHook("/Script/FortniteGame.FortProjectileBase.OnStopCallback", OnStopCallback, OnStopCallbackOG);
    Utils::ExecHook("/Script/FortniteGame.FortMinigame.EndGame", EndGame);
    MH_CreateHook((LPVOID)(ImageBase + 0x2F005F0), BeginPlay, nullptr);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2EDE4F0), OnStopCallback, (LPVOID*)(&OnStopCallbackOG));//48 89 74 24 ? 48 89 7C 24 ? 55 48 8D 6C 24 ? 48 81 EC ? ? ? ? 48 8B F9 48 8B F2 8B 89
    Utils::SwapVFTs(AFortPlayerControllerAthena::StaticClass()->DefaultObject, 0x4A3, ServerGiveCreativeItemHook, nullptr);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.ServerStartMinigame", ServerStartMinigame, ServerStartMinigameOG);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.ServerEndMinigame", ServerEndMinigame, ServerEndMinigameOG);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerController.ServerUpdateGameplayOptions", ServerUpdateGameplayOptions);
    Utils::ExecHook("/Script/FortniteGame.FortCreativeMoveTool.ServerDuplicateStartInteracting", ServerDuplicateStartInteracting);
    Utils::ExecHook("/Script/FortniteGame.FortCreativeMoveTool.ServerSetAllowGravity", ServerSetAllowGravity);
    Utils::ExecHook("/Script/FortniteGame.FortCreativeMoveTool.ServerSpawnActorWithTransform", ServerSpawnActorWithTransform);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerGameplay.ServerCreativeSetFlightSpeedIndex", ServerCreativeSetFlightSpeedIndex, ServerCreativeSetFlightSpeedIndexOG);
}