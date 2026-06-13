
#include "pch.h"
#include "../Public/Abilities.h"

void Abilities::InternalServerTryActiveAbilityHook(UAbilitySystemComponent* AbilitySystemComponent,
    FGameplayAbilitySpecHandle Handle, bool InputPressed, const FPredictionKey& PredictionKey,
    const FGameplayEventData* TriggerEventData)
{
    FGameplayAbilitySpec* Spec = FindAbilitySpecFromHandle(AbilitySystemComponent, Handle);
    if (!Spec)
    {
        printf("InternalServerTryActiveAbility. Rejecting ClientActivation of ability with invalid SpecHandle!");
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        return;
    }

    const UGameplayAbility* AbilityToActivate = Spec->Ability;

    if (!AbilityToActivate)
        return;

    UGameplayAbility* InstancedAbility = nullptr;
    Spec->InputPressed = true;

    AFortPlayerStateAthena* PS = (AFortPlayerStateAthena*)AbilitySystemComponent->GetOwner();
    auto Pawn = PS ? PS->GetCurrentPawn() : nullptr;
    AFortPlayerController* PC = nullptr;

    if (Pawn)
    {
        PC = (AFortPlayerController*)Pawn->GetOwner();

        if (Spec->Ability->GetName() == "Default__GAB_InterrogatePlayer_Reveal_C")
        {
            if ((Pawn->GetHealth() + 20.f) >= 100)
            {
                auto NewShield = (Pawn->GetShield() + 20.f) >= 100 ? 100 : (Pawn->GetShield() + 20.f);
                Pawn->SetShield(NewShield);
            }
            else
            {
                auto NewHealth = Pawn->GetHealth() + 20.f;
                Pawn->SetHealth(NewHealth);
            }
        }
        /*else if (Spec->Ability->GetName() == "Default__GAT_Athena_c4_Detonate_C")
        {
            auto Def = Utils::StaticLoadObject<UFortItemDefinition>(
                "/Game/Athena/Items/Consumables/C4/Athena_C4.Athena_C4");
            float MaxStackSize = FortInventory::GetMaxStackSize(Def);
            FFortItemEntry* FoundEntry = nullptr;

            for (int32 i = 0; i < PC->WorldInventory->Inventory.ReplicatedEntries.Num(); i++)
            {
                FFortItemEntry& Entry = PC->WorldInventory->Inventory.ReplicatedEntries[i];
                if (Entry.ItemDefinition == Def && (Entry.Count < MaxStackSize))
                {
                    FoundEntry = &Entry;
                }
            }

            if (FoundEntry && FoundEntry->Count == 0)
            {
                FortInventory::RemoveItem22(PC, Def, true);
            }
        }*/
    }

    // Attempt to activate the ability (server side) and tell the client if it succeeded or failed.
    if (InternalTryActivateAbility(AbilitySystemComponent, Handle, PredictionKey, &InstancedAbility, nullptr, TriggerEventData))
    {
        // TryActivateAbility handles notifying the client of success
    }
    else
    {
        AbilitySystemComponent->ClientActivateAbilityFailed(Handle, PredictionKey.Current);
        Spec->InputPressed = false;

        AbilitySystemComponent->ActivatableAbilities.MarkItemDirty(*Spec);
    }
}

FGameplayAbilitySpec* Abilities::FindAbilitySpecFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle)
{
    for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->ActivatableAbilities.Items)
    {
        if (Spec.Handle.Handle == Handle.Handle)
        {
            return &Spec;
        }
    }

    return nullptr;
}

void Abilities::GiveDefaultAbilitySet(UAbilitySystemComponent* AbilitySystemComponent)
{
    static UFortAbilitySet* GAS_AthenaPlayer = Utils::StaticLoadObject<UFortAbilitySet>("/Game/Abilities/Player/Generic/Traits/DefaultPlayer/GAS_AthenaPlayer.GAS_AthenaPlayer");

    if (!GAS_AthenaPlayer)
    {
        printf("Failed to load GAS_AthenaPlayer");
        return;
    }

    for (int i = 0; i < GAS_AthenaPlayer->GameplayAbilities.Num(); ++i)
    {
        UGameplayAbility* AbilityClass = Utils::Cast<UGameplayAbility>(GAS_AthenaPlayer->GameplayAbilities[i].Get()->DefaultObject);
        if (!AbilityClass) continue;

        Abilities::GiveAbility(AbilitySystemComponent, AbilityClass);
    }

    for (int i = 0; i < GAS_AthenaPlayer->GrantedGameplayEffects.Num(); ++i)
    {
        UClass* GameplayEffect = GAS_AthenaPlayer->GrantedGameplayEffects[i].GameplayEffect.Get();
        float Level = GAS_AthenaPlayer->GrantedGameplayEffects[i].Level;

        if (!GameplayEffect)
            continue;

        AbilitySystemComponent->BP_ApplyGameplayEffectToSelf(GameplayEffect, Level, FGameplayEffectContextHandle());
    }
}

void Abilities::GiveAbility(UAbilitySystemComponent* AbilitySystemComponent, UGameplayAbility* GameplayAbility)
{
    if (!GameplayAbility)
        return;

    FGameplayAbilitySpec Spec;
    SpecConstructor(&Spec, GameplayAbility, 1, -1, nullptr);
    InternalGiveAbility(AbilitySystemComponent, &Spec.Handle, Spec);
}

void Abilities::GiveAbilityProppines(AFortPlayerPawnAthena* Pawn, UGameplayAbility* GameplayAbility)
{
    if (!Pawn || !GameplayAbility)
    {
        std::cout << "Pawn || GameplayAbility is null" << std::endl;
        return;
    }

    auto AbilitySystemComponent = Pawn->AbilitySystemComponent;

    if (!AbilitySystemComponent) {
        std::cout << "AbilitySystemComponent is null" << std::endl;
        return;
    }

    FGameplayAbilitySpec Spec;
    SpecConstructor(&Spec, GameplayAbility, 1, -1, nullptr);
    InternalGiveAbility(AbilitySystemComponent, &Spec.Handle, Spec);
}

void Abilities::GiveAbilityAndActivateOnce(UAbilitySystemComponent* AbilitySystemComponent, UGameplayAbility* GameplayAbility, UObject* SourceObject)
{
    if (!GameplayAbility)
        return;

    FGameplayAbilitySpec Spec;
    SpecConstructor(&Spec, GameplayAbility, 1, -1, SourceObject);
    GiveAbilityAndActivateOnceFn(AbilitySystemComponent, &Spec.Handle, Spec, nullptr);
}

void Abilities::ExecuteGameplayCue(AFortPlayerPawnAthena* Pawn, FGameplayTag GameplayTag)
{
    Pawn->NetMulticast_InvokeGameplayCueAdded(GameplayTag, FPredictionKey(), Pawn->AbilitySystemComponent->MakeEffectContext());
    Pawn->NetMulticast_InvokeGameplayCueExecuted(GameplayTag, FPredictionKey(), Pawn->AbilitySystemComponent->MakeEffectContext());
}

void Abilities::K2_ExecuteGameplayCue(UObject* Context, FFrame& Stack, void* Ret)
{
    UPROPERTY(FGameplayTag, GameplayCueTag);
    UPROPERTY(FGameplayEffectContextHandle, ContextHandle);

    if (UGameplayAbility* GameplayAbility = Utils::Cast<UGameplayAbility>(Context))
    {
        if (UAbilitySystemComponent* AbilitySystemComponent = GameplayAbility->GetAbilitySystemComponentFromActorInfo())
        {
            AbilitySystemComponent->NetMulticast_InvokeGameplayCueAdded(GameplayCueTag, FPredictionKey(), ContextHandle);
            AbilitySystemComponent->NetMulticast_InvokeGameplayCueExecuted(GameplayCueTag, FPredictionKey(), ContextHandle);
        }
    }

    K2_ExecuteGameplayCueOG(Context, Stack, Ret);
}

//void (*_OnUnEquip)(AFortWeapon* Weapon);
//void OnUnEquip(AFortWeapon* Weapon)
//{
//   // _OnUnEquip(Weapon);
//
//    AFortPlayerPawnAthena* PlayerPawn = Utils::Cast<AFortPlayerPawnAthena>(Weapon->GetOwner());
//
//    if (PlayerPawn == NULL)
//        return;
//
//    AFortPlayerStateAthena* PlayerState = Utils::Cast<AFortPlayerStateAthena>(PlayerPawn->PlayerState);
//
//    if (PlayerState == NULL)
//        return;
//
//    UAbilitySystemComponent* AbilitySystemComponent = PlayerState->AbilitySystemComponent;
//
//    if (AbilitySystemComponent == NULL)
//        return;
//
//    static void (*ClearAbility)(UAbilitySystemComponent*, const FGameplayAbilitySpecHandle & Handle) =
//        decltype(ClearAbility)(InSDKUtils::GetImageBase() + 0xA3B4A0);//ClearAbility_ (offset): 0xA3B4A0 Attempted to call ClearAbility() on the client. This is not allowed!
//
//    if (Weapon->PrimaryAbilitySpecHandle.Handle != -1)
//    {
//        ClearAbility(AbilitySystemComponent, Weapon->PrimaryAbilitySpecHandle);
//        Weapon->PrimaryAbilitySpecHandle.Handle = -1;
//    }
//
//    if (Weapon->SecondaryAbilitySpecHandle.Handle != -1)
//    {
//        ClearAbility(AbilitySystemComponent, Weapon->SecondaryAbilitySpecHandle);
//        Weapon->SecondaryAbilitySpecHandle.Handle = -1;
//    }
//
//    if (Weapon->ReloadAbilitySpecHandle.Handle != -1)
//    {
//        ClearAbility(AbilitySystemComponent, Weapon->ReloadAbilitySpecHandle);
//        Weapon->ReloadAbilitySpecHandle.Handle = -1;
//    }
//
//    if (Weapon->ImpactAbilitySpecHandle.Handle != -1)
//    {
//        ClearAbility(AbilitySystemComponent, Weapon->ImpactAbilitySpecHandle);
//        Weapon->ImpactAbilitySpecHandle.Handle = -1;
//    }
//
//    for (FGameplayAbilitySpecHandle& EquippedAbilityHandle : Weapon->EquippedAbilityHandles)
//    {
//        if (EquippedAbilityHandle.Handle != -1)
//        {
//            ClearAbility(AbilitySystemComponent, EquippedAbilityHandle);
//            EquippedAbilityHandle.Handle = -1;
//        }
//    }
//
//    Weapon->EquippedAbilityHandles.Free();
//
//    for (FFortAbilitySetHandle& EquippedAbilitySetHandle : Weapon->EquippedAbilitySetHandles)
//    {
//        UFortKismetLibrary::UnequipFortAbilitySet(EquippedAbilitySetHandle);
//    }
//
//    Weapon->EquippedAbilitySetHandles.Free();
//
//    _OnUnEquip(Weapon);
//}

UFortAbilitySet* Abilities::LoadAbilitySet(TSoftObjectPtr<UFortAbilitySet> SoftAbilitySet)
{
    UFortAbilitySet* AbilitySet = SoftAbilitySet.Get();

    if (!AbilitySet && SoftAbilitySet.ObjectID.AssetPathName.IsValid())
    {
        const FString& AssetPathName = UKismetStringLibrary::Conv_NameToString(SoftAbilitySet.ObjectID.AssetPathName);
        AbilitySet = StaticLoadObjectGay<UFortAbilitySet>(AssetPathName.CStr());
    }

    return AbilitySet;
}


void (*_OnUnEquip)(AFortWeapon* Weapon);
void OnUnEquip(AFortWeapon* Weapon)
{
    //static bool (*RemoveItemAbilitySet)(IFortInventoryOwnerInterface* InventoryOwner, UFortAbilitySet* AbilitySet, UFortItem* Item) =
    //    decltype(RemoveItemAbilitySet)(InSDKUtils::GetImageBase() + 0x65244A0);

    //auto Pawn = Cast<AFortPlayerPawnAthena>(Weapon->GetOwner());
    //auto Controller = Cast<AFortPlayerControllerAthena>(Pawn->Controller);

    //auto InventoryOwner = (IFortInventoryOwnerInterface*)(__int64(Controller) + 0x708);

    //for (FFortAbilitySetHandle& Abilityset : Weapon->EquippedAbilitySetHandles)
    //{
    //    if (Abilityset.l == NULL)
    //        continue;
    //}

    AFortPlayerPawnAthena* PlayerPawn = Utils::Cast<AFortPlayerPawnAthena>(Weapon->GetOwner());

    if (PlayerPawn == NULL)
        return;

    AFortPlayerStateAthena* PlayerState = Utils::Cast<AFortPlayerStateAthena>(PlayerPawn->PlayerState);

    if (PlayerState == NULL)
        return;

    UAbilitySystemComponent* AbilitySystemComponent = PlayerState->AbilitySystemComponent;

    if (AbilitySystemComponent == NULL)
        return;

    static void (*ClearAbility)(UAbilitySystemComponent*, const FGameplayAbilitySpecHandle & Handle) =
        decltype(ClearAbility)(InSDKUtils::GetImageBase() + 0xA3B4A0);

    if (Weapon->PrimaryAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->PrimaryAbilitySpecHandle);
        Weapon->PrimaryAbilitySpecHandle.Handle = -1;
    }

    if (Weapon->SecondaryAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->SecondaryAbilitySpecHandle);
        Weapon->SecondaryAbilitySpecHandle.Handle = -1;
    }

    if (Weapon->ReloadAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->ReloadAbilitySpecHandle);
        Weapon->ReloadAbilitySpecHandle.Handle = -1;
    }

    if (Weapon->ImpactAbilitySpecHandle.Handle != -1)
    {
        ClearAbility(AbilitySystemComponent, Weapon->ImpactAbilitySpecHandle);
        Weapon->ImpactAbilitySpecHandle.Handle = -1;
    }

    for (FGameplayAbilitySpecHandle& EquippedAbilityHandle : Weapon->EquippedAbilityHandles)
    {
        if (EquippedAbilityHandle.Handle != -1)
        {
            ClearAbility(AbilitySystemComponent, EquippedAbilityHandle);
            EquippedAbilityHandle.Handle = -1;
        }
    }

    Weapon->EquippedAbilityHandles.Free();

    for (FFortAbilitySetHandle& EquippedAbilitySetHandle : Weapon->EquippedAbilitySetHandles)
    {
        UFortKismetLibrary::UnequipFortAbilitySet(EquippedAbilitySetHandle);
    }

    Weapon->EquippedAbilitySetHandles.Free();

    _OnUnEquip(Weapon);
}



void Abilities::InitAbilitiesHooks()
{
    Utils::SwapVFTs(UAbilitySystemComponent::StaticClass()->DefaultObject, 0xfd, Abilities::InternalServerTryActiveAbilityHook, nullptr);
    Utils::SwapVFTs(UFortAbilitySystemComponent::StaticClass()->DefaultObject, 0xfd, Abilities::InternalServerTryActiveAbilityHook, nullptr);
    Utils::SwapVFTs(UFortAbilitySystemComponentAthena::StaticClass()->DefaultObject, 0xfd, Abilities::InternalServerTryActiveAbilityHook, nullptr);
    Utils::ExecHook(("/Script/GameplayAbilities.GameplayAbility.K2_ExecuteGameplayCue"), K2_ExecuteGameplayCue, K2_ExecuteGameplayCueOG);
    Utils::ExecHook(("/Script/FortniteGame.FortWeaponComponent.OnUnEquip"), OnUnEquip, _OnUnEquip);

}
