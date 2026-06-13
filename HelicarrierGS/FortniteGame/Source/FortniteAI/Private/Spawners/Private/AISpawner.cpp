#include "pch.h"
#include <FortniteGame/Source/FortniteAI/Public/Spawners/Public/AISpawner.h>

void FortSpawner::SpawnAI(UClass* Class, const std::string& Name)
{
    auto GameMode = Utils::Cast<AFortGameModeAthena>(UWorld::GetWorld()->AuthorityGameMode);
    if (!GameMode)
        return;
    TArray<AFortAthenaPatrolPath*> PossibleSpawnPaths;
    TArray<AActor*> FoundActors;

    UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), AFortAthenaPatrolPath::StaticClass(), &FoundActors);

    for (auto& Actor : FoundActors)
    {
        AFortAthenaPatrolPath* PatrolPath = Utils::Cast<AFortAthenaPatrolPath>(Actor);
        if (!PatrolPath) continue;

        if (PatrolPath->GameplayTags.GameplayTags.Num() == 0) continue;
        auto PathName = PatrolPath->GameplayTags.GameplayTags[0].TagName.ToString();

        if (PathName.length() >= Name.length() && PathName.substr(PathName.length() - Name.length()) == Name)
        {
            PossibleSpawnPaths.Add(PatrolPath);
        }
    }

    if (PossibleSpawnPaths.Num() == 0)
    {
        return;
    }

    auto ComponentList = ((UFortAthenaAIBotSpawnerData*)Class)->CreateComponentListFromClass(Class, UWorld::GetWorld());
    auto AISystem = (UAthenaAISystem*)UWorld::GetWorld()->AISystem;

    if (((Name == "Tomato" || Name == "Date" || Name == "MangLauncher" || Name == "Mang") || Name.find("TomatoQuinnJet") == 0) && PossibleSpawnPaths.Num() > 0)
    {
        for (int i = PossibleSpawnPaths.Num() - 1; i > 0; --i)
        {
            int j = rand() % (i + 1);
            auto Temp = PossibleSpawnPaths[i];
            PossibleSpawnPaths[i] = PossibleSpawnPaths[j];
            PossibleSpawnPaths[j] = Temp;
        }

        for (int i = 0; i < PossibleSpawnPaths.Num(); ++i)
        {
            auto PatrolPath = PossibleSpawnPaths[i];

            if (PatrolPath->PatrolPoints.Num() == 0) continue;

            auto Transform = PatrolPath->PatrolPoints[0]->GetTransform();
            FVector Loc = Transform.Translation;

            GameMode->ServerBotManager->SpawnAI(Transform.Translation, Transform.Rotation.Rotator(), ComponentList);
        }
    }
    else
    {
        int RandomIndex = rand() % PossibleSpawnPaths.Num();
        auto PatrolPath = PossibleSpawnPaths[RandomIndex];

        if (PatrolPath->PatrolPoints.Num() == 0)
        {
            return;
        }

        auto Transform = PatrolPath->PatrolPoints[0]->GetTransform();
        FVector Loc = Transform.Translation;

        GameMode->ServerBotManager->SpawnAI(Transform.Translation, Transform.Rotation.Rotator(), ComponentList);
    }
}

void FortSpawner::RequestAISpawn()
{
    auto TonyStark = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/TomatoAlpha/BP_AIBotSpawnerData_Gasket_TomatoAlpha.BP_AIBotSpawnerData_Gasket_TomatoAlpha_C");
    SpawnAI(TonyStark, "TomatoAlpha");

    auto DoctorDoom = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/DateAlpha/BP_AIBotSpawnerData_Gasket_DateAlpha.BP_AIBotSpawnerData_Gasket_DateAlpha_C");
    SpawnAI(DoctorDoom, "DateAlpha");

    auto DoomHenchMen = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/Date/BP_AIBotSpawnerData_Gasket_Date.BP_AIBotSpawnerData_Gasket_Date_C");
    SpawnAI(DoomHenchMen, "Date");

    auto Wolverine = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/Wasabi/BP_AIBotSpawnerData_Gasket_Wasabi.BP_AIBotSpawnerData_Gasket_Wasabi_C");
    SpawnAI(Wolverine, "Wasabi");

    auto StarkBot = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/Tomato/BP_AIBotSpawnerData_Gasket_Tomato.BP_AIBotSpawnerData_Gasket_Tomato_C");
    SpawnAI(StarkBot, "Tomato");
    SpawnAI(StarkBot, "TomatoQuinnJet17");
    SpawnAI(StarkBot, "TomatoQuinnJet16");
    SpawnAI(StarkBot, "TomatoQuinnJet15");
    SpawnAI(StarkBot, "TomatoQuinnJet14");
    SpawnAI(StarkBot, "TomatoQuinnJet13");
    SpawnAI(StarkBot, "TomatoQuinnJet12");
    SpawnAI(StarkBot, "TomatoQuinnJet11");
    SpawnAI(StarkBot, "TomatoQuinnJet10");
    SpawnAI(StarkBot, "TomatoQuinnJet09");
    SpawnAI(StarkBot, "TomatoQuinnJet08");
    SpawnAI(StarkBot, "TomatoQuinnJet07");
    SpawnAI(StarkBot, "TomatoQuinnJet06");
    SpawnAI(StarkBot, "TomatoQuinnJet05");
    SpawnAI(StarkBot, "TomatoQuinnJet04");
    SpawnAI(StarkBot, "TomatoQuinnJet04");
    SpawnAI(StarkBot, "TomatoQuinnJet02");
    SpawnAI(StarkBot, "TomatoQuinnJet01");

    //14.40 bosses

   // auto Midas = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/MangAlpha/BP_AIBotSpawnerData_Gasket_MangAlpha.BP_AIBotSpawnerData_Gasket_MangAlpha_C");
  //  SpawnAI(Midas, "MangAlpha");

   // auto GhostHenchmen = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/Mang_Launcher/BP_AIBotSpawnerData_Gasket_MangLauncher.BP_AIBotSpawnerData_Gasket_MangLauncher_C");
   // SpawnAI(GhostHenchmen, "MangLauncher");

    //auto GhostHenchmen2 = Utils::StaticLoadObject<UClass>("/Gasket/AISpawnerData/Mang_Crossbow/BP_AIBotSpawnerData_Gasket_Mang_Crossbow.BP_AIBotSpawnerData_Gasket_Mang_Crossbow_C");
    //SpawnAI(GhostHenchmen2, "Mang");

}
