#include "pch.h"
#include <FortniteGame/Source/FortniteAI/Public/CosmeticLibraryBase/CosmeticLibraryBase.h>

void CosmeticLibraryBase::UFortAthenaAISpawnerDataComponent_CosmeticLibraryBaseOnSpawned(UFortAthenaAISpawnerDataComponent* CosmeticLibrary, AFortPlayerPawnAthena* Pawn)
{
	UFortAthenaAISpawnerDataComponent_CosmeticLibraryBaseOnSpawnedOG(CosmeticLibrary, Pawn);

	if (!CosmeticLibrary || !Pawn)
		return;

	auto PC = Utils::Cast<AFortAthenaAIBotController>(Pawn->GetController());

	if (!PC)
		return;

	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

	if (PC->CosmeticLoadoutBC.Character)
	{
		if (PC->CosmeticLoadoutBC.Character->HeroDefinition)
		{
			for (int i = 0; i < PC->CosmeticLoadoutBC.Character->HeroDefinition->Specializations.Num(); i++)
			{
				auto SpecStr = UKismetStringLibrary::Conv_NameToString(PC->CosmeticLoadoutBC.Character->HeroDefinition->Specializations[i].ObjectID.AssetPathName);
				UFortHeroSpecialization* Spec = Utils::StaticFindObject<UFortHeroSpecialization>(SpecStr.ToString());
				if (Spec)
				{
					for (int j = 0; j < Spec->CharacterParts.Num(); j++)
					{
						auto PartStr = UKismetStringLibrary::Conv_NameToString(Spec->CharacterParts[j].ObjectID.AssetPathName);
						UCustomCharacterPart* CharacterPart = Utils::StaticFindObject<UCustomCharacterPart>(PartStr.ToString());
						if (CharacterPart)
						{
							PlayerState->CharacterData.Parts[(uintptr_t)CharacterPart->CharacterPartType] = CharacterPart;
						}
						PartStr.Free();
					}
				}
				SpecStr.Free();
			}
		}
	}
	PlayerState->OnRep_CharacterData();

	PC->PlayerBotPawn = Pawn;
}

void CosmeticLibraryBase::InitCosmeticLibraryBase()
{
	MH_CreateHook((LPVOID)(ImageBase + 0x1FB2240), UFortAthenaAISpawnerDataComponent_CosmeticLibraryBaseOnSpawned, (LPVOID*)&UFortAthenaAISpawnerDataComponent_CosmeticLibraryBaseOnSpawnedOG);// "UFortAthenaAISpawnerDataComponent_CosmeticBase::OnSpawned on %s..."
}