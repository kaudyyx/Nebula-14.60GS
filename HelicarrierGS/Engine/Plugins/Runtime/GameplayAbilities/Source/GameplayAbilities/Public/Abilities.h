
#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class Abilities
{
public:
    using InternalTryActivateAbility_t = bool(*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle, FPredictionKey, UGameplayAbility**, void*, const FGameplayEventData*);
    using InternalGiveAbility_t = FGameplayAbilitySpecHandle * (*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec);
    using SpecConstructor_t = __int64(*)(FGameplayAbilitySpec*, UObject*, int, int, UObject*);
    using GiveAbilityAndActivateOnceFn_t = FGameplayAbilitySpecHandle(*)(UAbilitySystemComponent*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec, FGameplayEventData*);

    static inline InternalTryActivateAbility_t InternalTryActivateAbility =
        reinterpret_cast<InternalTryActivateAbility_t>(InSDKUtils::GetImageBase() + 0xa52940);

    static inline InternalGiveAbility_t InternalGiveAbility =
        reinterpret_cast<InternalGiveAbility_t>(InSDKUtils::GetImageBase() + 0xa4bb20);

    static inline SpecConstructor_t SpecConstructor =
        reinterpret_cast<SpecConstructor_t>(InSDKUtils::GetImageBase() + 0xa27b60);

    static inline GiveAbilityAndActivateOnceFn_t GiveAbilityAndActivateOnceFn =
        reinterpret_cast<GiveAbilityAndActivateOnceFn_t>(InSDKUtils::GetImageBase() + 0xa4bc50);

    //public:
    static void InternalServerTryActiveAbilityHook(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey, const FGameplayEventData* TriggerEventData);
    static FGameplayAbilitySpec* FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle);

    static void GiveDefaultAbilitySet(UAbilitySystemComponent* AbilitySystemComponent);
    static void GiveAbility(UAbilitySystemComponent* AbilitySystemComponent, UGameplayAbility* GameplayAbility);
    static void GiveAbilityAndActivateOnce(UAbilitySystemComponent* AbilitySystemComponent, UGameplayAbility* GameplayAbility, UObject* SourceObject);

    static void ExecuteGameplayCue(AFortPlayerPawnAthena* Pawn, FGameplayTag GameplayTag);

    static void GiveAbilityProppines(AFortPlayerPawnAthena* Pawn, UGameplayAbility* GameplayAbility);

    static UFortAbilitySet* LoadAbilitySet(TSoftObjectPtr<UFortAbilitySet> SoftAbilitySet);

    static void K2_ExecuteGameplayCue(UObject* Context, FFrame& Stack, void* Ret);
    static inline void (*K2_ExecuteGameplayCueOG)(UObject* Context, FFrame& Stack, void* Ret);
    static void RemoveWeaponAbilities(AActor*);
    static void InitAbilitiesHooks();
};
