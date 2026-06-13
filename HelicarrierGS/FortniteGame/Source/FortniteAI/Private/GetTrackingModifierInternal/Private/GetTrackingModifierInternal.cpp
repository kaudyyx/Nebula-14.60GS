#include "pch.h"
#include <FortniteGame/Source/FortniteAI/Public/GetTrackingModifierInternal/Public/GetTrackingModifierInternal.h>

float GetTrackingModifierInternal::UFortAthenaAIBotAimingDigestedSkillSet_GetTrackingModifierInternal(UFortAthenaAIBotAimingDigestedSkillSet* SkillSet, int Curve, double SignNegationProbability)
{
    return 1.0f;
}


void GetTrackingModifierInternal::InitGetTrackingModifierInternal()
{
    MH_CreateHook((LPVOID)(ImageBase + 0x1F50610), UFortAthenaAIBotAimingDigestedSkillSet_GetTrackingModifierInternal, (LPVOID*)&UFortAthenaAIBotAimingDigestedSkillSet_GetTrackingModifierInternalOG);//"UFortAthenaAIBotAimingDigestedSkillSet::GetTrackingModifierInternal, couldn't find valid value"
}