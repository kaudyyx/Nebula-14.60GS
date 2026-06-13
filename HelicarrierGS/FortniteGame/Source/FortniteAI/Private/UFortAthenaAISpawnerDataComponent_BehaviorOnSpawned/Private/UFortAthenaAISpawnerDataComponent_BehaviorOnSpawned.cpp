#include "pch.h"
#include <FortniteGame/Source/FortniteAI/Public/UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned/Public/UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned.h>
static char(__fastcall* RegisterComponentWithWorld)(UObject* Component, UObject* World, __int64 Flags) = (decltype(RegisterComponentWithWorld))(InSDKUtils::GetImageBase() + 0x4B84040);
void UFortAthenaAISpawnerData_BehaviorOnSpawned::UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned(UFortAthenaAISpawnerDataComponent_Behavior* Behavior, AFortPawn* Pawn)
{
	if (!Pawn || !Behavior)
		return;
	auto Controller = (AFortAthenaAIBotController*)Pawn->GetController();
	auto PC = Utils::Cast<AFortAthenaAIBotController>(Pawn->GetController());

	if (!PC)
		return;

	UBehaviorTree* BTAsset = Behavior->BehaviorTree;

	UBlackboardComponent* BlackboardComp = PC->Blackboard;
	if (BTAsset->BlackboardAsset && PC->Blackboard == nullptr)
	{
		PC->UseBlackboard(BTAsset->BlackboardAsset, &BlackboardComp);
	}

	if (!PC->BrainComponent || !PC->BrainComponent->IsA(UBehaviorTreeComponent::StaticClass()))
	{
		PC->BrainComponent = (UBrainComponent*)PC->AddComponentByClass(UAthenaBehaviorTreeComponent::StaticClass(), true, FTransform(), false);
		RegisterComponentWithWorld(PC->BrainComponent, UWorld::GetWorld(), 0);
		PC->RunBehaviorTree(PC->BehaviorTree);
		PC->Blackboard->BrainComp->StartLogic();
	}

	return UFortAthenaAISpawnerDataComponent_BehaviorOnSpawnedOG(Behavior, Pawn);
}

void UFortAthenaAISpawnerData_BehaviorOnSpawned::InitUFortAthenaAISpawnerDataComponent_BehaviorOnSpawned()
{
	MH_CreateHook((LPVOID)(ImageBase + 0x1FB1C00), UFortAthenaAISpawnerDataComponent_BehaviorOnSpawned, (LPVOID*)&UFortAthenaAISpawnerDataComponent_BehaviorOnSpawnedOG);//"UFortAthenaAISpawnerDataComponent_Behav"
}