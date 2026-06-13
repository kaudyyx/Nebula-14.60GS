#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

struct FCelestiaProfileUpdate {
	FName BackendName;
	int32 Count;
};

class Challenges
{
public:
	static inline xmap<AFortPlayerControllerAthena*, xvector<FCelestiaProfileUpdate>> ProfileUpdateMap;
	static inline xmap<UFortAccoladeItemDefinition*, bool> OnceOnlyMap;
	static inline xmap<AFortPlayerControllerAthena*, xvector<FFortMcpQuestObjectiveInfo>> ObjCompArray;
	static inline xmap<UClass*, uint32> CountMap;

	static void ProgressQuest(AFortPlayerControllerAthena* PC, UFortQuestManager* QuestManager, UFortQuestItem* QuestItem, UFortQuestItemDefinition* QuestDefinition, FName BackendName, int32 PCount);
	static void GiveAccolade(AFortPlayerControllerAthena* PC, UFortAccoladeItemDefinition* Accolade);
	static void SendStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count, EFortQuestObjectiveStatEvent StatEvent);
	static inline void (*SendComplexCustomStatEventOG)(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count);
	static void SendComplexCustomStatEvent(UFortQuestManager* QuestManager, UObject* TargetObject, FGameplayTagContainer& AdditionalSourceTags, FGameplayTagContainer& TargetTags, bool* QuestActive, bool* QuestCompleted, int32 Count);
	static bool test(UFortQuestManager* QuestManager, EFortQuestObjectiveStatEvent SE);
	static void InitChallenges();
};