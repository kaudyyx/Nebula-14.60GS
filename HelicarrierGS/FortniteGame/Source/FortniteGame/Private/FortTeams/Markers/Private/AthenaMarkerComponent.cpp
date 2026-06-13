#include "pch.h"
#include <FortniteGame/Source/FortniteGame/Public/FortTeams/Markers/Public/AthenaMarkerComponent.h>
static FFortWorldMarkerData* (*FFortWorldMarkerDataCtor)(FFortWorldMarkerData* a1) = decltype(FFortWorldMarkerDataCtor)(__int64(GetModuleHandleW(0)) + 0x128F150);//in execClientAddMarker first sub intro the func sub_128F150((__int64)v9);                     // offset for FFortWorldMarkerDataCtor
void AthenaMarkerComponent::ServerAddMapMarker(UAthenaMarkerComponent* Comp, const FFortClientMarkerRequest& MarkerRequest)
{
	AFortPlayerControllerAthena* PC = (AFortPlayerControllerAthena*)Comp->GetOwner();
	AFortPlayerStateAthena* PlayerState = (AFortPlayerStateAthena*)PC->PlayerState;

	FFortWorldMarkerData MarkerData{};
//	FFortWorldMarkerDataCtor(&MarkerData);
	FMarkerID MarkerID{};
	MarkerID.PlayerId = PlayerState->PlayerId;
	MarkerID.InstanceID = MarkerRequest.InstanceID;
	MarkerData.MarkerType = MarkerRequest.MarkerType;
	MarkerData.Owner = PlayerState;
	MarkerData.BasePosition = MarkerRequest.BasePosition;
	MarkerData.WorldNormal = MarkerRequest.WorldNormal;
	MarkerData.BasePositionOffset = MarkerRequest.BasePositionOffset;
	MarkerData.MarkerID = MarkerID;

	for (size_t i = 0; i < PlayerState->PlayerTeam->TeamMembers.Num(); i++)
	{
		if (PlayerState->PlayerTeam->TeamMembers[i]->IsA(AFortPlayerControllerAthena::StaticClass()) && PlayerState->PlayerTeam->TeamMembers[i] != PC)
		{
			((AFortPlayerControllerAthena*)PlayerState->PlayerTeam->TeamMembers[i])->MarkerComponent->ServerAddMapMarker(MarkerRequest);
		}
	}
}


void AthenaMarkerComponent::Setup()
{
	//Utils::SwapVTable(UAthenaMarkerComponent::StaticClass()->DefaultObject, 0x80, ServerRemoveMapMarker, nullptr);
	Utils::SwapVTable(UAthenaMarkerComponent::StaticClass()->DefaultObject, 0x81, ServerAddMapMarker, nullptr);
}

//3776680