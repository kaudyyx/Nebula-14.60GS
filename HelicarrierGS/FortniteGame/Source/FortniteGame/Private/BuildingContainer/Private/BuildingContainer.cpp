#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/BuildingContainer/Public/BuildingContainer.h>

int Looting::GetClipSize(UFortItemDefinition* Definition)
{
    if (!Definition)
        return 0;
    if (auto WeaponDef = Utils::Cast<UFortWeaponRangedItemDefinition>(Definition)) {
        for (auto& RowPair : WeaponDef->WeaponStatHandle.DataTable->RowMap) {
            if (RowPair.First == WeaponDef->WeaponStatHandle.RowName) {
                return ((FFortRangedWeaponStats*)RowPair.Second)->ClipSize;
            }
        }
    }

    return 0;
}


void Looting::SetupLDSForPackage(TArray<FFortItemEntry>& LootDrops, SDK::FName Package, int i, std::string& TierGroup, int WorldLevel = ((AFortGameStateAthena*)UWorld::GetWorld()->GameState)->WorldLevel) {
    TArray<FFortLootPackageData*> LPGroups;
    for (auto const& Val : LPGroupsAll)
    {
        if (!Val)
            continue;

        if (Val->LootPackageID != Package)
            continue;
        if (i != -1 && Val->LootPackageCategory != i)
            continue;
        if (WorldLevel >= 0) {
            if (Val->MaxWorldLevel >= 0 && WorldLevel > Val->MaxWorldLevel)
                continue;
            if (Val->MinWorldLevel >= 0 && WorldLevel < Val->MinWorldLevel)
                continue;
        }

        LPGroups.Add(Val);
    }
    if (LPGroups.Num() == 0)
        return;

    auto LootPackage = PickWeighted(LPGroups, [](float Total) { return ((float)rand() / 32767.f) * Total; });
    if (!LootPackage)
        return;

    if (LootPackage->LootPackageCall.Num() > 1)
    {
        for (int i = 0; i < LootPackage->Count; i++)
            SetupLDSForPackage(LootDrops, UKismetStringLibrary::Conv_StringToName(LootPackage->LootPackageCall), 0, TierGroup);

        return;
    }

    auto ItemDefinition = Utils::Cast<UFortWorldItemDefinition>(LootPackage->ItemDefinition.NewGet());
    if (!ItemDefinition)
        return;
    bool found = false;
    for (auto& LootDrop : LootDrops) {
        if (LootDrop.ItemDefinition == ItemDefinition) {
            LootDrop.Count += LootPackage->Count;
            if (LootDrop.Count > ItemDefinition->MaxStackSize.Value) {
                auto OGCount = LootDrop.Count;
                LootDrop.Count = ItemDefinition->MaxStackSize.Value;

                if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary) LootDrops.Add(*Inventory::MakeItemEntry(ItemDefinition, OGCount - ItemDefinition->MaxStackSize.Value, std::clamp(Inventory::GetLevel(ItemDefinition->LootLevelData), ItemDefinition->MinLevel, ItemDefinition->MaxLevel)));
            }
            if (Inventory::GetQuickbar(LootDrop.ItemDefinition) == EFortQuickBars::Secondary) found = true;
        }
    }

    if (!found && LootPackage->Count > 0) {
        LootDrops.Add(*Inventory::MakeItemEntry(ItemDefinition, LootPackage->Count, std::clamp(Inventory::GetLevel(ItemDefinition->LootLevelData), ItemDefinition->MinLevel, ItemDefinition->MaxLevel)));
    }
}

void Looting::SetupLootGroups(AFortGameStateAthena* GameState, bool bTierDataGroups)
{
    if (!GameState) return;

    if (bTierDataGroups) {
        LPGroupsAll.Free();
        TierDataAllGroups.Free();
    }
    else {
        if (LPGroupsAll.Num() > 0 && TierDataAllGroups.Num() > 0)
            return;
    }

    static UDataTable* LootPackages = nullptr;
    static UDataTable* LootTierData = nullptr;

    if (!LootPackages || !LootTierData) {
        LootPackages = GameState->CurrentPlaylistInfo.BasePlaylist->LootPackages.NewGet();
        LootTierData = GameState->CurrentPlaylistInfo.BasePlaylist->LootTierData.NewGet();

        if (!LootPackages || !LootTierData)
        {
            LootPackages = Utils::StaticLoadObject<UDataTable>("/Game/Items/DataTables/AthenaLootTierData_Client.AthenaLootTierData_Client");
            LootTierData = Utils::StaticLoadObject<UDataTable>("/Game/Items/DataTables/AthenaLootPackages_Client.AthenaLootPackages_Client");
        }
    }
    // fix for LootPackages->RowMap is on Unreal Containers at TMap need this operators
    /*template<typename NewValueType>
    operator TMap<KeyElementType, NewValueType*>()
    {
        return *(TMap<KeyElementType, NewValueType*> *) this;
    }

    template<typename NewValueType>
    operator TMap<KeyElementType, NewValueType*>() const
    {
        return *(TMap<KeyElementType, NewValueType*> *) this;
    }*/
    if (LootPackages)
    {
        for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>)LootPackages->RowMap) {
            LPGroupsAll.Add(Val);
        }
    }
    if (LootTierData) {
        for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) LootTierData->RowMap) {
            if (!Val) continue;
            TierDataAllGroups.Add(Val);
        }
    }

    auto GameFeatureDataArray = GetObjectsOfClass2<UFortGameFeatureData>();
    for (const auto& GameFeatureData : GameFeatureDataArray) {
        bool bAppliedPlaylistData = false;
        auto LootTableData = GameFeatureData->DefaultLootTableData;
        for (int i = 0; i < Utils::GetCurrentPlaylist()->GameplayTagContainer.GameplayTags.Num(); i++) {
            auto& Tag = Utils::GetCurrentPlaylist()->GameplayTagContainer.GameplayTags[i];

            for (auto& [CurrentOverideTag, Val] : GameFeatureData->PlaylistOverrideLootTableData)
            {
                if (Tag == CurrentOverideTag) { // to fix this == error is on FGameplayTag
                    UDataTable* PlaylistLootPackages = Val.LootPackageData.NewGet();
                    UDataTable* PLaylistLootTierData = Val.LootTierData.NewGet();
                    if (PlaylistLootPackages && PLaylistLootTierData) {
                        for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>)PlaylistLootPackages->RowMap) {
                            LPGroupsAll.Add(Val);
                        }
                        for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) PLaylistLootTierData->RowMap) {
                            if (!Val) continue;
                            TierDataAllGroups.Add(Val);
                        }
                    }
                }
            }
        }
        if (auto LootPackageData = LootTableData.LootPackageData.NewGet())
        {
            for (auto& [Key, Val] : (TMap<FName, FFortLootPackageData*>)LootPackageData->RowMap) {
                LPGroupsAll.Add(Val);
            }
        }
        if (auto LootTierDataTable = LootTableData.LootTierData.NewGet()) {
            for (auto& [Key, Val] : (TMap<FName, FFortLootTierData*>) LootTierDataTable->RowMap) {
                if (!Val) continue;
                TierDataAllGroups.Add(Val);
            }
        }
    }
}

TArray<FFortItemEntry> Looting::PickLootDrops(std::string& TierGroupName, bool bSetupTables, int LootTier, int WorldLevel) {

    SetupLootGroups(((AFortGameStateAthena*)UWorld::GetWorld()->GameState), bSetupTables);

    if (TierDataAllGroups.Num() == 0 || LPGroupsAll.Num() == 0) {
        std::cout << "No Loot Groups or Tier Data found!" << std::endl;
        return {};
    }


    TArray<FFortLootTierData*> TierDataGroups;
    for (auto const& Val : TierDataAllGroups) {
        if (!Val) continue;
        std::string ValTierGroup = Val->TierGroup.IsValid() ? Val->TierGroup.ToString() : "";
        if (ValTierGroup == TierGroupName && (LootTier == -1 ? true : LootTier == Val->LootTier))
            TierDataGroups.Add(Val);
    }

    if (TierDataGroups.Num() == 0)
        return {};

    auto LootTierData = PickWeighted(TierDataGroups, [](float Total) { return ((float)rand() / 32767.f) * Total; });
    if (!LootTierData)
        return {};

    float DropCount = 0;
    if (LootTierData->NumLootPackageDrops > 0) {
        DropCount = LootTierData->NumLootPackageDrops < 1 ? 1 : (float)((int)((LootTierData->NumLootPackageDrops * 2) - .5f) >> 1);

        if (LootTierData->NumLootPackageDrops > 1) {
            float idk = LootTierData->NumLootPackageDrops - DropCount;

            if (idk > 0.0000099999997f)
                DropCount += idk >= ((float)rand() / 32767);
        }
    }

    float AmountOfLootDrops = 0;
    float MinLootDrops = 0;

    for (auto& Min : LootTierData->LootPackageCategoryMinArray)
        AmountOfLootDrops += Min;

    int SumWeights = 0;

    for (int i = 0; i < LootTierData->LootPackageCategoryWeightArray.Num(); ++i)
        if (LootTierData->LootPackageCategoryWeightArray[i] > 0 && LootTierData->LootPackageCategoryMaxArray[i] != 0)
            SumWeights += LootTierData->LootPackageCategoryWeightArray[i];

    while (SumWeights > 0)
    {
        AmountOfLootDrops++;

        if (AmountOfLootDrops >= LootTierData->NumLootPackageDrops) {
            AmountOfLootDrops = AmountOfLootDrops;
            break;
        }

        SumWeights--;
    }

    if (!AmountOfLootDrops)
        AmountOfLootDrops = AmountOfLootDrops;

    TArray<FFortItemEntry> LootDrops;

    for (int i = 0; i < AmountOfLootDrops && i < LootTierData->LootPackageCategoryMinArray.Num(); i++)
        for (int j = 0; j < LootTierData->LootPackageCategoryMinArray[i] && LootTierData->LootPackageCategoryMinArray[i] >= 1; j++)
            SetupLDSForPackage(LootDrops, LootTierData->LootPackage, i, TierGroupName, WorldLevel);

    std::map<UFortWorldItemDefinition*, int32> AmmoMap;
    for (auto& Item : LootDrops)
        if (Item.ItemDefinition->IsA<UFortWeaponRangedItemDefinition>() && !Item.ItemDefinition->IsStackable() && ((UFortWorldItemDefinition*)Item.ItemDefinition)->GetAmmoWorldItemDefinition_BP())
        {
            auto AmmoDefinition = ((UFortWorldItemDefinition*)Item.ItemDefinition)->GetAmmoWorldItemDefinition_BP();
            int i = 0;
            auto AmmoEntry = LootDrops.Search([&](FFortItemEntry& Entry)
                {
                    if (AmmoMap[AmmoDefinition] > 0 && i < AmmoMap[AmmoDefinition])
                    {
                        i++;
                        return false;
                    }
                    AmmoMap[AmmoDefinition]++;
                    return Entry.ItemDefinition == AmmoDefinition;
                });

            if (AmmoEntry)
                continue;

            FFortLootPackageData* Group = nullptr;
            static auto AmmoSmall = UKismetStringLibrary::Conv_StringToName(L"WorldList.AthenaAmmoSmall");
            for (auto const& Val : LPGroupsAll)
                if (Val->LootPackageID == AmmoSmall && Val->ItemDefinition.NewGet() == AmmoDefinition)
                {
                    Group = Val;
                    break;
                }

            if (Group)
                LootDrops.Add(*Inventory::MakeItemEntry(AmmoDefinition, Group->Count, 0));
        }

    return LootDrops;
}


bool Looting::SpawnLoot(ABuildingContainer* Container)
{
    bool _r = false;
    __try { _r = SpawnLoot_Impl(Container); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
    return _r;
}

bool Looting::SpawnLoot_Impl(ABuildingContainer* Container)
{

    /*if (Globals::bLateGame)
        return false;*/

    auto GameState = Utils::Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState);
    auto GameMode = Utils::Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);
    if (!GameState || !GameMode || !Container || Container->bAlreadySearched)
        return false;

    FName LootTierGroupToUse = Container->SearchLootTierGroup;

    for (auto& [SupportTierGroup, Redirect] : GameMode->RedirectAthenaLootTierGroups) {
        if (LootTierGroupToUse == SupportTierGroup) {
            LootTierGroupToUse = Redirect;
            break;
        }
    }

    //printf("LootTierGroupToUse: %s", LootTierGroupToUse.ToString().c_str());

    FVector Location = Container->K2_GetActorLocation() +
        (Container->GetActorForwardVector() * Container->LootSpawnLocation_Athena.X) +
        (Container->GetActorRightVector() * Container->LootSpawnLocation_Athena.Y) +
        (Container->GetActorUpVector() * Container->LootSpawnLocation_Athena.Z);

    std::string LootTierGroupStr = LootTierGroupToUse.ToString();

    for (const FFortItemEntry& Entry : Looting::PickLootDrops(LootTierGroupStr, !LootTierGroupToUse.ToString().contains("Loot_AthenaFloorLoot"))) {
        FSpawnPickupData Data{};
        Data.ItemDefinition = Entry.ItemDefinition;
        Data.Count = Entry.Count;
        Data.Location = Location;
        Data.FortPickupSourceTypeFlag = EFortPickupSourceTypeFlag::Container;
        Data.FortPickupSpawnSource = EFortPickupSpawnSource::Unset;
        Inventory::SpawnPickup67(Data);
    }

    Container->bAlreadySearched = true;
    Container->OnRep_bAlreadySearched();
    Container->SearchBounceData.SearchAnimationCount++;
    Container->BounceContainer();

    return Container;
}

//finishing

AFortPickup* Looting::SpawnPickup(UObject* Object, FFrame& Stack, AFortPickup** Ret) {
    UFortWorldItemDefinition* ItemDefinition;
    int32 NumberToSpawn;
    AFortPawn* TriggeringPawn;
    FVector Position;
    FVector Direction;
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&NumberToSpawn);
    Stack.StepCompiledIn(&TriggeringPawn);
    Stack.StepCompiledIn(&Position);
    Stack.StepCompiledIn(&Direction);
    Stack.IncrementCode();

    auto Pickup = Inventory::SpawnPickup(Position, ItemDefinition, NumberToSpawn, ItemDefinition->IsA<UFortWeaponItemDefinition>() ? Inventory::GetStats((UFortWeaponItemDefinition*)ItemDefinition)->ClipSize : 0, EFortPickupSourceTypeFlag::Other, EFortPickupSpawnSource::SupplyDrop);
    return *Ret = Pickup;
}


AFortPickup* Looting::K2_SpawnPickupInWorld(UObject* Object, FFrame& Stack, AFortPickup** Ret) {
    class UObject* WorldContextObject;
    class UFortWorldItemDefinition* ItemDefinition;
    int32 NumberToSpawn;
    FVector Position;
    FVector Direction;
    int32 OverrideMaxStackCount;
    bool bToss;
    bool bRandomRotation;
    bool bBlockedFromAutoPickup;
    int32 PickupInstigatorHandle;
    EFortPickupSourceTypeFlag SourceType;
    EFortPickupSpawnSource Source;
    class AFortPlayerController* OptionalOwnerPC;
    bool bPickupOnlyRelevantToOwner;

    Stack.StepCompiledIn(&WorldContextObject);
    Stack.StepCompiledIn(&ItemDefinition);
    Stack.StepCompiledIn(&NumberToSpawn);
    Stack.StepCompiledIn(&Position);
    Stack.StepCompiledIn(&Direction);
    Stack.StepCompiledIn(&OverrideMaxStackCount);
    Stack.StepCompiledIn(&bToss);
    Stack.StepCompiledIn(&bRandomRotation);
    Stack.StepCompiledIn(&bBlockedFromAutoPickup);
    Stack.StepCompiledIn(&PickupInstigatorHandle);
    Stack.StepCompiledIn(&SourceType);
    Stack.StepCompiledIn(&Source);
    Stack.StepCompiledIn(&OptionalOwnerPC);
    Stack.StepCompiledIn(&bPickupOnlyRelevantToOwner);
    Stack.IncrementCode();

    auto Pickup = Inventory::SpawnPickup(Position, ItemDefinition, NumberToSpawn, 0, SourceType, Source, OptionalOwnerPC ? OptionalOwnerPC->MyFortPawn : nullptr, bToss, bRandomRotation);
    return *Ret = Pickup;
}

AFortPickup* Looting::SpawnItemVariantPickupInWorld(UObject* Object, FFrame& Stack, AFortPickup** Ret) {
    UObject* WorldContextObject;
    FSpawnItemVariantParams Params;
    Stack.StepCompiledIn(&WorldContextObject);
    Stack.StepCompiledIn(&Params);
    Stack.IncrementCode();

    auto Pickup = Inventory::SpawnPickup(Params.position, Params.WorldItemDefinition, Params.NumberToSpawn, 0, Params.SourceType, Params.Source, nullptr, Params.bToss, Params.bRandomRotation);
    return *Ret = Pickup;
}


void (*OnAuthorityRandomUpgradeAppliedOG)(ABuildingContainer*, const FName&);
void OnAuthorityRandomUpgradeApplied(ABuildingContainer* Container, const FName& UpgradeTierGroup)
{
 static bool WorldSetup = false;
    if (!WorldSetup)
    {
        WorldSetup = true;

        TArray<AActor*> BuildingContainers;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), ABuildingContainer::StaticClass(), &BuildingContainers);

        for (AActor*& UpgradeContainer : BuildingContainers)
        {
            ABuildingContainer* Container = (ABuildingContainer*)UpgradeContainer;

            if (!Container)
                continue;

            if (Container->GetName().contains("Chest") && !Container->GetName().contains("Faction"))
            {
                if ((std::rand() % 100) < 25)
                {
                    if (Container->PotentialRandomUpgrades.Num() > 0)
                    {
                        auto& MeshSet = Container->AlternateMeshes[0].MeshSets[0];

                        Container->SearchLootTierGroup = Container->PotentialRandomUpgrades[0].LootTierGroupIfApplied;
                        Container->StaticMesh = MeshSet.BaseMesh;
                        Container->StaticMeshComponent->SetStaticMesh(Container->StaticMesh);
                        Container->SearchedMesh = MeshSet.SearchedMesh;
                        Container->SearchSpeed = MeshSet.SearchSpeed;
                        Container->DeathSound = MeshSet.DeathSound;

                        Container->ForceNetUpdate();
                    }
                }
            }


            if (Container->GetName().contains("Ammo"))
            {
                if ((std::rand() % 100) < 40)
                {
                    if (Container->PotentialRandomUpgrades.Num() > 0)
                    {
                        auto& MeshSet = Container->AlternateMeshes[0].MeshSets[0];

                        Container->SearchLootTierGroup = Container->PotentialRandomUpgrades[0].LootTierGroupIfApplied;
                        Container->StaticMesh = MeshSet.BaseMesh;
                        Container->StaticMeshComponent->SetStaticMesh(Container->StaticMesh);
                        Container->SearchedMesh = MeshSet.SearchedMesh;
                        Container->SearchSpeed = MeshSet.SearchSpeed;
                        Container->DeathSound = MeshSet.DeathSound;

                        Container->ForceNetUpdate();
                    }
                }
            }

        }

        BuildingContainers.Free();
    }
    OnAuthorityRandomUpgradeAppliedOG(Container, UpgradeTierGroup);
}

void Looting::InitLooting()
{
    MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2302D90), SpawnLoot, nullptr);
    Utils::ExecHook(("/Script/FortniteGame.BuildingContainer.OnAuthorityRandomUpgradeApplied"), OnAuthorityRandomUpgradeApplied, OnAuthorityRandomUpgradeAppliedOG);
}