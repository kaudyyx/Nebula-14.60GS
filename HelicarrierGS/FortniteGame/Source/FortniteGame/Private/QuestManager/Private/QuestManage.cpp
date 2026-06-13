#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/QuestManager/Public/QuestManage.h>

void Challenges::ProgressQuest(AFortPlayerControllerAthena* PC, UFortQuestManager* QuestManager, UFortQuestItem* QuestItem, UFortQuestItemDefinition* QuestDefinition, FName BackendName, int32 PCount)
{
	auto Count = QuestManager->GetObjectiveCompletionCount(QuestDefinition, BackendName);
	auto Obj = QuestDefinition->Objectives.Search([BackendName](FFortMcpQuestObjectiveInfo& item) { return item.BackendName == BackendName; });

	bool allObjsCompleted = false;
	if (!Obj) return;
	if (Obj->Count == Count + 1) {
		allObjsCompleted = true;
		ObjCompArray[PC].push_back(*Obj);

		auto CompletionCount = 0;
		for (auto& Obj : QuestDefinition->Objectives)
		{
			bool Found = false;
			for (auto& ObjComp : ObjCompArray[PC])
			{
				if (Obj.BackendName == ObjComp.BackendName)
				{
					Found = true;
					break;
				}
			}
			if (!Found)
			{
				allObjsCompleted = false;
			}
			else CompletionCount++;
		}
		if (QuestDefinition->ObjectiveCompletionCount != 0 && QuestDefinition->ObjectiveCompletionCount == CompletionCount + 1) allObjsCompleted = true;
	}

	if (Count <= Obj->Count) {
		auto PlayerState = PC->PlayerState->Cast<AFortPlayerStateAthena>();

			if (PlayerState && PlayerState->PlayerTeam)
			{
				for (const auto& TeamMember : PlayerState->PlayerTeam->TeamMembers)
			{
				auto TeamMemberPC = (AFortPlayerControllerAthena*)TeamMember;
				if (TeamMemberPC->IsA<AFortAthenaAIBotController>())
					continue;
					auto TeamMemberQuestManager = TeamMemberPC->GetQuestManager(ESubGame::Athena);
					if (!TeamMemberQuestManager) continue;
					TeamMemberQuestManager->HandleQuestUpdated(TeamMemberPC, QuestDefinition, BackendName, Count + 1, 1,
					TeamMemberPC == PC ? nullptr : PlayerState, Count == Obj->Count, allObjsCompleted);
			}
		}
	}
	Count = QuestManager->GetObjectiveCompletionCount(QuestDefinition, BackendName);
	FCelestiaProfileUpdate* ProfUpd = nullptr;
	for (auto& ProfUpdInMap : ProfileUpdateMap[PC]) {
		if (ProfUpdInMap.BackendName == BackendName) {
			ProfUpd = (FCelestiaProfileUpdate*)&ProfUpdInMap;
			break;
		}
	}
	if (!ProfUpd) {
		FCelestiaProfileUpdate ProfUpd;
		ProfUpd.BackendName = BackendName;
		ProfUpd.Count = Count;
		ProfileUpdateMap[PC].push_back(ProfUpd);
	}
	else {
		for (auto& ProfUpdInMap : ProfileUpdateMap[PC]) {
			if (ProfUpdInMap.BackendName == BackendName) {
				ProfUpdInMap.Count = Count;
				break;
			}
		}
	}
	if (allObjsCompleted) {
		int32 XPCount = 0;

		if (auto RewardsTable = QuestDefinition->RewardsTable)
		{
			static auto Name = UKismetStringLibrary::Conv_StringToName(L"Default");
			auto DefaultRow = RewardsTable->Search([](FName& RName, uint8* Row) { return RName == Name; });
			if (DefaultRow)
				XPCount = (*(FFortQuestRewardTableRow**)DefaultRow)->Quantity;
		}

		if (XPCount) {
			FXPEventEntry QuestEntry;
			QuestEntry.EventXpValue = XPCount;
			QuestEntry.QuestDef = QuestDefinition;
			QuestEntry.Time = UGameplayStatics::GetTimeSeconds(UWorld::Get());
			QuestEntry.TotalXpEarnedInMatch = PC->XPComponent->TotalXpEarned + XPCount;
			QuestEntry.SimulatedXpEvent = UKismetTextLibrary::Conv_StringToText(L"Objective completed");

			PC->XPComponent->ChallengeXp += XPCount;
			PC->XPComponent->TotalXpEarned += XPCount;
			PC->XPComponent->OnXpUpdated(PC->XPComponent->CombatXp, PC->XPComponent->SurvivalXp, PC->XPComponent->MedalBonusXP, PC->XPComponent->ChallengeXp, PC->XPComponent->MatchXp, PC->XPComponent->TotalXpEarned);

			PC->XPComponent->InMatchProfileVer++;
			PC->XPComponent->OnInMatchProfileUpdate(PC->XPComponent->InMatchProfileVer);
			PC->XPComponent->OnProfileUpdated();

			/*PC->XPComponent->HighPrioXPEvent(QuestEntry);*/
		}
	}

	if (allObjsCompleted)
		QuestManager->ClaimQuestReward(QuestItem);
}

void Challenges::GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* Accolade)
{
	if (!PC || !Accolade)
		return;

	FXPEventInfo Info{};

	Info.Accolade = UKismetSystemLibrary::GetPrimaryAssetIdFromObject(Accolade);
	float XpValue = Accolade->XpRewardAmount.GetValue();

	if (XpValue == 0)
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Accolade->XpRewardAmount.Curve.CurveTable, UKismetStringLibrary::Conv_StringToName(L"Default_Medal"), Accolade->XpRewardAmount.Value, nullptr, &XpValue, FString());

	Info.EventXpValue = (int32)XpValue;
	Info.RestedValuePortion = Info.EventXpValue;
	Info.RestedXPRemaining = Info.EventXpValue;
	Info.TotalXpEarnedInMatch = Info.EventXpValue + PC->XPComponent->TotalXpEarned;
	Info.Priority = Accolade->Priority;
	Info.SimulatedText = Accolade->GetShortDescription();
	Info.EventName = Accolade->Name;
	Info.SeasonBoostValuePortion = 0;

	PC->XPComponent->MatchXp += Info.EventXpValue;
	PC->XPComponent->TotalXpEarned += Info.EventXpValue;

	FAthenaAccolades AthenaAccolade{};
	AthenaAccolade.AccoladeDef = Accolade;
	AthenaAccolade.Count = 1;
	AthenaAccolade.TemplateId = Accolade->Name.GetRawWString().c_str();

	for (auto& TSRemove : Accolade->AccoladeToReplace)
	{
		UFortAccoladeItemDefinition* AccoladeToRemove = TSRemove;
		auto AthenaAccoladeIndex = PC->XPComponent->PlayerAccolades.SearchIndex([AccoladeToRemove](FAthenaAccolades& item)
			{ return item.AccoladeDef == AccoladeToRemove; });

		auto MedalIndex = PC->XPComponent->MedalsEarned.SearchIndex([AccoladeToRemove](UFortAccoladeItemDefinition* item)
			{ return item == AccoladeToRemove; });

		if (AthenaAccoladeIndex != -1)
			PC->XPComponent->PlayerAccolades.Remove(AthenaAccoladeIndex);

		if (MedalIndex != -1)
			PC->XPComponent->MedalsEarned.Remove(MedalIndex);
	}

	PC->XPComponent->PlayerAccolades.Add(AthenaAccolade);

	if (Accolade->AccoladeType == EFortAccoladeType::Medal)
	{
		PC->XPComponent->MedalsEarned.Add(Accolade);
		PC->XPComponent->ClientMedalsRecived(PC->XPComponent->PlayerAccolades);
	}

	PC->XPComponent->OnXPEvent(Info);
}


void Challenges::SendStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count, EFortQuestObjectiveStatEvent StatEvent)
{
	// thread([&]()
	{
		auto PC = (AFortPlayerControllerAthena*)QuestManager->GetPlayerControllerBP();

		if (!PC) // we are fucked anyway ykyk
			return;

		FGameplayTagContainer PlayerSourceTags;
		FGameplayTagContainer ContextTags;

		QuestManager->GetSourceAndContextTags(&PlayerSourceTags, &ContextTags);

		ContextTags.AppendTags(AFortGameStateAthena::Get()->CurrentPlaylistInfo.BasePlaylist->GameplayTagContainer);
		AdditionalSourceTags.AppendTags(PlayerSourceTags);

		static UDataTable* AthenaObjectiveStatXPTable = Utils::StaticFindObject<UDataTable>("/Game/Athena/Items/Quests/AthenaObjectiveStatXPTable.AthenaObjectiveStatXPTable");

		if (AthenaObjectiveStatXPTable)
		{
			for (const auto& [RowName, RowPtr] : AthenaObjectiveStatXPTable->RowMap)
			{
				auto Row = (FFortQuestObjectiveStatXPTableRow*)RowPtr;

				if (Row->Type != StatEvent || (Row->CountThreshhold > 0 && Row->CountThreshhold < Count) || Count > Row->MaxCount)
					continue;

				if (!TargetTags.HasAll(Row->TargetTags))
					continue;

				if (!AdditionalSourceTags.HasAll(Row->SourceTags))
					continue;

				if (!ContextTags.HasAll(Row->ContextTags))
					continue;

				if (!Utils::IsConditionMet(Row->Condition, TargetTags, AdditionalSourceTags, ContextTags))
					continue;

				static auto AccoladeName = UKismetStringLibrary::Conv_StringToName(L"Accolades");

				if (Row->AccoladePrimaryAssetId.PrimaryAssetType.Name == AccoladeName)
				{
					auto AccoladeToGive = (UFortAccoladeItemDefinition*)UKismetSystemLibrary::GetObjectFromPrimaryAssetId(Row->AccoladePrimaryAssetId);

					if (!AccoladeToGive)
						continue;

					if (Row->bOnceOnly)
					{
						if (OnceOnlyMap[AccoladeToGive])
							continue;

						OnceOnlyMap[AccoladeToGive] = true;
					}

					GiveAccolade(PC, AccoladeToGive);
				}
			}
		}

		for (auto& CurrentQuest : QuestManager->CurrentQuests)
		{
			if (CurrentQuest->HasCompletedQuest())
				continue;

			auto QuestDef = CurrentQuest->GetQuestDefinitionBP();

			if (!QuestDef)
				continue;

			if (QuestManager->HasCompletedQuest(QuestDef))
				continue;

			// Utils::Log("QuestDef: " + QuestDef->GetName());

			for (auto& Objective : QuestDef->Objectives)
			{
				if (QuestManager->HasCompletedObjectiveWithName(QuestDef, Objective.BackendName) ||
					QuestManager->HasCompletedObjective(QuestDef, Objective.ObjectiveStatHandle) ||
					CurrentQuest->HasCompletedObjectiveWithName(Objective.BackendName) ||
					CurrentQuest->HasCompletedObjective(Objective.ObjectiveStatHandle))
				{
					continue;
				}

				// Utils::Log("BackendName: " + Objective.BackendName.ToString());

				auto StatTable = Objective.ObjectiveStatHandle.DataTable;
				auto& StatRowName = Objective.ObjectiveStatHandle.RowName;

				if (!StatTable || !StatRowName.IsValid())
				{
					// Utils::Log("Using InlineObjectiveStats!");

					for (auto& ObjectiveStat : Objective.InlineObjectiveStats)
					{
						if (ObjectiveStat.Type != StatEvent)
							continue;
						bool bFoundCorrectQuest = true; // start with true and set to false if we dont contain

						for (auto& TagCondition : ObjectiveStat.TagConditions)
						{
							if (!TagCondition.Require || !bFoundCorrectQuest)
								continue;

							switch (TagCondition.Type)
							{
							case EInlineObjectiveStatTagCheckEntryType::Target:
							{
								if (!ObjectiveStat.bHasInclusiveTargetTags)
									break;

								if (!TargetTags.HasTag(TagCondition.Tag))
									bFoundCorrectQuest = false;

								break;
							}
							case EInlineObjectiveStatTagCheckEntryType::Source:
							{
								if (!ObjectiveStat.bHasInclusiveSourceTags)
									break;

								if (!AdditionalSourceTags.HasTag(TagCondition.Tag))
									bFoundCorrectQuest = false;

								break;
							}
							case EInlineObjectiveStatTagCheckEntryType::Context:
							{
								if (!ObjectiveStat.bHasInclusiveContextTags)
									break;

								if (!ContextTags.HasTag(TagCondition.Tag))
									bFoundCorrectQuest = false;

								break;
							}
							case EInlineObjectiveStatTagCheckEntryType::EInlineObjectiveStatTagCheckEntryType_MAX:
							{
								break;
							}
							default:
								break;
							}
						}

						if (!Utils::IsConditionMet(ObjectiveStat.Condition, TargetTags, AdditionalSourceTags, ContextTags))
							continue;

						if (ObjectiveStat.Type != StatEvent)
							bFoundCorrectQuest = false;

						if (bFoundCorrectQuest)
							ProgressQuest(PC, QuestManager, CurrentQuest, QuestDef, Objective.BackendName, Count);
					}
				}
				else if (StatTable && StatRowName.ComparisonIndex)
				{
					auto& RowMap = StatTable->RowMap;

					for (const auto& [RowName, RowPtr] : RowMap)
					{
						if (RowName == StatRowName)
						{
							auto Row = (FFortQuestObjectiveStatTableRow*)RowPtr;

							if (Row->Type != StatEvent)
								continue;

							if (!TargetTags.HasAll(Row->TargetTagContainer))
								continue;

							if (!AdditionalSourceTags.HasAll(Row->SourceTagContainer))
								continue;

							if (!ContextTags.HasAll(Row->ContextTagContainer))
								continue;

							if (!Utils::IsConditionMet(Row->Condition, TargetTags, AdditionalSourceTags, ContextTags))
								continue;

							// todo implement count

							ProgressQuest(PC, QuestManager, CurrentQuest, QuestDef, Objective.BackendName, Count);
						}
					}
				}
			}
		}
	} // ).detach(); // trolled
}


void Challenges::SendComplexCustomStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count)
{
	if (__int64(_ReturnAddress()) == ImageBase + 0x3312300)//48 8B 4D ? 48 85 C9 74 ? E8 ? ? ? ? 48 8B 4D ? 48 85 C9 74 ? E8 ? ? ? ? 48 8B 4D ? 48 85 C9 74 ? E8 ? ? ? ? 48 8B 4D ? 48 85 C9 74 ? E8 ? ? ? ? 48 8B 9C 24 ? ? ? ? 48 81 C4 ? ? ? ? 41 5F 41 5E 5F 5E 5D C3 CC B9
	{
		SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count, EFortQuestObjectiveStatEvent::ComplexCustom);
	}
	else
	{
		SendStatEvent(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, nullptr, nullptr, 1, EFortQuestObjectiveStatEvent::ComplexCustom);
	}

	return SendComplexCustomStatEventOG(QuestManager, TargetObject, AdditionalSourceTags, TargetTags, QuestActive, QuestCompleted, Count);
}

bool Challenges::test(UFortQuestManager* QuestManager, EFortQuestObjectiveStatEvent SE) {
	//Utils::Log("test!");
	////Utils::Log((int32)SE);
	//Utils::Log(QuestManager->GetPlayerControllerBP()->InteractionComponent->InteractActor->GetName());
	FGameplayTagContainer SourceTags;
	FGameplayTagContainer ContextTags;
	FGameplayTagContainer PlaylistContextTags;

	QuestManager->GetSourceAndContextTags(&SourceTags, &ContextTags);
	for (auto& Tag : SourceTags.GameplayTags)
		//Utils::Log(Tag.TagName.ToString());
		return true;
}

void Challenges::InitChallenges()
{
	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + Addresses::SendComplexCustomStatEvent), SendComplexCustomStatEvent, (LPVOID*)&SendComplexCustomStatEventOG);//4C 8B DC 49 89 5B ? 57 48 83 EC ? 33 C0 49 8D 53
	MH_CreateHook((LPVOID)(InSDKUtils::GetImageBase() + 0x2C2C2E0), test, nullptr);//48 89 5C 24 ? 57 48 83 EC ? 0F B6 FA 48 8D 47 or "FortniteGame.EFortQuestObjectiveStatEve"
}