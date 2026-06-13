#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>

class PlayerController {
public:
	static void ServerCheat(AFortPlayerControllerAthena*, FString);
	DefHookOg(void, ServerExecuteInventoryItemHook, AFortPlayerControllerAthena* PC, FGuid Guid);

	DefHookOg(void, ServerAcknowledgePossessionHook, AFortPlayerControllerAthena* PC, APawn* Pawn);

	DefHookOg(void, ServerAttemptAircraftJumpHook, UFortControllerComponent_Aircraft* Comp, FRotator ClientRot);
	static void ServerClientIsReadyToRespawn(AFortPlayerControllerAthena* PC);


	DefHookOg(void, OnDamageServerHook, ABuildingSMActor* Actor, float Damage, FGameplayTagContainer DamageTags, FVector Momentum, FHitResult HitInfo, AFortPlayerControllerAthena* InstigatedBy, AActor* DamageCauser, FGameplayEffectContextHandle EffectContext);
	DefHookOg(void, ServerCreateBuildingActor, AFortPlayerControllerAthena* PC, FCreateBuildingActorData& CreateBuildingData);
	DefHookOg(void, ServerBeginEditingBuildingActor, AFortPlayerControllerAthena* PC, ABuildingSMActor* BuildingActorToEdit);
	DefHookOg(void, ServerEditBuildingActor, AFortPlayerController* PlayerController, ABuildingSMActor* BuildingActorToEdit, TSubclassOf<ABuildingSMActor> NewBuildingClass, uint8 RotationIterations, bool bMirrored);
	static void ServerEndEditingBuildingActor(UObject*, FFrame&);
	DefHookOg(void, ServerHandlePickupInfo, AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo& Params);

	//public:
	static void InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry& PickupEntry);
	//private:
	DefHookOg(char, CompletePickupAnimation, AFortPickup* Pickup);
	static void ServerAttemptInventoryDrop(AFortPlayerControllerAthena* PC, FGuid Guid, int32 Count, bool bTrash);

	DefHookOg(void, NetMulticast_Athena_BatchedDamageCues, AFortPlayerPawnAthena*, FAthenaBatchedDamageGameplayCues_Shared, FAthenaBatchedDamageGameplayCues_NonShared);

	static void ReloadWeapon(AFortWeapon*, int);

	DefHookOg(void, ServerHandlePickup, AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo Info);

	DefHookOg(void, DestroyPickup, AFortPickup* Pickup);

	DefHookOg(void, OnCapsuleBeginOverlap, AFortPlayerPawn* PlayerPawn, FFrame& Stack, void* Ret);

	DefHookOg(void, ServerPlayEmoteItemHook, AFortPlayerController* PlayerController, UFortItemDefinition* EmoteAsset, float RandomEmoteNumber);

	//DefHookOg(void, ServerAddMapMarker, UAthenaMarkerComponent* MarkerComponent, const FFortClientMarkerRequest& MarkerRequest);
	//DefHookOg(void, ServerRemoveMapMarker, UAthenaMarkerComponent* MarkerComponent, const FMarkerID& MarkerID, ECancelMarkerReason CancelReason);
	DefHookOg(void, MovingEmoteStopped, UObject* Context, FFrame& Stack);
	DefHookOg(void, GetPlayerViewPoint, AFortPlayerController* PC, FVector& Loc, FRotator& Rot);
	static void InitFortPlayerController();

	// _Impl helpers
	static void ServerAcknowledgePossessionHook_Impl(AFortPlayerControllerAthena* PC, APawn* Pawn);
	static void ServerExecuteInventoryItemHook_Impl(AFortPlayerControllerAthena* PC, FGuid Guid);
	static void ServerAttemptInventoryDrop_Impl(AFortPlayerControllerAthena* PC, FGuid Guid, int32 Count, bool bTrash);
	static void ServerAttemptAircraftJumpHook_Impl(UFortControllerComponent_Aircraft* Comp, FRotator ClientRot);
	static void GetPlayerViewPoint_Impl(AFortPlayerController* PC, FVector& Loc, FRotator& Rot);
	static void ServerClientIsReadyToRespawn_Impl(AFortPlayerControllerAthena* PC);
	static void ServerPlayEmoteItemHook_Impl(AFortPlayerController* PlayerController, UFortItemDefinition* EmoteAsset, float RandomEmoteNumber);
	static void ServerCheat_Impl(AFortPlayerControllerAthena* PC, FString Msg);
};