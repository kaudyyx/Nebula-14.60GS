#include "pch.h"
#include "../../../Public/PlayerController/Public/PlayerController.h"
#include <FortniteGame/Source/FortniteGame/Public/LateGame/Public/LateGame.h>
#include <FortniteGame/Source/FortniteGame/Public/QuestManager/Public/QuestManage.h>
#include <FortniteGame/Source/FortniteAI/Public/Spawners/Public/AISpawner.h>
#include <string>
#include <thread>
#include <unordered_set>
#include <vector>
#include <algorithm>
#include <Engine/Source/ThirdParty/API/Header/API.h>

namespace NebulaMatchStats { bool ApplyMatchStats(const std::string& username, int32_t amount); }
namespace NebulaXP { bool AddXP(const std::string& username, int32_t xpAmount); }
namespace NebulaPlayerData { void FetchDivisionAsync(const std::string& username); int GetPlayerDivision(const std::string& username); void ClearCache(); }

static std::unordered_set<std::string> PlayersThatJoined{};

namespace
{
    std::unordered_set<std::string> AwardedBusfarePlayers;
    std::unordered_set<int>         AwardedWinTeams;
    std::unordered_set<std::string> TournamentStartedPlayers;
    std::unordered_set<std::string> TournamentFinishedPlayers;
    static std::unordered_map<std::string, int32_t> TournamentMatchPoints;

    static std::string GetPlayerUsername(AFortPlayerStateAthena* PlayerState)
    {
        if (!PlayerState)
            return std::string();
        return PlayerState->GetPlayerName().ToString();
    }

    static std::string GetPlacementReason(int Placement)
    {
        switch (Placement)
        {
        case 2:  return "placement2";
        case 3:  return "placement3";
        case 4:  return "placement4";
        case 5:  return "placement5";
        case 10: return "placement10";
        case 20: return "placement20";
        case 25: return "placement25";
        default: return std::string();
        }
    }

    static void ApplyMatchStatAsync(const std::string& Username, int32_t Amount)
    {
        if (Username.empty() || Amount == 0)
            return;
        std::thread([Username, Amount]()
            {
                try { NebulaMatchStats::ApplyMatchStats(Username, Amount); }
                catch (...) {}
            }).detach();
    }

    static void ApplyMatchStatToPlayerState(AFortPlayerStateAthena* PlayerState, int32_t Amount)
    {
        if (!PlayerState || Amount == 0)
            return;
        std::string Username = GetPlayerUsername(PlayerState);
        if (Username.empty())
            return;
        ApplyMatchStatAsync(Username, Amount);
    }

    static void ApplyMatchStatToTeam(int TeamIndex, int32_t Amount)
    {
        if (Amount == 0)
            return;
        auto World = UWorld::GetWorld();
        if (!World)
            return;
        auto GameStateAthena = Utils::Cast<AFortGameStateAthena>(World->GameState);
        if (!GameStateAthena)
            return;
        for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
        {
            AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
            if (!TeamInfo || TeamInfo->Team != TeamIndex)
                continue;
            for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
            {
                AFortPlayerControllerAthena* TeamMember = Utils::Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
                if (!TeamMember)
                    continue;
                AFortPlayerStateAthena* TeamMemberPlayerState = Utils::Cast<AFortPlayerStateAthena>(TeamMember->PlayerState);
                if (!TeamMemberPlayerState)
                    continue;
                ApplyMatchStatToPlayerState(TeamMemberPlayerState, Amount);
            }
            break;
        }
    }

    static void ApplyPlacementToTeam(int TeamIndex, int32_t Amount)
    {
        ApplyMatchStatToTeam(TeamIndex, Amount);
    }

    static int32_t GetPlacementPoints(int threshold)
    {
        for (const auto& p : placements)
            if (p.placementThreshold == threshold) return p.pointsScored;
        return 0;
    }

    static void TryAwardBusfare(AFortPlayerStateAthena* PlayerState)
    {
        if (!PlayerState)
            return;
        std::string Username = GetPlayerUsername(PlayerState);
        if (Username.empty())
            return;
        if (AwardedBusfarePlayers.find(Username) != AwardedBusfarePlayers.end())
            return;
        if (Globals::bArena || (Globals::bEnableBackendMode && !Globals::IsTournamentModeEnabled()))
        {
            AwardedBusfarePlayers.insert(Username);
            int playerDiv = NebulaPlayerData::GetPlayerDivision(Username);
            int32_t busFare = GetArenaBusFare(playerDiv);
            if (busFare != 0)
                ApplyMatchStatAsync(Username, busFare);
            if (kXPBusFare > 0)
            {
                std::thread([Username]()
                    {
                        try { NebulaXP::AddXP(Username, kXPBusFare); }
                        catch (...) {}
                    }).detach();
            }
        }
    }

    static int GetTournamentTimeAliveSeconds()
    {
        auto World = UWorld::GetWorld();
        if (!World)
            return 0;
        float TimeSeconds = UGameplayStatics::GetTimeSeconds(World);
        if (TimeSeconds < 0.f)
            TimeSeconds = 0.f;
        return (int)TimeSeconds;
    }

    static void TryStartTournamentForPlayer(AFortPlayerStateAthena* PlayerState)
    {
        if (!Globals::IsTournamentModeEnabled() || !PlayerState)
            return;
        std::string Username = GetPlayerUsername(PlayerState);
        if (Username.empty())
            return;
        if (TournamentStartedPlayers.find(Username) != TournamentStartedPlayers.end())
            return;
        TournamentStartedPlayers.insert(Username);
        NebulaTournament::StartMatchAsync(Username, Globals::NebulaSessionId);
    }

    static void TournamentAccumulateMilestonePoints(const std::string& Username, int32_t Amount)
    {
        if (Username.empty() || Amount == 0)
            return;
        TournamentMatchPoints[Username] += Amount;
    }

    static void TryFinishTournamentForPlayer(AFortPlayerStateAthena* PlayerState, int Placement, bool bVictory)
    {
        if (!Globals::IsTournamentModeEnabled() || !PlayerState)
            return;
        std::string Username = GetPlayerUsername(PlayerState);
        if (Username.empty())
            return;
        if (TournamentFinishedPlayers.find(Username) != TournamentFinishedPlayers.end())
            return;
        TournamentFinishedPlayers.insert(Username);
        int timeAlive = GetTournamentTimeAliveSeconds();
        NebulaTournament::FinishMatchAsync(Username, Placement, PlayerState->KillScore,
            Globals::NebulaSessionId, bVictory, timeAlive);
        int32_t killPts = PlayerState->KillScore * GetArenaKillPoints();
        int32_t milestonePts = 0;
        {
            auto it = TournamentMatchPoints.find(Username);
            if (it != TournamentMatchPoints.end())
                milestonePts = it->second;
        }
        int32_t placementBonus = 0;
        if (bVictory || Placement == 1)
            placementBonus = placements[0].pointsScored;
        else
            placementBonus = GetPlacementPoints(Placement);
        int32_t totalPoints = killPts + milestonePts + placementBonus;
        if (totalPoints > 0)
            NebulaTournament::ChangePointsAsync(Username, totalPoints);
    }

}
static void ServerAttemptInteract_Impl(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData, EInteractionBeingAttempted InteractionBeingAttempted, int32 RequestID);
static void ServerReadyToStartMatch_Impl(AFortPlayerControllerAthena* PC);
static void Redeploy_Glider_Impl(UObject* Object, FFrame& Stack);
static void K2_ActivateAbility_Impl(UObject* Object, FFrame& Stack);
static __int64 __fastcall ABuildingSMActor_PostUpdate_Impl(ABuildingSMActor* Actor);
static void ServerReturnToMainMenu_Impl(AFortPlayerControllerAthena* PC);
static void ServerSetInAircraft_Impl(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft);

void (*ServerAcknowledgePossessionOG)(AFortPlayerControllerAthena* PC, APawn* Pawn);
void PlayerController::ServerAcknowledgePossessionHook(AFortPlayerControllerAthena* PC, APawn* Pawn)
{
    __try { ServerAcknowledgePossessionHook_Impl(PC, Pawn); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerAcknowledgePossessionHook_Impl(AFortPlayerControllerAthena* PC, APawn* Pawn)
{
    auto GS = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    auto GM = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    AFortPlayerStateAthena* PlayerState = Utils::Cast<AFortPlayerStateAthena>(PC->PlayerState);
    if (!PlayerState)
        return;
    PC->AcknowledgedPawn = Pawn;
    if (PC->CosmeticLoadoutPC.Character)
        ((AFortPlayerStateAthena*)PC->PlayerState)->HeroType = PC->CosmeticLoadoutPC.Character->HeroDefinition;
    UFortKismetLibrary::UpdatePlayerCustomCharacterPartsVisualization(PlayerState);
    ((void(*)(APlayerState*, APawn*))(ImageBase + 0x2BEE4C0))(PC->PlayerState, Pawn);
    static bool First = false;
}

void PlayerController::ServerExecuteInventoryItemHook(AFortPlayerControllerAthena* PC, FGuid Guid)
{
    __try { ServerExecuteInventoryItemHook_Impl(PC, Guid); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerExecuteInventoryItemHook_Impl(AFortPlayerControllerAthena* PC, FGuid Guid)
{
    if (!PC || !PC->MyFortPawn || !PC->WorldInventory || PC->IsInAircraft())
        return;
    auto entry = PC->WorldInventory->Inventory.ReplicatedEntries.Search([&](FFortItemEntry& entry) {
        return entry.ItemGuid == Guid;
        });
    if (!entry || !PC->MyFortPawn)
        return;
    UFortWeaponItemDefinition* ItemDefinition = entry->ItemDefinition->IsA<UFortGadgetItemDefinition>() ? ((UFortGadgetItemDefinition*)entry->ItemDefinition)->GetWeaponItemDefinition() : (UFortWeaponItemDefinition*)entry->ItemDefinition;
    if (auto Deco = (UFortContextTrapItemDefinition*)ItemDefinition->Cast<UFortDecoItemDefinition>())
    {
        PC->MyFortPawn->PickUpActor(nullptr, Deco);
        if (!PC->MyFortPawn->CurrentWeapon)
            return;
        PC->MyFortPawn->CurrentWeapon->ItemEntryGuid = Guid;
        if (auto ContextTrap = PC->MyFortPawn->CurrentWeapon->Cast<AFortDecoTool_ContextTrap>())
            ContextTrap->ContextTrapItemDefinition = Deco;
        return;
    }
    PC->MyFortPawn->EquipWeaponDefinition(ItemDefinition, Guid);
}

void PlayerController::ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid Guid, int32 Count, bool bTrash)
{
    __try { ServerAttemptInventoryDrop_Impl(PC, Guid, Count, bTrash); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerAttemptInventoryDrop_Impl(AFortPlayerControllerAthena* PC, FGuid Guid, int32 Count, bool bTrash)
{
    if (!PC || !PC->Pawn || !PC->WorldInventory)
        return;
    auto itemEntry = PC->WorldInventory->Inventory.ReplicatedEntries.Search([Guid](FFortItemEntry& entry)
        { return entry.ItemGuid == Guid; });
    if (!itemEntry || (itemEntry->Count - Count) < 0)
        return;
    itemEntry->Count -= Count;
    Inventory::SpawnPickup(PC->Pawn->K2_GetActorLocation() + PC->Pawn->GetActorForwardVector() * 70.f + FVector(0, 0, 50), *itemEntry, EFortPickupSourceTypeFlag::Player, EFortPickupSpawnSource::Unset, PC->MyFortPawn, Count);
    if (itemEntry->Count == 0)
        Inventory::Remove(PC, Guid);
    else
        Inventory::ReplaceEntry(PC, *itemEntry);
    return;
}

void PlayerController::ServerAttemptAircraftJumpHook(UFortControllerComponent_Aircraft* Comp, FRotator ClientRot)
{
    __try { ServerAttemptAircraftJumpHook_Impl(Comp, ClientRot); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerAttemptAircraftJumpHook_Impl(UFortControllerComponent_Aircraft* Comp, FRotator ClientRot)
{
    if (!Comp)
        return;
    auto World = UWorld::GetWorld();
    if (!World)
        return;
    auto PlayerController = Utils::Cast<AFortPlayerControllerAthena>(Comp->GetOwner());
    if (!PlayerController)
        return;
    auto PlayerState = (AFortPlayerStateAthena*)PlayerController->PlayerState;
    auto GameMode = (AFortGameModeAthena*)World->AuthorityGameMode;
    auto GameState = (AFortGameStateAthena*)World->GameState;
    if (!GameMode || !GameState)
        return;
    GameMode->RestartPlayer(PlayerController);
    if (!PlayerController->MyFortPawn || !PlayerController->WorldInventory)
        return;
    if (Globals::bLateGame)
    {
        for (int i = PlayerController->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
        {
            auto ItemDef = Utils::Cast<UFortWorldItemDefinition>(PlayerController->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition);
            if (!ItemDef)
                continue;
            if (ItemDef->bCanBeDropped)
                Inventory::Remove(PlayerController, PlayerController->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid);
        }
    }
    if (Globals::bLateGame && PlayerController->MyFortPawn)
    {
        if (GameState->Aircrafts.Num() > 0 && GameState->Aircrafts[0])
        {
            FVector AircraftLoc = GameState->Aircrafts[0]->K2_GetActorLocation();
            PlayerController->MyFortPawn->K2_SetActorLocation(AircraftLoc, false, nullptr, false);
            auto* MovComp = (UCharacterMovementComponent*)PlayerController->MyFortPawn->GetMovementComponent();
            if (MovComp)
                MovComp->SetMovementMode(EMovementMode::MOVE_Falling, 0);
        }
    }
    PlayerController->ClientSetRotation(ClientRot, true);
    if (PlayerController->MyFortPawn)
    {
        PlayerController->MyFortPawn->BeginSkydiving(true);
        PlayerController->MyFortPawn->SetHealth(100);
        if (Globals::bLateGame)
        {
            PlayerController->MyFortPawn->SetShield(100);
            auto GetTrap = []() -> FItemAndCount
                {
                    static UEAllocatedVector<FItemAndCount> Traps
                    {
                        FItemAndCount(1, {}, Utils::StaticFindObject<UFortItemDefinition>("/Game/Athena/Items/Traps/TID_Floor_Player_Launch_Pad_Athena.TID_Floor_Player_Launch_Pad_Athena")),
                    };
                    FItemAndCount Result{};
                    for (int i = 0; i < 12; i++)
                    {
                        Result = Traps[rand() % Traps.size()];
                        if (Result.Item)
                            return Result;
                    }
                    return Result;
                };
            auto SafeGetWeaponClip = [](UFortItemDefinition* Item) -> int
                {
                    if (!Item || !Item->IsA<UFortWeaponItemDefinition>())
                        return 0;
                    auto* Stats = Inventory::GetStats((UFortWeaponItemDefinition*)Item);
                    return Stats ? Stats->ClipSize : 0;
                };
            auto SafeGiveItem = [&](const FItemAndCount& ItemAndCount, int LoadedAmmo = 0)
                {
                    if (!ItemAndCount.Item || ItemAndCount.Count <= 0)
                        return;
                    Inventory::GiveItem(PlayerController, ItemAndCount.Item, ItemAndCount.Count, LoadedAmmo, true);
                };
            FItemAndCount Shotgun{};
            FItemAndCount AssaultRifle{};
            FItemAndCount Flex{};
            FItemAndCount Heal{};
            FItemAndCount HealSlot2{};
            FItemAndCount Trap{};
            for (int i = 0; i < 16 && !Shotgun.Item; i++)
                Shotgun = Lategame::GetShotguns();
            for (int i = 0; i < 16 && !AssaultRifle.Item; i++)
                AssaultRifle = Lategame::GetAssaultRifles();
            for (int i = 0; i < 16 && !Flex.Item; i++)
                Flex = Lategame::GetSnipers();
            for (int i = 0; i < 16 && !Heal.Item; i++)
                Heal = Lategame::GetHeals();
            for (int i = 0; i < 16 && !HealSlot2.Item; i++)
                HealSlot2 = Lategame::GetHeals();
            Trap = GetTrap();
            int ShotgunClipSize = SafeGetWeaponClip(Shotgun.Item);
            int AssaultRifleClipSize = SafeGetWeaponClip(AssaultRifle.Item);
            int FlexClipSize = SafeGetWeaponClip(Flex.Item);
            int HealClipSize = SafeGetWeaponClip(Heal.Item);
            int HealSlot2ClipSize = SafeGetWeaponClip(HealSlot2.Item);
            Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Wood), 500);
            Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Stone), 500);
            Inventory::GiveItem(PlayerController, Lategame::GetResource(EFortResourceType::Metal), 500);
            Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Assault), 250);
            Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Shotgun), 50);
            Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Submachine), 400);
            Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Rocket), 6);
            Inventory::GiveItem(PlayerController, Lategame::GetAmmo(EAmmoType::Sniper), 20);
            SafeGiveItem(AssaultRifle, AssaultRifleClipSize);
            SafeGiveItem(Shotgun, ShotgunClipSize);
            int Pattern = rand() % 3;
            if (Pattern == 0)
            {
                SafeGiveItem(Flex, FlexClipSize);
                SafeGiveItem(Heal, HealClipSize);
                SafeGiveItem(HealSlot2, HealSlot2ClipSize);
            }
            else if (Pattern == 1)
            {
                SafeGiveItem(Heal, HealClipSize);
                SafeGiveItem(Flex, FlexClipSize);
                SafeGiveItem(HealSlot2, HealSlot2ClipSize);
            }
            else
            {
                SafeGiveItem(Heal, HealClipSize);
                SafeGiveItem(HealSlot2, HealSlot2ClipSize);
                SafeGiveItem(Flex, FlexClipSize);
            }
            SafeGiveItem(Trap, 0);
        }
    }
}

static inline void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData, EInteractionBeingAttempted InteractionBeingAttempted, int32 RequestID);
static void ServerAttemptInteract(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData, EInteractionBeingAttempted InteractionBeingAttempted, int32 RequestID)
{
    __try { ServerAttemptInteract_Impl(Comp, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void ServerAttemptInteract_Impl(UFortControllerComponent_Interaction* Comp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData, EInteractionBeingAttempted InteractionBeingAttempted, int32 RequestID)
{
    if (!Comp || !Comp->GetOwner())
    {
        ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID);
        return;
    }
    auto PC = Comp->GetOwner()->Cast<AFortPlayerControllerAthena>();
    ServerAttemptInteractOG(Comp, ReceivingActor, InteractComponent, InteractType, OptionalObjectData, InteractionBeingAttempted, RequestID);
    if (!PC)
        return;
    // if (!Globals::bLateGame)
  //   {
    if (auto BuildingActor = ReceivingActor->Cast<ABuildingActor>())
    {
        FGameplayTagContainer SourceTags;
        FGameplayTagContainer ContextTags;
        FGameplayTagContainer TargetTags = BuildingActor->StaticGameplayTags;
        auto QuestManager = PC->GetQuestManager(ESubGame::Athena);
        if (QuestManager)
        {
            QuestManager->GetSourceAndContextTags(&SourceTags, &ContextTags);
            Challenges::CountMap[BuildingActor->Class]++;
            if (auto Chest = ReceivingActor->Cast<ATiered_Chest_Athena_C>())
            {
                static auto Name = UKismetStringLibrary::Conv_StringToName(L"Loot_AthenaTreasure");
                if (Chest->SearchLootTierGroup == Name)
                {
                    auto PS = PC->PlayerState->Cast<AFortPlayerStateAthena>();
                    const char* AccoladePath = nullptr;
                    switch (Challenges::CountMap[BuildingActor->Class])
                    {
                    case 3:  AccoladePath = "/Game/Athena/Items/Accolades/AccoladeId_008_SearchChests_Bronze.AccoladeId_008_SearchChests_Bronze"; break;
                    case 7:  AccoladePath = "/Game/Athena/Items/Accolades/AccoladeId_009_SearchChests_Silver.AccoladeId_009_SearchChests_Silver"; break;
                    case 12: AccoladePath = "/Game/Athena/Items/Accolades/AccoladeId_010_SearchChests_Gold.AccoladeId_010_SearchChests_Gold"; break;
                    default: break;
                    }
                    if (AccoladePath)
                    {
                        auto Accolade = Utils::StaticFindObject<UFortAccoladeItemDefinition>(AccoladePath);
                        Challenges::GiveAccolade(PC, Accolade);
                    }
                }
            }
            Challenges::SendStatEvent(QuestManager, BuildingActor, SourceTags, TargetTags, nullptr, nullptr, Challenges::CountMap[BuildingActor->Class], EFortQuestObjectiveStatEvent::Interact);
        }
        //  }
    }
}

void (*GetPlayerViewPointOG)(AFortPlayerController*, FVector&, FRotator&);
void PlayerController::GetPlayerViewPoint(AFortPlayerController* PC, FVector& Loc, FRotator& Rot)
{
    __try { GetPlayerViewPoint_Impl(PC, Loc, Rot); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::GetPlayerViewPoint_Impl(AFortPlayerController* PC, FVector& Loc, FRotator& Rot)
{
    static auto Spec = UKismetStringLibrary::Conv_StringToName(L"Spectating");
    if (PC->StateName == Spec)
    {
        Loc = PC->LastSpectatorSyncLocation;
        Rot = PC->LastSpectatorSyncRotation;
    }
    else if (PC->GetViewTarget())
    {
        Loc = PC->GetViewTarget()->K2_GetActorLocation();
        Rot = PC->GetControlRotation();
    }
    else
        return GetPlayerViewPointOG(PC, Loc, Rot);
}

void (*ServerReadyToStartMatchOG)(AFortPlayerControllerAthena* PC);
void ServerReadyToStartMatch(AFortPlayerControllerAthena* PC)
{
    __try { ServerReadyToStartMatch_Impl(PC); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void ServerReadyToStartMatch_Impl(AFortPlayerControllerAthena* PC)
{
    if (!PC)
        return;
    AwardedBusfarePlayers.clear();
    AwardedWinTeams.clear();
    TournamentStartedPlayers.clear();
    TournamentFinishedPlayers.clear();
    TournamentMatchPoints.clear();
    PlayersThatJoined.clear();
    gBusfareDone.clear();
    NebulaPlayerData::ClearCache();
    AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    AFortGameStateAthena* GameState = (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
    if (!GameState)
        return ServerReadyToStartMatchOG(PC);
    GameState->DefaultBattleBus = Utils::StaticLoadObject<UAthenaBattleBusItemDefinition>("/Game/Athena/Items/Cosmetics/BattleBuses/BBID_BusUpgrade3.BBID_BusUpgrade3");
    AFortPlayerStateAthena* PlayerState = Utils::Cast<AFortPlayerStateAthena>(PC->PlayerState);
    if (!PlayerState)
        return;
    static bool areyoureal = false;
    if (!areyoureal && !Globals::bLateGame)
    {
        FortSpawner::RequestAISpawn();
        TArray<AActor*> WarmupActors;
        static UClass* WarmupClass = Utils::StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), WarmupClass, &WarmupActors);
        for (auto& WarmupActor : WarmupActors)
        {
            auto Container = (ABuildingContainer*)WarmupActor;
            Container->BP_SpawnLoot(nullptr);
            Container->K2_DestroyActor();
        }
        WarmupActors.Free();
        WarmupClass = Utils::StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C");
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), WarmupClass, &WarmupActors);
        for (auto& WarmupActor : WarmupActors)
        {
            auto Container = (ABuildingContainer*)WarmupActor;
            Container->BP_SpawnLoot(nullptr);
            Container->K2_DestroyActor();
        }
        WarmupActors.Free();
        if (!Globals::bCreative)
            PlayerState->bResurrectionChipDisabled = false;
        areyoureal = true;
    }
    return ServerReadyToStartMatchOG(PC);
}

static inline void (*Redeploy_GliderOG)(UObject* Object, FFrame& Stack);
static void Redeploy_Glider(UObject* Object, FFrame& Stack)
{
    __try { Redeploy_Glider_Impl(Object, Stack); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void Redeploy_Glider_Impl(UObject* Object, FFrame& Stack)
{
    auto PlayerLaunch = (UGA_WilliePete_PlayerLaunch_C*)Object;
    auto Pawn = (AFortPlayerPawnAthena*)PlayerLaunch->GetActivatingPawn();
    return Redeploy_GliderOG(Object, Stack);
}

static inline void (*K2_ActivateAbilityOG)(UObject* Object, FFrame& Stack);
static void K2_ActivateAbility(UObject* Object, FFrame& Stack)
{
    __try { K2_ActivateAbility_Impl(Object, Stack); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static void K2_ActivateAbility_Impl(UObject* Object, FFrame& Stack)
{
    auto PlayerLaunch = (UGA_WilliePete_PlayerLaunch_C*)Object;
    auto Pawn = (AFortPlayerPawnAthena*)PlayerLaunch->GetActivatingPawn();
    PlayerLaunch->OwningPawn = Pawn;
    Pawn->LaunchCharacterJump({ 0, 0, PlayerLaunch->LaunchHeightValue.GetValue() }, false, true, true, true);
    return K2_ActivateAbilityOG(Object, Stack);
}

inline void ApplySiphonEffect(AFortPlayerState* PlayerState)
{
    if (PlayerState)
    {
        UFortAbilitySystemComponent* AbilitySystemComponent = PlayerState->AbilitySystemComponent;
        if (AbilitySystemComponent)
        {
            FGameplayTag GameplayTag = FGameplayTag();
            GameplayTag.TagName = UKismetStringLibrary::Conv_StringToName(L"GameplayCue.Shield.PotionConsumed");
            AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(GameplayTag, FPredictionKey(), FGameplayEffectContextHandle());
            AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(GameplayTag, FPredictionKey(), FGameplayEffectContextHandle());
        }
    }
}

inline void GiveSiphonBonus(AFortPlayerController* PlayerController, AFortPawn* Pawn, bool bGiveBuildingResource = true, bool bHealPlayer = true)
{
    if (PlayerController)
    {
        if (bGiveBuildingResource)
        {
            UFortKismetLibrary::K2_GiveBuildingResource(PlayerController, EFortResourceType::Wood, 50);
            UFortKismetLibrary::K2_GiveBuildingResource(PlayerController, EFortResourceType::Stone, 50);
            UFortKismetLibrary::K2_GiveBuildingResource(PlayerController, EFortResourceType::Metal, 50);
        }
        if (bHealPlayer && Pawn)
        {
            float MaxHealth = Pawn->GetMaxHealth();
            float MaxShield = Pawn->GetMaxShield();
            float Health = Pawn->GetHealth();
            float Shield = Pawn->GetShield();
            float SiphonAmount = 200.0f;
            float RemainingSiphonAmount = SiphonAmount;
            if (Health < MaxHealth)
            {
                float NewHealth = std::clamp(Health + SiphonAmount, 0.0f, MaxHealth);
                Pawn->SetHealth(NewHealth);
                RemainingSiphonAmount -= (NewHealth - Health);
            }
            if (RemainingSiphonAmount > 0.0f)
            {
                float NewShield = std::clamp(Shield + RemainingSiphonAmount, 0.0f, MaxShield);
                Pawn->SetShield(NewShield);
            }
            AFortPlayerState* PlayerState = Utils::Cast<AFortPlayerState>(PlayerController->PlayerState);
            if (PlayerState)
                ApplySiphonEffect(PlayerState);
        }
    }
}

static void (*RemoveFromAlivePlayers)(UObject* GameMode, UObject* PlayerController, APlayerState* PlayerState, APawn* FinisherPawn, UFortWeaponItemDefinition* FinishingWeapon, uint8_t DeathCause, char a7) = decltype(RemoveFromAlivePlayers)(InSDKUtils::GetImageBase() + 0x206ecf0);

void (*ClientOnPawnDiedOG)(AFortPlayerControllerZone* PlayerControllerZone, const FFortPlayerDeathReport& DeathReport);


static void ClientOnPawnDied_Impl(AFortPlayerControllerZone* PlayerControllerZone, const FFortPlayerDeathReport& DeathReport)
{
    AFortPlayerPawnAthena* PlayerPawnAthena = Utils::Cast<AFortPlayerPawnAthena>(PlayerControllerZone->MyFortPawn);
    AFortPlayerStateAthena* PlayerStateAthena = Utils::Cast<AFortPlayerStateAthena>(PlayerControllerZone->PlayerState);
    if (!PlayerPawnAthena || !PlayerStateAthena)
        return;
    AFortGameModeAthena* GameModeAthena = Utils::Cast<AFortGameModeAthena>(Utils::GetGameMode());
    if (!GameModeAthena)
        return;
    AFortGameStateAthena* GameStateAthena = Utils::Cast<AFortGameStateAthena>(GameModeAthena->GameState);
    if (!GameStateAthena)
        return;
    AFortPlayerStateAthena* KillerPlayerState = Utils::Cast<AFortPlayerStateAthena>(DeathReport.KillerPlayerState);
    AFortPlayerPawnAthena* KillerPawn = Utils::Cast<AFortPlayerPawnAthena>(DeathReport.KillerPawn);
    AFortPlayerControllerAthena* KillerPlayerControllerAthena = KillerPawn ? Utils::Cast<AFortPlayerControllerAthena>(KillerPawn->Controller) : nullptr;
    AActor* DamageCauser = DeathReport.DamageCauser;
    FGameplayTagContainer TagContainer = PlayerPawnAthena ? *(FGameplayTagContainer*)(__int64(PlayerPawnAthena) + 0x15b0) : FGameplayTagContainer();
    float Distance = KillerPawn ? KillerPawn->GetDistanceTo(PlayerPawnAthena) : 0;
    EDeathCause DeathCause = AFortPlayerStateAthena::ToDeathCause(TagContainer, PlayerPawnAthena->bIsDBNO);
    FDeathInfo& DeathInfo = PlayerStateAthena->DeathInfo;
    DeathInfo.FinisherOrDowner = KillerPlayerState ? KillerPlayerState : PlayerStateAthena;
    DeathInfo.bDBNO = PlayerPawnAthena->bIsDBNO;
    DeathInfo.DeathCause = DeathCause;
    DeathInfo.Distance = (DeathCause == EDeathCause::FallDamage) ? PlayerPawnAthena->LastFallDistance : Distance;
    DeathInfo.DeathLocation = PlayerPawnAthena->K2_GetActorLocation();
    DeathInfo.bInitialized = true;
    PlayerStateAthena->PawnDeathLocation = DeathInfo.DeathLocation;
    PlayerStateAthena->OnRep_DeathInfo();
    AFortPlayerControllerAthena* PlayerControllerAthena = Utils::Cast<AFortPlayerControllerAthena>(PlayerControllerZone);
    if (PlayerControllerAthena)
    {
        if (KillerPlayerState && PlayerStateAthena != KillerPlayerState)
        {
            KillerPlayerState->KillScore++;
            KillerPlayerState->OnRep_Kills();
            KillerPlayerState->ClientReportKill(PlayerStateAthena);
            if (Globals::bEnableBackendMode && !Globals::IsTournamentModeEnabled())
            {
                std::string killerName = KillerPlayerState->GetPlayerName().ToString();
                ApplyMatchStatAsync(killerName, GetArenaKillPoints());
                ApplyMatchStatToTeam(KillerPlayerState->TeamIndex, GetArenaKillPoints());
                if (kXPPerKill > 0)
                {
                    std::thread([killerName]()
                        {
                            try { NebulaXP::AddXP(killerName, kXPPerKill); }
                            catch (...) {}
                        }).detach();
                }
            }
            if (PlayerControllerZone->MyFortPawn && ((KillerPlayerState && KillerPlayerState->Place != 1) || PlayerStateAthena->Place != 1))
            {
                static auto AccoladeId_012_Elimination = Utils::StaticFindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_012_Elimination.AccoladeId_012_Elimination");
                static auto AccoladeId_014_Elimination_Bronze = Utils::StaticFindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_014_Elimination_Bronze.AccoladeId_014_Elimination_Bronze");
                static auto AccoladeId_015_Elimination_Silver = Utils::StaticFindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_015_Elimination_Silver.AccoladeId_015_Elimination_Silver");
                static auto AccoladeId_016_Elimination_Gold = Utils::StaticFindObject<UFortAccoladeItemDefinition>("/Game/Athena/Items/Accolades/AccoladeId_016_Elimination_Gold.AccoladeId_016_Elimination_Gold");
                auto* KillerPC = Utils::Cast<AFortPlayerControllerAthena>(KillerPlayerState->Owner);
                if (KillerPC)
                {
                    switch (KillerPlayerState->KillScore)
                    {
                    case 1:
                        Challenges::GiveAccolade(KillerPC, AccoladeId_014_Elimination_Bronze);
                        break;
                    case 4:
                        Challenges::GiveAccolade(KillerPC, AccoladeId_015_Elimination_Silver);
                        break;
                    case 8:
                        Challenges::GiveAccolade(KillerPC, AccoladeId_016_Elimination_Gold);
                        break;
                    default:
                        Challenges::GiveAccolade(KillerPC, AccoladeId_012_Elimination);
                        break;
                    }
                }
            }
            float Distance = DeathInfo.Distance / 100.0f;
            static auto AccoladeId_DistanceShot = Utils::StaticFindObject<UFortAccoladeItemDefinition>(
                "/Game/Athena/Items/Accolades/AccoladeId_DistanceShot.AccoladeId_DistanceShot");
            static auto AccoladeId_051_LongShot = Utils::StaticFindObject<UFortAccoladeItemDefinition>(
                "/Game/Athena/Items/Accolades/AccoladeId_051_LongShot.AccoladeId_051_LongShot");
            static auto AccoladeId_052_LudicrousShot = Utils::StaticFindObject<UFortAccoladeItemDefinition>(
                "/Game/Athena/Items/Accolades/AccoladeId_052_LudicrousShot.AccoladeId_052_LudicrousShot");
            static auto AccoladeId_053_ImpossibleShot = Utils::StaticFindObject<UFortAccoladeItemDefinition>(
                "/Game/Athena/Items/Accolades/AccoladeId_053_ImpossibleShot.AccoladeId_053_ImpossibleShot");
            if (auto* KillerPCDist = Utils::Cast<AFortPlayerControllerAthena>(KillerPlayerState->Owner))
            {
                if (Distance >= 100.0f && Distance < 150.0f)
                    Challenges::GiveAccolade(KillerPCDist, AccoladeId_DistanceShot);
                else if (Distance >= 150.0f && Distance < 200.0f)
                    Challenges::GiveAccolade(KillerPCDist, AccoladeId_051_LongShot);
                else if (Distance >= 200.0f && Distance < 250.0f)
                    Challenges::GiveAccolade(KillerPCDist, AccoladeId_052_LudicrousShot);
                else if (Distance >= 250.0f)
                    Challenges::GiveAccolade(KillerPCDist, AccoladeId_053_ImpossibleShot);
            }
            for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
            {
                AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
                if (!TeamInfo)
                    continue;
                if (TeamInfo->Team != KillerPlayerState->TeamIndex)
                    continue;
                for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
                {
                    AFortPlayerControllerAthena* TeamMember = Utils::Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
                    if (!TeamMember)
                        continue;
                    AFortPlayerStateAthena* TeamMemberPlayerState = Utils::Cast<AFortPlayerStateAthena>(TeamMember->PlayerState);
                    if (!TeamMemberPlayerState)
                        continue;
                    TeamMemberPlayerState->TeamKillScore++;
                    TeamMemberPlayerState->OnRep_TeamKillScore();
                    TeamMemberPlayerState->ClientReportTeamKill(TeamMemberPlayerState->TeamKillScore);
                }
                break;
            }
            AFortPlayerControllerAthena* KillerPlayerController = Utils::Cast<AFortPlayerControllerAthena>(KillerPlayerState->Owner);
            if (KillerPlayerController)
                GiveSiphonBonus(KillerPlayerController, KillerPawn);
        }
        if (!GameStateAthena->IsRespawningAllowed(PlayerStateAthena) && !PlayerPawnAthena->bIsDBNO)
        {
            AFortPlayerStateAthena* CorrectKillerPlayerState = (KillerPlayerState && KillerPlayerState == PlayerStateAthena) ? nullptr : KillerPlayerState;
            UFortWeaponItemDefinition* KillerWeaponItemDefinition = nullptr;
            if (DamageCauser)
            {
                AFortProjectileBase* ProjectileBase = Utils::Cast<AFortProjectileBase>(DamageCauser);
                AFortWeapon* Weapon = Utils::Cast<AFortWeapon>(DamageCauser);
                if (ProjectileBase)
                {
                    AFortWeapon* ProjectileBaseWeapon = Utils::Cast<AFortWeapon>(ProjectileBase->Owner);
                    if (ProjectileBaseWeapon)
                        KillerWeaponItemDefinition = ProjectileBaseWeapon->WeaponData;
                }
                else if (Weapon)
                    KillerWeaponItemDefinition = Weapon->WeaponData;
                if (PlayerControllerZone->WorldInventory)
                {
                    for (auto ItemInstance : PlayerControllerZone->WorldInventory->Inventory.ItemInstances)
                    {
                        if (ItemInstance && ItemInstance->CanBeDropped())
                        {
                            const auto& ItemEntry = ItemInstance->ItemEntry;
                            Utils::SpawnPickup(
                                ItemEntry.ItemDefinition,
                                ItemEntry.Count,
                                ItemEntry.LoadedAmmo,
                                PlayerPawnAthena->K2_GetActorLocation(),
                                EFortPickupSourceTypeFlag::Player,
                                EFortPickupSpawnSource::PlayerElimination
                            );
                        }
                    }
                }
            }
            bool bMatchEnded = GameModeAthena->HasMatchEnded();
            RemoveFromAlivePlayers(UWorld::GetWorld()->AuthorityGameMode, PlayerControllerAthena, CorrectKillerPlayerState, DeathReport.KillerPawn, DeathReport.KillerWeapon, (uint8)DeathCause, 0);
            {
                auto* FreshGS = UWorld::GetWorld() ? Utils::Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState) : nullptr;
                if (!FreshGS)
                    goto ClientOnPawnDied_End;
                GameStateAthena = FreshGS;
            }
            if (GameStateAthena->GamePhase > EAthenaGamePhase::Warmup)
            {
                auto SendMatchReport = [&](AFortPlayerControllerAthena* MatchPlayerControllerAthena) -> void
                    {
                        UAthenaPlayerMatchReport* PlayerMatchReport = MatchPlayerControllerAthena->MatchReport;
                        AFortPlayerStateAthena* MatchPlayerStateAthena = Utils::Cast<AFortPlayerStateAthena>(MatchPlayerControllerAthena->PlayerState);
                        if (PlayerMatchReport && MatchPlayerStateAthena)
                        {
                            if (PlayerMatchReport->bHasTeamStats)
                            {
                                FAthenaMatchTeamStats& TeamStats = PlayerMatchReport->TeamStats;
                                TeamStats.Place = GameStateAthena->TeamsLeft;
                                TeamStats.TotalPlayers = GameStateAthena->TotalPlayers;
                                MatchPlayerControllerAthena->ClientSendTeamStatsForPlayer(TeamStats);
                            }
                            if (PlayerMatchReport->bHasMatchStats)
                            {
                                FAthenaMatchStats& MatchStats = PlayerMatchReport->MatchStats;
                                MatchPlayerControllerAthena->ClientSendMatchStatsForPlayer(MatchStats);
                            }
                            if (PlayerMatchReport->bHasRewards)
                            {
                                FAthenaRewardResult& EndOfMatchResults = PlayerMatchReport->EndOfMatchResults;
                                EndOfMatchResults.LevelsGained = 5;
                                EndOfMatchResults.BookLevelsGained = 10;
                                EndOfMatchResults.TotalSeasonXpGained = 15;
                                EndOfMatchResults.TotalBookXpGained = 20;
                                EndOfMatchResults.PrePenaltySeasonXpGained = 25;
                                MatchPlayerControllerAthena->ClientSendEndBattleRoyaleMatchForPlayer(true, EndOfMatchResults);
                            }
                            MatchPlayerStateAthena->Place = GameStateAthena->TeamsLeft;
                            MatchPlayerStateAthena->OnRep_Place();
                        }
                    };
                if (Globals::bEnableBackendMode)
                {
                    for (auto place : placements)
                    {
                        auto* PlacementGS = Utils::Cast<AFortGameStateAthena>(UWorld::GetWorld() ? UWorld::GetWorld()->GameState : nullptr);
                        if (!PlacementGS) break;
                        if (place.placementThreshold == PlacementGS->PlayersLeft)
                        {
                            auto* PlacementGM = Utils::Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);
                            if (!PlacementGM) break;
                            for (int i = 0; i < PlacementGM->AlivePlayers.Num(); i++)
                            {
                                auto player = PlacementGM->AlivePlayers[i];
                                if (!player) continue;
                                player->ClientReportTournamentPlacementPointsScored(place.placementThreshold, place.pointsScored);
                                if ((Globals::bArena || Globals::bEnableBackendMode) && player->PlayerState)
                                {
                                    std::string pname = ((AFortPlayerStateAthena*)player->PlayerState)->GetPlayerName().ToString();
                                    if (Globals::IsTournamentModeEnabled())
                                    {
                                        TournamentAccumulateMilestonePoints(pname, place.pointsScored);
                                    }
                                    else
                                    {
                                        ApplyMatchStatAsync(pname, place.pointsScored);
                                        if (place.pointsScored > 0)
                                        {
                                            std::thread([pname, pts = place.pointsScored]()
                                                {
                                                    try { NebulaXP::AddXP(pname, pts * 5); }
                                                    catch (...) {}
                                                }).detach();
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
                {
                    AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
                    if (!TeamInfo)
                        continue;
                    if (TeamInfo->Team != PlayerStateAthena->TeamIndex)
                        continue;
                    bool bIsTeamAlive = false;
                    for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
                    {
                        AFortPlayerControllerAthena* TeamMember = Utils::Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
                        if (!TeamMember || TeamMember == PlayerControllerAthena)
                            continue;
                        AFortPlayerPawn* TeamMemberPlayerPawn = Utils::Cast<AFortPlayerPawn>(TeamMember->MyFortPawn);
                        if (!TeamMemberPlayerPawn || TeamMemberPlayerPawn->bIsDying)
                            continue;
                        bIsTeamAlive = true;
                        break;
                    }
                    if (!bIsTeamAlive)
                    {
                        for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
                        {
                            AFortPlayerControllerAthena* TeamMember = Utils::Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
                            if (!TeamMember)
                                continue;
                            AFortPlayerStateAthena* TeamMemberPlayerState = Utils::Cast<AFortPlayerStateAthena>(TeamMember->PlayerState);
                            if (TeamMemberPlayerState)
                                TryFinishTournamentForPlayer(TeamMemberPlayerState, GameStateAthena->TeamsLeft + 1, false);
                            SendMatchReport(TeamMember);
                        }
                        if (Globals::bEnableBackendMode && !Globals::IsTournamentModeEnabled())
                        {
                            int32_t placementAmount = GetPlacementPoints(GameStateAthena->TeamsLeft + 1);
                            ApplyPlacementToTeam(PlayerStateAthena->TeamIndex, placementAmount);
                        }
                    }
                    break;
                }
                if (GameStateAthena->TeamsLeft == 1 && KillerPlayerState && !bMatchEnded)
                {
                    for (int32 i = 0; i < GameStateAthena->Teams.Num(); i++)
                    {
                        AFortTeamInfo* TeamInfo = GameStateAthena->Teams[i];
                        if (!TeamInfo)
                            continue;
                        if (TeamInfo->Team != KillerPlayerState->TeamIndex)
                            continue;
                        for (int32 j = 0; j < TeamInfo->TeamMembers.Num(); j++)
                        {
                            AFortPlayerControllerAthena* TeamMember = Utils::Cast<AFortPlayerControllerAthena>(TeamInfo->TeamMembers[j]);
                            if (!TeamMember)
                                continue;
                            AFortPlayerStateAthena* TeamMemberPlayerState = Utils::Cast<AFortPlayerStateAthena>(TeamMember->PlayerState);
                            if (!TeamMemberPlayerState)
                                continue;
                            TeamMemberPlayerState->Place = GameStateAthena->TeamsLeft;
                            TeamMemberPlayerState->OnRep_Place();
                            TryFinishTournamentForPlayer(TeamMemberPlayerState, 1, true);
                            TeamMember->ClientNotifyWon(KillerPawn, KillerWeaponItemDefinition, DeathCause);
                            TeamMember->ClientNotifyTeamWon(KillerPawn, KillerWeaponItemDefinition, DeathCause);
                            SendMatchReport(TeamMember);
                        }
                        if (Globals::bEnableBackendMode && !Globals::IsTournamentModeEnabled())
                        {
                            if (AwardedWinTeams.find(KillerPlayerState->TeamIndex) == AwardedWinTeams.end())
                            {
                                AwardedWinTeams.insert(KillerPlayerState->TeamIndex);
                                ApplyMatchStatToTeam(KillerPlayerState->TeamIndex, placements[0].pointsScored);
                            }
                        }
                        if (Globals::bEnableBackendMode)
                        {
                            NebulaSessions::SetStatusEnded();
                        }
                        break;
                    }
                }
            }
        }
    }
ClientOnPawnDied_End:
    ClientOnPawnDiedOG(PlayerControllerZone, DeathReport);
}

void ClientOnPawnDied(AFortPlayerControllerZone* PlayerControllerZone, const FFortPlayerDeathReport& DeathReport)
{
    __try
    {
        ClientOnPawnDied_Impl(PlayerControllerZone, DeathReport);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        printf("[ClientOnPawnDied] Excecao capturada, chamando OG como fallback.\n");
        if (ClientOnPawnDiedOG)
            ClientOnPawnDiedOG(PlayerControllerZone, DeathReport);
    }
}

__int64 (*ABuildingSMActor_PostUpdateOG)(ABuildingSMActor*);
__int64 __fastcall ABuildingSMActor_PostUpdate(ABuildingSMActor* Actor)
{
    __int64 _r{};
    __try { _r = ABuildingSMActor_PostUpdate_Impl(Actor); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

__int64 __fastcall ABuildingSMActor_PostUpdate_Impl(ABuildingSMActor* Actor)
{
    if (auto BuildingContainer = Utils::Cast<ABuildingContainer>(Actor))
    {
        if (BuildingContainer->bStartAlreadySearched_Athena == 1)
        {
            if (BuildingContainer->K2_GetActorLocation().Z == 0)
                return 0;
            TArray<AActor*> WarmupActors;
            static UClass* WarmupClass = Utils::StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_Warmup.Tiered_Athena_FloorLoot_Warmup_C");
            UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), WarmupClass, &WarmupActors);
            for (auto& WarmupActor : WarmupActors)
            {
                auto Container = (ABuildingContainer*)WarmupActor;
                Container->BP_SpawnLoot(nullptr);
                Container->K2_DestroyActor();
            }
            WarmupActors.Free();
            WarmupClass = Utils::StaticLoadObject<UClass>("/Game/Athena/Environments/Blueprints/Tiered_Athena_FloorLoot_01.Tiered_Athena_FloorLoot_01_C");
            UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), WarmupClass, &WarmupActors);
            for (auto& WarmupActor : WarmupActors)
            {
                auto Container = (ABuildingContainer*)WarmupActor;
                Container->BP_SpawnLoot(nullptr);
                Container->K2_DestroyActor();
            }
            WarmupActors.Free();
        }
    }
    return ABuildingSMActor_PostUpdateOG(Actor);
}

void (*ServerReturnToMainMenuOG)(AFortPlayerController* PlayerController);
void ServerReturnToMainMenu(AFortPlayerControllerAthena* PC)
{
    __try { ServerReturnToMainMenu_Impl(PC); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void ServerReturnToMainMenu_Impl(AFortPlayerControllerAthena* PC)
{
    if (!PC)
        return;
    if (PC->Pawn)
        PC->ServerSuicide();
    PC->ClientTravel(L"/Game/Maps/Frontend", ETravelType::TRAVEL_Absolute, false, FGuid());
    ServerReturnToMainMenuOG(PC);
}

static inline void(*ServerSetInAircraftOG)(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft);
void ServerSetInAircraft(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft)
{
    __try { ServerSetInAircraft_Impl(PlayerState, bNewInAircraft); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void ServerSetInAircraft_Impl(AFortPlayerStateAthena* PlayerState, bool bNewInAircraft)
{
    if (!PlayerState)
        return ServerSetInAircraftOG(PlayerState, bNewInAircraft);
    if (Globals::bArena)
    {
        //ServerSetInAircraftOG(PlayerState, bNewInAircraft);
        if (bNewInAircraft)
            TryAwardBusfare(PlayerState);
        //return;
    }
    AFortPlayerControllerAthena* PC = Utils::Cast<AFortPlayerControllerAthena>(PlayerState->Owner);
    if (!Globals::bLateGame)
    {
        if (!Globals::bCreative || !Globals::bEnableBattleLab)
        {
            if (PC && PC->WorldInventory)
            {
                for (int i = PC->WorldInventory->Inventory.ReplicatedEntries.Num() - 1; i >= 0; i--)
                {
                    auto ItemDef = Utils::Cast<UFortWorldItemDefinition>(PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemDefinition);
                    if (!ItemDef)
                        continue;
                    if (ItemDef->bCanBeDropped)
                        Inventory::Remove(PC, PC->WorldInventory->Inventory.ReplicatedEntries[i].ItemGuid);
                }
            }
        }
    }
    if (bNewInAircraft)
        TryAwardBusfare(PlayerState);

    if (Globals::bEnableBackendMode)
    {
        auto World = UWorld::GetWorld();
        auto GM = World ? Utils::Cast<AFortGameModeAthena>(World->AuthorityGameMode) : nullptr;
        if (GM)
        {
            int alive = GM->AlivePlayers.Num();
            NebulaSessions::SetPlayerCount(alive);
        }
    }
    ServerSetInAircraftOG(PlayerState, bNewInAircraft);
}

void PlayerController::ServerClientIsReadyToRespawn(AFortPlayerControllerAthena* PC)
{
    __try { ServerClientIsReadyToRespawn_Impl(PC); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerClientIsReadyToRespawn_Impl(AFortPlayerControllerAthena* PC)
{
    if (!PC || !PC->PlayerState)
        return;
    auto PS = PC->PlayerState->Cast<AFortPlayerStateAthena>();
    if (!PS)
        return;
    if (PS->RespawnData.bRespawnDataAvailable && PS->RespawnData.bServerIsReady)
    {
        PS->RespawnData.bClientIsReady = true;
        FTransform Transform{};
        Transform.Rotation = Utils::RotatorToQuat(PS->RespawnData.RespawnRotation);
        Transform.Translation = PS->RespawnData.RespawnLocation;
        Transform.Scale3D = FVector(1.0f, 1.0f, 1.0f);
        auto Pawn = (AFortPlayerPawnAthena*)AFortGameModeAthena::Get()->SpawnDefaultPawnAtTransform(PC, Transform);
        PC->Possess(Pawn);
        Pawn->SetMaxHealth(100);
        Pawn->SetMaxShield(100);
        Pawn->SetHealth(100);
        Pawn->SetShield(100);
        Pawn->bCanBeDamaged = true;
        PC->RespawnPlayerAfterDeath(true);
        Pawn->BeginSkydiving(true);
    }
}

void PlayerController::ServerPlayEmoteItemHook(AFortPlayerController* PlayerController, UFortItemDefinition* EmoteAsset, float RandomEmoteNumber)
{
    __try { ServerPlayEmoteItemHook_Impl(PlayerController, EmoteAsset, RandomEmoteNumber); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerPlayEmoteItemHook_Impl(AFortPlayerController* PlayerController, UFortItemDefinition* EmoteAsset, float RandomEmoteNumber)
{
    if (!PlayerController || !EmoteAsset || !PlayerController->MyFortPawn)
        return;
    auto ASC = ((AFortPlayerStateAthena*)PlayerController->PlayerState)->AbilitySystemComponent;
    FGameplayAbilitySpec NewSpec;
    UClass* Ability = nullptr;
    if (auto Spray = EmoteAsset->Cast<UAthenaSprayItemDefinition>())
        Ability = UGAB_Spray_Generic_C::StaticClass();
    else if (auto Toy = EmoteAsset->Cast<UAthenaToyItemDefinition>())
        Ability = Toy->ToySpawnAbility.Get();
    else if (auto Emote = EmoteAsset->Cast<UAthenaDanceItemDefinition>())
    {
        auto DA = Emote->CustomDanceAbility.Get();
        Ability = DA ? DA : UGAB_Emote_Generic_C::StaticClass();
        PlayerController->MyFortPawn->bMovingEmote = Emote->bMovingEmote;
        PlayerController->MyFortPawn->bMovingEmoteForwardOnly = Emote->bMoveForwardOnly;
        PlayerController->MyFortPawn->EmoteWalkSpeed = Emote->WalkForwardSpeed;
    }
    if (Ability)
    {
        FGameplayAbilitySpec Spec{};
        FGameplayAbilitySpec* (*FGameplayAbilitySpecCtor)(FGameplayAbilitySpec * Spec, UGameplayAbility * Ability, int Level, int InputID, UObject * SourceObject) = decltype(FGameplayAbilitySpecCtor)(__int64(ImageBase + 0xa27b60));
        FGameplayAbilitySpecCtor(&Spec, (UGameplayAbility*)Ability->DefaultObject, 1, -1, EmoteAsset);
        FGameplayAbilitySpecHandle(*GiveAbilityAndActivateOnce)(UAbilitySystemComponent * ASC, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec, __int64) = decltype(GiveAbilityAndActivateOnce)(__int64(ImageBase + 0xa4bc50));
        GiveAbilityAndActivateOnce(((AFortPlayerStateAthena*)PlayerController->PlayerState)->AbilitySystemComponent, &Spec.Handle, Spec, 0);
    }
}

void PlayerController::MovingEmoteStopped(UObject* Context, FFrame& Stack)
{
    Stack.IncrementCode();
    AFortPawn* Pawn = (AFortPawn*)Context;
    Pawn->bMovingEmote = false;
    Pawn->bMovingEmoteFollowingOnly = false;
}

void (*ServerLoadingScreenDroppedOG)(AFortPlayerControllerAthena* PC);

static void ServerLoadingScreenDropped_Impl(AFortPlayerControllerAthena* PC)
{
    if (!PC) return;
    if (PC->IsA(AFortLiveBroadcastController::StaticClass()))
        return;

    APlayerState* RawPS = PC->PlayerState;
    if (!RawPS) { ServerLoadingScreenDroppedOG(PC); return; }

    std::string joinName = RawPS->GetPlayerName().ToString();
    if (joinName.empty()) { ServerLoadingScreenDroppedOG(PC); return; }

    if (PlayersThatJoined.count(joinName))
    {
        ServerLoadingScreenDroppedOG(PC); return;
    }
    PlayersThatJoined.insert(joinName);

    AFortPlayerStateAthena* PS = Utils::Cast<AFortPlayerStateAthena>(RawPS);

    NebulaPlayerData::FetchDivisionAsync(joinName);
    TryStartTournamentForPlayer(PS);

    if (Globals::IsArenaModeEnabled() || (Globals::bEnableBackendMode && !Globals::IsTournamentModeEnabled()))
    {
        if (gBusfareDone.find(joinName) == gBusfareDone.end())
        {
            gBusfareDone.insert(joinName);
            int pDiv = NebulaPlayerData::GetPlayerDivision(joinName);
            int32_t bFare = GetArenaBusFare(pDiv);
            if (bFare != 0)
                ApplyMatchStatAsync(joinName, bFare);
        }
    }

    ServerLoadingScreenDroppedOG(PC);
}

void ServerLoadingScreenDropped(AFortPlayerControllerAthena* PC)
{
    __try
    {
        ServerLoadingScreenDropped_Impl(PC);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        printf("[ServerLoadingScreenDropped] Excecao 0x%X capturada — player em estado invalido, servidor protegido.\n",
            GetExceptionCode());
    }
}

void PlayerController::ServerCheat(AFortPlayerControllerAthena* PC, FString Msg)
{
    __try { ServerCheat_Impl(PC, Msg); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

void PlayerController::ServerCheat_Impl(AFortPlayerControllerAthena* PC, FString Msg)
{
    string CheatStr = Msg.ToString();
    if (CheatStr == "StartEvent")
    {
        UObject* Test = UObject::FindObject<UObject>("BP_NewYearTimer_C Athena_NYE_Celebration.Athena_NYE_Celebration.PersistentLevel.BP_NewYearTimer_2");
        Test->ProcessEvent(Test->Class->GetFunction("BP_NewYearTimer_C", "startNYE"), nullptr);
    }
    else if (CheatStr == "StartCreativeGame")
    {
        AFortMinigame* MG = PC->GetMinigame();
        if (MG)
            MG->AdvanceState();
    }
    else if (CheatStr == "startaircraft")
        UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("startaircraft"), nullptr);
    else if (CheatStr == "pausesafezone")
        UKismetSystemLibrary::GetDefaultObj()->ExecuteConsoleCommand(UWorld::GetWorld(), TEXT("pausesafezone"), nullptr);
    else if (CheatStr.contains("give "))
    {
        string wid = Utils::SplitString(true, "give ", CheatStr);
        UFortWeaponItemDefinition* WeaponDef = UObject::FindObject<UFortWeaponItemDefinition>(wid + "." + wid);
        if (WeaponDef)
        {
            Inventory::GiveItem(PC, WeaponDef, 1, wid.contains("WID_Hook_Gun_Slide") || wid.contains("WID_Athena_HappyGhost") || wid.contains("WID_Hook_Gun_VR_Ore_T03") ? 10 : 0);
            if ((!wid.contains("WID_Hook_Gun_Slide") && !wid.contains("WID_Athena_HappyGhost") && !wid.contains("WID_Hook_Gun_VR_Ore_T03")) && WeaponDef->GetAmmoWorldItemDefinition_BP() && WeaponDef->GetAmmoWorldItemDefinition_BP() != WeaponDef)
                Inventory::GiveItem(PC, WeaponDef->GetAmmoWorldItemDefinition_BP(), 999);
            else
                Inventory::GiveItem(PC, WeaponDef->GetAmmoWorldItemDefinition_BP(), 999);
        }
    }
}

void PlayerController::InitFortPlayerController()
{
    Utils::SwapVFTs(AFortPlayerControllerAthena::StaticClass()->DefaultObject, 0x110, ServerAcknowledgePossessionHook, (LPVOID*)&ServerAcknowledgePossessionOG);
    Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x220, ServerAttemptInventoryDrop, nullptr);
    Utils::SwapVFTs(AFortPlayerControllerAthena::StaticClass()->DefaultObject, 0x210, ServerExecuteInventoryItemHook, nullptr);
    Utils::SwapVFTs(UFortControllerComponent_Aircraft::StaticClass()->DefaultObject, 0x8E, ServerAttemptAircraftJumpHook, nullptr);
    Utils::SwapVFTs(UFortControllerComponent_Interaction::StaticClass()->DefaultObject, 0x90, ServerAttemptInteract, (LPVOID*)&ServerAttemptInteractOG);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2b69620), GetPlayerViewPoint, (LPVOID*)&GetPlayerViewPointOG);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x32c36e0), ClientOnPawnDied, (LPVOID*)&ClientOnPawnDiedOG);
    Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x26C, ServerReadyToStartMatch, (LPVOID*)&ServerReadyToStartMatchOG);
    Utils::ExecHook("/Game/Athena/Items/EnvironmentalItems/HidingProps/Props/GA_WilliePete_PlayerLaunch.GA_WilliePete_PlayerLaunch_C.K2_ActivateAbility", K2_ActivateAbility, K2_ActivateAbilityOG);
    Utils::ExecHook("/Game/Athena/Items/EnvironmentalItems/HidingProps/Props/GA_WilliePete_PlayerLaunch.GA_WilliePete_PlayerLaunch_C.Redeploy Glider", Redeploy_Glider, Redeploy_GliderOG);
    Utils::HookVTable(AFortPlayerControllerAthena::GetDefaultObj(), 0x268, ServerReturnToMainMenu, (PVOID*)&ServerReturnToMainMenuOG);
    Utils::HookVTable(AFortPlayerStateAthena::StaticClass()->DefaultObject, 0xFC, ServerSetInAircraft, (LPVOID*)&ServerSetInAircraftOG);
    Utils::ExecHook("/Script/FortniteGame.FortPlayerControllerAthena.ServerClientIsReadyToRespawn", ServerClientIsReadyToRespawn);
    Utils::ExecHook("/Script/FortniteGame.FortPawn.MovingEmoteStopped", MovingEmoteStopped);
    Utils::SwapVFTs(AFortPlayerControllerAthena::StaticClass()->DefaultObject, 0x1CC, ServerPlayEmoteItemHook, nullptr);
    Utils::SwapVFTs(AFortPlayerControllerAthena::StaticClass()->DefaultObject, 0x26E, ServerLoadingScreenDropped, (PVOID*)&ServerLoadingScreenDroppedOG);
    Utils::SwapVFTs(AAthena_PlayerController_C::StaticClass()->DefaultObject, 0x1CA, ServerCheat, nullptr);
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x206af40), ABuildingSMActor_PostUpdate, (LPVOID*)&ABuildingSMActor_PostUpdateOG);
}