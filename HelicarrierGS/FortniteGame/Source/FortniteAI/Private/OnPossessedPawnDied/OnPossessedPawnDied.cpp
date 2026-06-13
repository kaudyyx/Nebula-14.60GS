#include "pch.h"
#include <FortniteGame/Source/FortniteAI/Public/OnPossessedPawnDied/OnPossessedPawnDied.h>
#include <FortniteGame/Source/FortniteGame/Public/QuestManager/Public/QuestManage.h>
#include <FortniteGame/Source/FortnitePhoebe/Public/FortnitePhoebe.h>

static __declspec(noinline) void OnPossessedPawnDied_Impl(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
{
	if (!PC || !InstigatedBy)
		return OnPossessedPawnDie::OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);

	if (PC->Pawn && PC->Inventory && !PC->Pawn->IsA<ABP_Pawn_DangerGrape_C>())
		for (auto& Entry : PC->Inventory->Inventory.ReplicatedEntries)
		{
			if (!Entry.ItemDefinition) continue;
			if (Entry.ItemDefinition->IsA<UFortWeaponMeleeItemDefinition>() || Entry.ItemDefinition->IsA<UFortAmmoItemDefinition>())
				continue;
			Inventory::SpawnPickup(PC->Pawn->K2_GetActorLocation(), Entry, EFortPickupSourceTypeFlag::AI, EFortPickupSpawnSource::PlayerElimination);
		}

	for (auto& bot : SpawnedBots)
	{
		if (bot && bot->PC && bot->TickEnabled && bot->PC == PC)
		{
			if (InstigatedBy->PlayerState)
				bot->OnDied((AFortPlayerStateAthena*)InstigatedBy->PlayerState, DamageCauser);
			break;
		}
	}

	if (auto KillerPC = InstigatedBy->Cast<AFortPlayerControllerAthena>())
	{
		FGameplayTagContainer SourceTags;
		FGameplayTagContainer ContextTags;
		if (PC->Pawn)
		{
			FGameplayTagContainer TargetTags = ((AFortPawn*)PC->Pawn)->GameplayTags;
			auto QuestManager = KillerPC->GetQuestManager(ESubGame::Athena);
			if (QuestManager) {
				QuestManager->GetSourceAndContextTags(&SourceTags, &ContextTags);
				Challenges::CountMap[PC->Class]++;
				Challenges::SendStatEvent(QuestManager, PC->Pawn, SourceTags, TargetTags, nullptr, nullptr, Challenges::CountMap[PC->Class], EFortQuestObjectiveStatEvent::Kill);
			}
		}
	}

	return OnPossessedPawnDie::OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum);
}

void OnPossessedPawnDie::OnPossessedPawnDied(AFortAthenaAIBotController* PC, AActor* DamagedActor, float Damage, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FVector HitLocation, UPrimitiveComponent* HitComp, FName BoneName, FVector Momentum)
{
	__try { OnPossessedPawnDied_Impl(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum); }
	__except (EXCEPTION_EXECUTE_HANDLER) {
		__try { OnPossessedPawnDiedOG(PC, DamagedActor, Damage, InstigatedBy, DamageCauser, HitLocation, HitComp, BoneName, Momentum); }
		__except (EXCEPTION_EXECUTE_HANDLER) {}
	}
}

void OnPossessedPawnDie::InitOnPossessedPawnDie()
{
	MH_CreateHook((LPVOID)(ImageBase + 0x1D69910), OnPossessedPawnDied, (LPVOID*)&OnPossessedPawnDiedOG);
}