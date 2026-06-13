#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>
#include <FortniteGame/Source/FortniteGame/Public/Inventory/Public/Inventory.h>

class FortPlayerPawn
{
public:
	class Original
	{
	public:
		static inline void (*GivePickupTo)(AFortPickup* Pickup, IFortInventoryOwnerInterface* InventoryOwner, bool DestroyAfterPickup);
	};
	//static void ServerHandlePickupInfo(AFortPlayerPawn* Pawn, AFortPickup* PickUp, const FFortPickupRequestInfo& Params_0);
	DefHookOg(void, ServerHandlePickupInfo, AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo& Params);
	static void InternalPickup(AFortPlayerControllerAthena* PlayerController, FFortItemEntry& PickupEntry);
	DefHookOg(char, CompletePickupAnimation, AFortPickup* Pickup);
	DefHookOg(void, OnCapsuleBeginOverlap, AFortPlayerPawn* PlayerPawn, FFrame& Stack, void* Ret);
	static void ServerAttemptInteract(UFortControllerComponent_Interaction* ControllerComp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData, EInteractionBeingAttempted InteractionBeingAttempted, int32 RequestId);
	static inline void (*ServerAttemptInteractOG)(UFortControllerComponent_Interaction* ControllerComp, AActor* ReceivingActor, UPrimitiveComponent* InteractComponent, ETInteractionType InteractType, UObject* OptionalObjectData, EInteractionBeingAttempted InteractionBeingAttempted, int32 RequestId);
	static inline void (*OpenActorOG)(UObject* Context, FFrame& Stack);
	static void OpenActor(UObject* Context, FFrame& Stack);
	static void GivePickupTo(AFortPickup* Pickup, IFortInventoryOwnerInterface* InventoryOwner, bool DestroyAfterPickup);
	static void SpawnItemVariantPickupInWorld(UObject*, FFrame&, AFortPickupAthena**);

	static void InitFortPlayerPawn();

	// _Impl helpers
	static void ServerHandlePickupInfo_Impl(AFortPlayerPawnAthena* Pawn, AFortPickup* Pickup, FFortPickupRequestInfo& Params);
	static char CompletePickupAnimation_Impl(AFortPickup* Pickup);
	static void OnCapsuleBeginOverlap_Impl(AFortPlayerPawn* PlayerPawn, FFrame& Stack, void* Ret);
};