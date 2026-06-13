#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class GetTrackingModifierInternal
{
public:
	static inline float (*UFortAthenaAIBotAimingDigestedSkillSet_GetTrackingModifierInternalOG)(UFortAthenaAIBotAimingDigestedSkillSet* SkillSet, int Curve, double SignNegationProbability);
	static float UFortAthenaAIBotAimingDigestedSkillSet_GetTrackingModifierInternal(UFortAthenaAIBotAimingDigestedSkillSet* SkillSet, int Curve, double SignNegationProbability);

	static void InitGetTrackingModifierInternal();
};