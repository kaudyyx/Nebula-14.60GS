#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>

#include <vector>
#include <map>
#include <numeric>

class Looting
{
public:
    template<typename T>
    static T* GetWeightedRow(const std::vector<T*>& Rows)
    {
        if (Rows.empty())
            return nullptr;

        float TotalWeight = 0.0f;

        for (const T* Row : Rows)
            if (Row)
                TotalWeight += Row->Weight;

        if (TotalWeight <= 0.0f)
            return nullptr;

        float RandomPoint = UKismetMathLibrary::RandomFloat() * TotalWeight;
        float CumProb = 0.0f;

        for (T* Row : Rows)
        {
            if (!Row) continue;

            CumProb += Row->Weight;
            if (RandomPoint <= CumProb)
                return Row;
        }

        return Rows[0];
    }

    template <typename T>
    static T* PickWeighted(TArray<T*>& Map, float (*RandFunc)(float), bool bCheckZero = true)
    {
        float TotalWeight = std::accumulate(
            Map.begin(),
            Map.end(),
            0.0f,
            [&](float acc, T*& p) { return acc + p->Weight; }
        );

        float RandomNumber = RandFunc(TotalWeight);

        for (auto& Element : Map)
        {
            float Weight = Element->Weight;

            if (bCheckZero && Weight == 0)
                continue;

            if (RandomNumber <= Weight)
                return Element;

            RandomNumber -= Weight;
        }

        return nullptr;
    }

    static inline TArray<FFortLootTierData*> TierDataAllGroups;
    static inline TArray<FFortLootPackageData*> LPGroupsAll;

    static void SetupLootGroups(AFortGameStateAthena* GameState, bool bTierDataGroups);
    static void SetupLDSForPackage(TArray<FFortItemEntry>& LootDrops, FName Package, int i, std::string& TierGroup, int WorldLevel);
    static TArray<FFortItemEntry> PickLootDrops(std::string& TierGroupName, bool bSetupTables = false, int LootTier = -1, int WorldLevel = 0);

    static bool SpawnLoot(ABuildingContainer* Container);
    static bool SpawnLoot_Impl(ABuildingContainer* Container);

    static int GetClipSize(UFortItemDefinition* Definition);
    static AFortPickup* SpawnPickup(UObject* Object, FFrame& Stack, AFortPickup** Ret);
    static AFortPickup* K2_SpawnPickupInWorld(UObject* Object, FFrame& Stack, AFortPickup** Ret);
    static AFortPickup* SpawnItemVariantPickupInWorld(UObject* Object, FFrame& Stack, AFortPickup** Ret);

    static void InitLooting();
};