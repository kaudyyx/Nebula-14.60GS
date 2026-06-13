#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>


class AthenaMarkerComponent
{
public:
	static void ServerAddMapMarker(UAthenaMarkerComponent* Comp, const FFortClientMarkerRequest& MarkerRequest);
public:
	static void Setup();
};
