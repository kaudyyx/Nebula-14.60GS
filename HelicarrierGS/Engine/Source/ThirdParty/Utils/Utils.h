
#pragma once
#include "pch.h"
#include <xstring>

inline AFortAIPawn* DadBroPawn = nullptr;
inline AFortPlayerPawnAthena* Pawn = nullptr;
inline AFortAthenaAIBotController* PC = nullptr;
inline AActor* TargetActor = nullptr;
inline AActor* TargetGoTo = nullptr;
inline bool Emoting = false;
inline float FloatValue = 0.f;
struct FActorSpawnParameters
{
public:
	FName Name;

	AActor* Template;
	AActor* Owner;
	APawn* Instigator;
	ULevel* OverrideLevel;
	ESpawnActorCollisionHandlingMethod SpawnCollisionHandlingOverride;

private:
	uint8	bRemoteOwned : 1;
public:
	uint8	bNoFail : 1;
	uint8	bDeferConstruction : 1;
	uint8	bAllowDuringConstructionScript : 1;
	EObjectFlags ObjectFlags;
};

namespace Utils
{
	static FString generateIslandCodeThing()
	{
		std::wstringstream ss;
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_int_distribution<> dist(0, 9999);

		for (int i = 0; i < 3; ++i)
		{
			ss << std::setw(4) << std::setfill(L'0') << dist(gen);
			if (i < 2) ss << L"-";
		}

		return FString(ss.str().c_str());
	}
	static void ShowFoundation(ABuildingFoundation* BuildingFoundation, EDynamicFoundationEnabledState EnabledState = EDynamicFoundationEnabledState::Enabled)
	{
		BuildingFoundation->bServerStreamedInLevel = true;
		BuildingFoundation->DynamicFoundationType = EDynamicFoundationType::Static;
		BuildingFoundation->OnRep_ServerStreamedInLevel();

		BuildingFoundation->FoundationEnabledState = EnabledState;

		BuildingFoundation->DynamicFoundationRepData.EnabledState = EnabledState;
		BuildingFoundation->DynamicFoundationTransform = BuildingFoundation->GetTransform();

		//BuildingFoundation->DynamicFoundationRepData.Rotation = BuildingFoundation->DynamicFoundationTransform.Rotation;
		BuildingFoundation->OnRep_DynamicFoundationRepData();

		BuildingFoundation->FlushNetDormancy();
		BuildingFoundation->ForceNetUpdate();
	}
	inline void HookVTable(void* instance, uintptr_t methodIndex, void* hookFunction, void** originalFunction = nullptr) {
		if (!instance || !hookFunction)
			return;

		auto vtable = *reinterpret_cast<void***>(instance);
		if (!vtable || !vtable[methodIndex])
			return;

		if (originalFunction)
			*originalFunction = vtable[methodIndex];

		DWORD oldProtection;
		if (VirtualProtect(&vtable[methodIndex], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection)) {
			vtable[methodIndex] = hookFunction;
			VirtualProtect(&vtable[methodIndex], sizeof(void*), oldProtection, &oldProtection);
		}
	}
	inline static void* (*ApplyCharacterCustomization)(AFortPlayerStateAthena* a1, APawn* a2) = decltype(ApplyCharacterCustomization)(InSDKUtils::GetImageBase() + 0x4cd7248);

	inline UGameplayStatics* GetStatics()
	{
		return (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
	}

	inline void SetScalableFloatVal(SDK::FScalableFloat& Float, float Value)
	{
		Float.Value = Value;
		Float.Curve.CurveTable = nullptr;
		Float.Curve.RowName = SDK::FName();
	}

	inline bool bFirstChestSearched = false;
	inline UFortKismetLibrary* GetFortKismet()
	{
		return (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
	}

	template <typename T>
	static T* StaticFindObject2(std::string ObjectName)
	{
		auto Name = std::wstring(ObjectName.begin(), ObjectName.end()).c_str();

		static UObject* (*StaticFindObjectOriginal)(UClass * Class, UObject * Package, const TCHAR * OrigInName, bool ExactClass) = decltype(StaticFindObjectOriginal)(__int64(GetModuleHandleW(0)) + Addresses::StaticFindObject);

		return (T*)StaticFindObjectOriginal(T::StaticClass(), nullptr, Name, false);
	}


	inline float EvaluateCurveTableRow(UCurveTable* CurveTable, FName RowName, float InXY,
		const FString& ContextString = FString(), EEvaluateCurveTableResult* OutResult = nullptr)
	{
		/*static auto fn = StaticLoadObject<UFunction>("/Script/Engine.DataTableFunctionLibrary.EvaluateCurveTableRow");

		float wtf{};
		EEvaluateCurveTableResult wtf1{};

		struct { UCurveTable* CurveTable; FName RowName; float InXY; EEvaluateCurveTableResult OutResult; float OutXY; FString ContextString; }
		UDataTableFunctionLibrary_EvaluateCurveTableRow_Params{ CurveTable, RowName, InXY, wtf1, wtf, ContextString };

		static auto DefaultClass = UDataTableFunctionLibrary::StaticClass();
		DefaultClass->ProcessEvent(fn, &UDataTableFunctionLibrary_EvaluateCurveTableRow_Params);

		if (OutResult)
			*OutResult = UDataTableFunctionLibrary_EvaluateCurveTableRow_Params.OutResult;

		return UDataTableFunctionLibrary_EvaluateCurveTableRow_Params.OutXY;*/

		float wtf{};
		//EEvaluateCurveTableResult wtf1{};
		struct FCurveTableRowHandle RH {
			CurveTable, RowName
		};
		return Utils::GetFortKismet()->EvaluateCurveTableRow(RH, InXY, &wtf, ContextString);
	}

	inline AFortGameStateAthena* GetGameState()
	{
		return reinterpret_cast<AFortGameStateAthena*>(UWorld::GetWorld()->GameState);
	}
	inline AFortGameModeAthena* GetGameMode()
	{
		return (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
	}

	inline UKismetMathLibrary* GetMath()
	{
		return (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
	}


	inline int GetOffset(UObject* Object, string name)
	{
		FProperty* Property = nullptr;

		for (UStruct* Cls = Object->Class; Cls; Cls = Cls->Super)
		{
			FField* ChildProperties = Cls->ChildProperties;
			if (ChildProperties)
			{
				Property = (FProperty*)ChildProperties;
				string PropStr = ChildProperties->Name.ToString();
				while (Property)
				{
					if (PropStr == name)
						return Property->Offset;

					Property = (FProperty*)Property->Next;
					PropStr = Property ? Property->Name.ToString() : "Invalid Property";
				}
			}
		}
		if (!Property)
			return 0;
		return Property->Offset;
	}

	inline int EonGetOffset(UObject* Object, std::string Name)
	{
		for (UObject* ObjClass = Object->Class; ObjClass; ObjClass = *(UObject**)(__int64(ObjClass) + 0x40))
		{
			void* Property = *(void**)(__int64(ObjClass) + 0x50);

			if (Property)
			{
				FName* PropFName = (FName*)(__int64(Property) + 0x28);
				string PropName = PropFName ? PropFName->ToString() : "";

				while (Property)
				{
					if (PropName == Name)
					{
						return *(int*)(__int64(Property) + 0x4C);
					}

					Property = *(void**)(__int64(Property) + 0x20);
					PropFName = (FName*)(__int64(Property) + 0x28);
					PropName = Property ? (PropFName ? PropFName->ToString() : "None") : "None";
				}
			}
		}

		return 0;
	}

	template<typename T>
	bool VectorContains(T Item, std::vector<T>& Vector)
	{
		for (auto& Def : Vector)
		{
			if (Def == Item)
				return true;
		}
		return false;
	}

	static inline float EvaluateScalableFloat(FScalableFloat& Float)
	{
		if (!Float.Curve.CurveTable)
			return Float.Value;

		float Out;
		UDataTableFunctionLibrary::EvaluateCurveTableRow(Float.Curve.CurveTable, Float.Curve.RowName, (float)0, nullptr, &Out, FString());
		return Out;
	}
	inline FQuat RotatorToQuat(FRotator Rotation)
	{
		FQuat Quat;
		const float DEG_TO_RAD = 3.14159f / 180.0f;
		const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

		float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
		float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
		float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
		float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
		float SR = sin(Rotation.Roll * DIVIDE_BY_2);
		float CR = cos(Rotation.Roll * DIVIDE_BY_2);

		Quat.X = CR * SP * SY - SR * CP * CY;
		Quat.Y = -CR * SP * CY - SR * CP * SY;
		Quat.Z = CR * CP * SY - SR * SP * CY;
		Quat.W = CR * CP * CY + SR * SP * SY;

		return Quat;
	}

	static inline void* _NpFH = nullptr;
	template <typename _Ot = void*>
	inline __forceinline static void ExecHook(const char* _Name, void* _Detour, _Ot& _Orig = _NpFH)
	{
		auto _Fn = Utils::StaticFindObject2<UFunction>(_Name);
		if (!_Fn)
			return;
		if (!std::is_same_v<_Ot, void*>)
			_Orig = (_Ot)_Fn->ExecFunction;

		_Fn->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(_Detour);
	}



	static inline TArray<AActor*> PlayerStarts;
	inline static TArray<AActor*> BuildingFoundations;
	template <class T>
	TArray<T*> GetAllActorsOfClass() {
		TArray<T*> ResultActors;

		if (UWorld* World = UWorld::GetWorld()) {
			TArray<AActor*> OutActors;
			UGameplayStatics::GetAllActorsOfClass(World, T::StaticClass(), &OutActors);

			for (AActor* Actor : OutActors) {
				if (T* CastedActor = Cast<T>(Actor)) {
					ResultActors.Add(CastedActor);
				}
			}
		}
		return ResultActors;
	}

	template<typename T>
	inline vector<T*> GetAllObjectsOfClass(UClass* Class = T::StaticClass())
	{
		std::vector<T*> Objects{};

		for (int i = 0; i < UObject::GObjects->Num(); ++i)
		{
			UObject* Object = UObject::GObjects->GetByIndex(i);

			if (!Object)
				continue;

			if (Object->GetFullName().contains("Default"))
				continue;

			if (Object->GetFullName().contains("Test"))
				continue;

			if (Object->IsA(Class) && !Object->IsDefaultObject())
			{
				Objects.push_back((T*)Object);
			}
		}

		return Objects;
	}

	inline static std::vector<UAthenaCharacterItemDefinition*> Characters{};
	inline static std::vector<UAthenaPickaxeItemDefinition*> Pickaxes{};
	inline static std::vector<UAthenaBackpackItemDefinition*> Backpacks{};
	inline static std::vector<UAthenaGliderItemDefinition*> Gliders{};
	inline static std::vector<UAthenaSkyDiveContrailItemDefinition*> Contrails{};
	inline std::vector<UAthenaDanceItemDefinition*> Dances{};
	inline UAthenaNavSystem* AthenaNavSystem = nullptr;
	inline AFortAthenaMutator_Bots* BotMutator = nullptr;

	inline UObject* (*StaticFindObject_)(UClass* Class, UObject* Package, const wchar_t* OrigInName, bool ExactClass) = decltype(StaticFindObject_)(InSDKUtils::GetImageBase() + Addresses::StaticFindObject);
	template <typename T>
	inline T* StaticFindObject(std::string ObjectPath, UClass* Class = nullptr)
	{
		return (T*)StaticFindObject_(Class, nullptr, std::wstring(ObjectPath.begin(), ObjectPath.end()).c_str(), false);
	}

	inline UObject* (*StaticLoadObject_)(UClass* Class, UObject* InOuter, const TCHAR* Name, const TCHAR* Filename, uint32_t LoadFlags, UObject* Sandbox, bool bAllowObjectReconciliation) = decltype(StaticLoadObject_)(InSDKUtils::GetImageBase() + Addresses::StaticLoadObject);
	template <typename T>
	inline T* StaticLoadObject(std::string Path, UClass* InClass = T::StaticClass(), UObject* Outer = nullptr)
	{
		return (T*)StaticLoadObject_(InClass, Outer, std::wstring(Path.begin(), Path.end()).c_str(), nullptr, 0, nullptr, false);
	}

	template <typename T>
	__forceinline T* Cast(UObject* Object)
	{
		if (Object && Object->IsA(T::StaticClass()))
		{
			return (T*)Object;
		}

		return nullptr;
	}

	inline 	UFortPlaylistAthena* GetCurrentPlaylist()
	{
		return Utils::Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState)->CurrentPlaylistInfo.BasePlaylist;
	}

	static inline FQuat FRotToQuat(FRotator Rotation) {
		FQuat Quat;
		const float DEG_TO_RAD = 3.14159f / 180.0f;
		const float DIVIDE_BY_2 = DEG_TO_RAD / 2.0f;

		float SP = sin(Rotation.Pitch * DIVIDE_BY_2);
		float CP = cos(Rotation.Pitch * DIVIDE_BY_2);
		float SY = sin(Rotation.Yaw * DIVIDE_BY_2);
		float CY = cos(Rotation.Yaw * DIVIDE_BY_2);
		float SR = sin(Rotation.Roll * DIVIDE_BY_2);
		float CR = cos(Rotation.Roll * DIVIDE_BY_2);

		Quat.X = CR * SP * SY - SR * CP * CY;
		Quat.Y = -CR * SP * CY - SR * CP * SY;
		Quat.Z = CR * CP * SY - SR * SP * CY;
		Quat.W = CR * CP * CY + SR * SP * SY;

		return Quat;
	}

	template<typename T>
	inline T* SpawnActor22(FVector Loc, FRotator Rot = FRotator(), AActor* Owner = nullptr, SDK::UClass* Class = T::StaticClass(), ESpawnActorCollisionHandlingMethod Handle = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn)
	{
		FTransform Transform{};
		Transform.Scale3D = FVector{ 1,1,1 };
		Transform.Translation = Loc;
		Transform.Rotation = FRotToQuat(Rot);

		return (T*)UGameplayStatics::FinishSpawningActor(UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, Handle, Owner), Transform);
	}


	template <class T>
	inline T* SpawnActor(FVector Location, FRotator Rotation = FRotator{ 0, 0, 0 }, UClass* Class = T::StaticClass(), FVector Scale3D = { 1,1,1 }, ESpawnActorCollisionHandlingMethod SpawnMethod = ESpawnActorCollisionHandlingMethod::AlwaysSpawn)
	{
		FQuat Quat{};
		FTransform Transform{};

		auto DEG_TO_RAD = 3.14159 / 180;
		auto DIVIDE_BY_2 = DEG_TO_RAD / 2;

		auto SP = sin(Rotation.Pitch * DIVIDE_BY_2);
		auto CP = cos(Rotation.Pitch * DIVIDE_BY_2);

		auto SY = sin(Rotation.Yaw * DIVIDE_BY_2);
		auto CY = cos(Rotation.Yaw * DIVIDE_BY_2);

		auto SR = sin(Rotation.Roll * DIVIDE_BY_2);
		auto CR = cos(Rotation.Roll * DIVIDE_BY_2);

		Quat.X = CR * SP * SY - SR * CP * CY;
		Quat.Y = -CR * SP * CY - SR * CP * SY;
		Quat.Z = CR * CP * SY - SR * SP * CY;
		Quat.W = CR * CP * CY + SR * SP * SY;

		Transform.Rotation = Quat;
		Transform.Scale3D = Scale3D;
		Transform.Translation = Location;

		auto Actor = UGameplayStatics::GetDefaultObj()->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, SpawnMethod, nullptr);
		if (Actor)
			UGameplayStatics::GetDefaultObj()->FinishSpawningActor(Actor, Transform);
		return (T*)Actor;
	}

	static void HookVFT(void** Vft, uintptr_t Idx, void* Function, void** Original)
	{
		if (Original)
			*Original = Vft[Idx];

		DWORD Protection;
		if (VirtualProtect(&Vft[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &Protection))
		{
			Vft[Idx] = Function;
			VirtualProtect(&Vft[Idx], sizeof(void*), Protection, &Protection);
		}
	}

	inline void SwapVFTs(void* Base, uintptr_t Index, void* Detour, void** Original)
	{
		auto VTable = (*(void***)Base);
		if (!VTable)
			return;

		if (!VTable[Index])
			return;

		if (Original)
			*Original = VTable[Index];

		DWORD dwOld;
		VirtualProtect(&VTable[Index], 8, PAGE_EXECUTE_READWRITE, &dwOld);
		VTable[Index] = Detour;
		DWORD dwTemp;
		VirtualProtect(&VTable[Index], 8, dwOld, &dwTemp);
	}

	template<typename T>
	T* Actors(UClass* Class = T::StaticClass(), FVector Loc = {}, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		return SpawnActor22<T>(Loc, Rot, Owner, Class);
	}

	inline string FVectorToString(const FVector& vec)
	{
		ostringstream oss;
		oss << vec.X << " " << vec.Y << " " << vec.Z;
		return oss.str();
	}

	inline void SwapVTable(void* base, int Idx, void* Detour, void** OG)
	{
		if (!base)
			return;

		void** VTable = *(void***)base;
		if (!VTable || !VTable[Idx])
			return;
		if (OG)
		{
			*OG = VTable[Idx];
		}

		DWORD oldProtection;

		VirtualProtect(&VTable[Idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtection);

		VTable[Idx] = Detour;

		VirtualProtect(&VTable[Idx], sizeof(void*), oldProtection, NULL);
	}


	template <class T>
	inline T* BuildingSpawnActor(FVector Location, FRotator Rotation = FRotator{ 0, 0, 0 }, UClass* Class = T::StaticClass(), FVector Scale3D = { 1,1,1 })
	{
		FTransform Transform{};
		Transform.Rotation = UKismetMathLibrary::Conv_RotatorToTransform(Rotation).Rotation;
		Transform.Scale3D = Scale3D;
		Transform.Translation = Location;

		auto Actor = UGameplayStatics::GetDefaultObj()->BeginSpawningActorFromClass(UWorld::GetWorld(), Class, Transform, false, nullptr);
		if (Actor)
			UGameplayStatics::GetDefaultObj()->FinishSpawningActor(Actor, Transform);
		return (T*)Actor;
	}

	template<typename T>
	T* GetDefaultObject()
	{
		return (T*)T::StaticClass()->DefaultObject;
	}

	inline AFortPlayerControllerAthena* GetPCFromId(FUniqueNetIdRepl& ID)
	{
		for (auto& PlayerState : UWorld::GetWorld()->GameState->PlayerArray)
		{
			auto PlayerStateAthena = Utils::Cast<AFortPlayerStateAthena>(PlayerState);
			if (!PlayerStateAthena)
				continue;
			if (PlayerStateAthena->AreUniqueIDsIdentical(ID, PlayerState->UniqueId))
				return Utils::Cast<AFortPlayerControllerAthena>(PlayerState->Owner);
		}

		return nullptr;
	}

	template<typename T>
	T* SpawnActorLlama(FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		static UGameplayStatics* statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		FTransform Transform{};
		Transform.Scale3D = FVector(1, 1, 1);
		Transform.Translation = Loc;

		return (T*)statics->FinishSpawningActor(statics->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), T::StaticClass(), Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner), Transform);
	}

	template<typename T>
	T* SpawnActorLlama(UClass* Class, FVector Loc, FRotator Rot = {}, AActor* Owner = nullptr)
	{
		static UGameplayStatics* statics = (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;

		FTransform Transform{};
		Transform.Scale3D = FVector(1, 1, 1);
		Transform.Translation = Loc;
		Transform.Rotation = FRotToQuat(Rot);

		return (T*)statics->FinishSpawningActor(statics->BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner), Transform);
	}

	template <typename T>
	__forceinline T* Cast22(UObject* Object)
	{
		if (Object && Object->Class && Object->IsA(T::StaticClass()))
			return reinterpret_cast<T*>(Object);

		return nullptr;
	}
	template<typename T>
	std::vector<T*> GetObjectsOfClass(UClass* Class = T::StaticClass())
	{
		std::vector<T*> ArrayOfObjects;
		for (int i = 0; i < UObject::GObjects->Num(); i++)
		{
			UObject* Object = UObject::GObjects->GetByIndex(i);

			if (!Object)
				continue;

			if (Object->IsA(Class))
			{
				ArrayOfObjects.push_back(Cast22<T>(Object));
			}
		}

		return ArrayOfObjects;
	}

	template <class T>
	T* SpawnActorCreative(FTransform Transform = {}, UClass* Class = T::StaticClass(), FVector Scale3D = { 1,1,1 })
	{
		FQuat Quat{};

		auto Actor = UGameplayStatics::BeginSpawningActorFromClass(UWorld::GetWorld(), Class, Transform, false, nullptr);
		if (Actor) UGameplayStatics::FinishSpawningActor(Actor, Transform);
		return (T*)Actor;
	}

	template <class T>
	T* SpawnActorV3(FVector Location, FRotator Rotation = FRotator{ 0, 0, 0 }, UClass* Class = T::StaticClass(), AActor* Owner = nullptr, FVector Scale3D = { 1,1,1 })
	{
		FActorSpawnParameters addr{};

		addr.Owner = Owner;
		addr.bDeferConstruction = false;
		addr.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

		FQuat Quat{};
		FTransform Transform{};

		auto DEG_TO_RAD = 3.14159 / 180;
		auto DIVIDE_BY_2 = DEG_TO_RAD / 2;

		auto SP = sin(Rotation.Pitch * DIVIDE_BY_2);
		auto CP = cos(Rotation.Pitch * DIVIDE_BY_2);

		auto SY = sin(Rotation.Yaw * DIVIDE_BY_2);
		auto CY = cos(Rotation.Yaw * DIVIDE_BY_2);

		auto SR = sin(Rotation.Roll * DIVIDE_BY_2);
		auto CR = cos(Rotation.Roll * DIVIDE_BY_2);

		Quat.X = CR * SP * SY - SR * CP * CY;
		Quat.Y = -CR * SP * CY - SR * CP * SY;
		Quat.Z = CR * CP * SY - SR * SP * CY;
		Quat.W = CR * CP * CY + SR * SP * SY;

		Transform.Rotation = Quat;
		Transform.Scale3D = Scale3D;
		Transform.Translation = Location;

		auto Actor = ((AActor * (*)(UWorld*, UClass*, FTransform const*, FActorSpawnParameters*))(InSDKUtils::GetImageBase() + 0x4da8350))(UWorld::GetWorld(), Class, &Transform, &addr);
		return (T*)Actor;
	}

	inline void BotSpawnPickup(FVector Loc, UFortItemDefinition* Def, EFortPickupSourceTypeFlag Flag, EFortPickupSpawnSource SpawnSource, int Count = 1, int LoadedAmmo = 0, AFortPawn* Owner = nullptr)
	{
		AFortPickupAthena* Pickup = Utils::SpawnActor<AFortPickupAthena>(Loc);
		Pickup->bRandomRotation = true;
		Pickup->PrimaryPickupItemEntry.ItemDefinition = Def;
		Pickup->PrimaryPickupItemEntry.Count = Count;
		Pickup->PrimaryPickupItemEntry.LoadedAmmo = LoadedAmmo;
		Pickup->OnRep_PrimaryPickupItemEntry();

		if (Flag == EFortPickupSourceTypeFlag::Container)
		{
			Pickup->bTossedFromContainer = true;
			Pickup->OnRep_TossedFromContainer();
		}

		Pickup->TossPickup(Loc, nullptr, -1, true, false, Flag, SpawnSource);
	}

	inline AFortPickup* SpawnPickup(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, bool bShouldCombine = false, AFortPawn* Pawn = nullptr)
	{
		auto SpawnedPickup = Actors<AFortPickup>(AFortPickup::StaticClass(), Loc);
		SpawnedPickup->bRandomRotation = true;

		auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
		PickupEntry.ItemDefinition = ItemDef;
		PickupEntry.Count = OverrideCount;
		PickupEntry.LoadedAmmo = LoadedAmmo;
		PickupEntry.ReplicationKey++;
		SpawnedPickup->OnRep_PrimaryPickupItemEntry();
		SpawnedPickup->PawnWhoDroppedPickup = Pawn;

		SpawnedPickup->TossPickup(Loc, Pawn, -1, true, bShouldCombine, SourceType, Source);

		SpawnedPickup->SetReplicateMovement(true);
		SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

		if (SourceType == EFortPickupSourceTypeFlag::Container)
		{
			SpawnedPickup->bTossedFromContainer = true;
			SpawnedPickup->OnRep_TossedFromContainer();
		}

		return SpawnedPickup;
	}

	inline AFortPickupAthena* SpawnPickupProperFInishing(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
	{

		auto SpawnedPickup = Utils::SpawnActor22<AFortPickupAthena>(Loc, FRotator{}, nullptr, AFortPickupAthena::StaticClass());
		SpawnedPickup->bRandomRotation = true;

		auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
		PickupEntry.ItemDefinition = ItemDef;
		PickupEntry.Count = OverrideCount;
		PickupEntry.LoadedAmmo = LoadedAmmo;
		PickupEntry.ReplicationKey++;
		SpawnedPickup->OnRep_PrimaryPickupItemEntry();
		SpawnedPickup->PawnWhoDroppedPickup = Pawn;

		SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);

		SpawnedPickup->SetReplicateMovement(true);
		SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

		if (SourceType == EFortPickupSourceTypeFlag::Container)
		{
			SpawnedPickup->bTossedFromContainer = true;
			SpawnedPickup->OnRep_TossedFromContainer();
		}

		return SpawnedPickup;
	}

	inline AFortPickupAthena* SpawnPickupFishing(UFortItemDefinition* ItemDef, int OverrideCount, int LoadedAmmo, FVector Loc, EFortPickupSourceTypeFlag SourceType, EFortPickupSpawnSource Source, AFortPawn* Pawn = nullptr)
	{

		auto SpawnedPickup = Utils::SpawnActor22<AFortPickupAthena>(Loc, FRotator{}, nullptr, AFortPickupAthena::StaticClass());
		SpawnedPickup->bRandomRotation = true;

		auto& PickupEntry = SpawnedPickup->PrimaryPickupItemEntry;
		PickupEntry.ItemDefinition = ItemDef;
		PickupEntry.Count = OverrideCount;
		PickupEntry.LoadedAmmo = LoadedAmmo;
		PickupEntry.ReplicationKey++;
		SpawnedPickup->OnRep_PrimaryPickupItemEntry();
		SpawnedPickup->PawnWhoDroppedPickup = Pawn;

		SpawnedPickup->TossPickup(Loc, Pawn, -1, true, false, SourceType, Source);

		SpawnedPickup->SetReplicateMovement(true);
		SpawnedPickup->MovementComponent = (UProjectileMovementComponent*)GetDefaultObject<UGameplayStatics>()->SpawnObject(UProjectileMovementComponent::StaticClass(), SpawnedPickup);

		if (SourceType == EFortPickupSourceTypeFlag::Container)
		{
			SpawnedPickup->bTossedFromContainer = true;
			SpawnedPickup->OnRep_TossedFromContainer();
		}

		return SpawnedPickup;
	}


	template <typename T = AActor>
	static T* FinishSpawnActor(AActor* Actor, FVector Loc, FRotator Rot)
	{
		FTransform Transform(Loc, Rot);

		return (T*)UGameplayStatics::FinishSpawningActor(Actor, Transform);
	};
	inline string SplitString(bool SecondString, string delim, string strtosplit)
	{
		auto start = 0U;
		auto end = strtosplit.find(delim);
		if (SecondString)
		{
			while (end != std::string::npos)
			{
				start = end + delim.length();
				end = strtosplit.find(delim, start);
			}
		}

		return strtosplit.substr(start, end);
	}

	inline AFortPickupAthena* EonSpawnPickup(FVector Loc, UFortItemDefinition* Def, int Count, int LoadedAmmo, EFortPickupSourceTypeFlag SourceTypeFlag = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* Pawn = nullptr, bool Toss = true)
	{
		AFortPickupAthena* NewPickup = SpawnActor<AFortPickupAthena>(Loc);
		NewPickup->bRandomRotation = true;
		NewPickup->PrimaryPickupItemEntry.ItemDefinition = Def;
		NewPickup->PrimaryPickupItemEntry.Count = Count;
		NewPickup->PrimaryPickupItemEntry.LoadedAmmo = LoadedAmmo;
		NewPickup->OnRep_PrimaryPickupItemEntry();
		NewPickup->PawnWhoDroppedPickup = Pawn;

		NewPickup->TossPickup(Loc, Pawn, -1, Toss, false, SourceTypeFlag, SpawnSource);

		if (SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox)
		{
			NewPickup->bTossedFromContainer = true;
			NewPickup->OnRep_TossedFromContainer();
		}

		return NewPickup;
	}

	inline AFortPickupAthena* EonSpawnPickup(FVector Loc, FFortItemEntry* Entry, EFortPickupSourceTypeFlag SourceTypeFlag = EFortPickupSourceTypeFlag::Tossed, EFortPickupSpawnSource SpawnSource = EFortPickupSpawnSource::Unset, AFortPlayerPawn* Pawn = nullptr, int OverrideCount = 0) //test
	{
		if (!Entry)
			return nullptr;

		AFortPickupAthena* NewPickup = SpawnActor<AFortPickupAthena>(Loc);
		if (!NewPickup || NewPickup->bPickedUp /* this shouldnt be set, but here just incase. */) return nullptr;
		NewPickup->bRandomRotation = true;
		//NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != 0 ? OverrideCount : Entry->Count;
		//NewPickup->PrimaryPickupItemEntry.LoadedAmmo = Entry->LoadedAmmo;
		//NewPickup->PrimaryPickupItemEntry.ItemDefinition = Entry->ItemDefinition;
		//NewPickup->PrimaryPickupItemEntry.ItemGuid = Entry->ItemGuid;
		NewPickup->PrimaryPickupItemEntry = *Entry;
		NewPickup->PrimaryPickupItemEntry.Count = OverrideCount != 0 ? OverrideCount : Entry->Count;
		NewPickup->OnRep_PrimaryPickupItemEntry();
		NewPickup->PawnWhoDroppedPickup = Pawn;

		NewPickup->TossPickup(Loc, Pawn, -1, true, false, SourceTypeFlag, SpawnSource);

		if (SpawnSource == EFortPickupSpawnSource::Chest || SpawnSource == EFortPickupSpawnSource::AmmoBox)
		{
			NewPickup->bTossedFromContainer = true;
			NewPickup->OnRep_TossedFromContainer();
		}

		return NewPickup;
	}

	template <typename T = UObject>
	inline T* Cast2(UObject* Object, bool bCastOnly = false)
	{
		if (Object)
		{
			if (!bCastOnly)
			{
				return Object->IsA(T::StaticClass()) ? (T*)Object : nullptr;
			}
			else
			{
				return (T*)Object;
			}
		}
		else
		{
			return nullptr;
		}
	}

	struct FParseConditionResult
	{
		bool bMatch;
		size_t NextStart;
	};

	static FParseConditionResult ParseCondition(xstring Condition, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags)
	{
		size_t CondTypeStart = -1, CondTypeEnd = -1, NextCond = -1;

		for (auto& c : Condition)
		{
			if (c == '>' || c == '<' || c == '=' || c == '&')
			{
				CondTypeStart = __int64(&c - Condition.data());
			}
			else if (CondTypeStart != -1 && (c != '>' && c != '<' && c != '=' && c != '&'))
			{
				CondTypeEnd = __int64(&c - Condition.data());
			}
			else if (CondTypeEnd != -1 && (c == '=' || c == '&'))
			{
				NextCond = __int64(&c - Condition.data());
			}

			if (CondTypeStart != -1 && CondTypeEnd != -1 && NextCond != -1)
				break;
		}

		if (CondTypeStart == Condition.npos)
		{
			CondTypeStart = Condition.find(" ");

			if (CondTypeStart == Condition.npos)
				return { false, NextCond };

			CondTypeStart++;

			if (CondTypeEnd == Condition.npos)
				CondTypeEnd = Condition.find(" ", CondTypeStart);

			NextCond = Condition.find("&&", CondTypeEnd + 1);
			if (NextCond == Condition.npos)
				NextCond = Condition.find("||", CondTypeEnd + 1);
		}
		else if (CondTypeEnd == Condition.npos)
		{
			CondTypeEnd = CondTypeStart + 1;
		}

		auto Left = Condition.substr(0, CondTypeStart - 1);
		auto CondType = Condition.substr(CondTypeStart, CondTypeEnd - CondTypeStart);
		auto Right = Condition.substr(CondTypeEnd + 1, NextCond == Condition.npos ? NextCond : (Condition.substr(NextCond - 1, 1) == " " ? NextCond - 1 : NextCond) - CondTypeEnd - 1);

		if (CondType == "HasTag" || CondType == "MissingTag")
		{
			FGameplayTagContainer Container;

			if (Left == "Target.Tags")
			{
				Container = TargetTags;
			}
			else if (Left == "Source.Tags")
			{
				Container = SourceTags;
			}
			else if (Left == "Context.Tags")
			{
				Container = ContextTags;
			}
			else
			{
				return { false, NextCond };
			}

			std::string RightStr(Right.c_str());
			bool bFound = false;
			for (auto& T : Container.GameplayTags) {
				if (T.TagName.ToString() == RightStr) { bFound = true; break; }
			}
			if (!bFound) {
				for (auto& T : Container.ParentTags) {
					if (T.TagName.ToString() == RightStr) { bFound = true; break; }
				}
			}

			if (CondType == "HasTag")
			{
				return { bFound, NextCond };
			}
			else
			{
				return { !bFound, NextCond };
			}
		}
		else
		{
			//Utils::Log("Hit unimplemented condition: ", Left, " ", CondType, " ", Right);
		}
		return { false, NextCond };
	}

	static bool IsConditionMet(const FString& InCondition, const FGameplayTagContainer& TargetTags, const FGameplayTagContainer& SourceTags, const FGameplayTagContainer& ContextTags)
	{
		xstring Condition(InCondition.ToString().c_str());

		if (Condition.empty())
			return true;

		FParseConditionResult Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

		if (Result.NextStart != Condition.npos)
		{
		loop:
			if (Result.NextStart == Condition.npos)
				return Result.bMatch;
			auto Start = Condition.substr(Result.NextStart, 2);

			if (Start == "&&")
			{
				Condition = Condition.substr(Result.NextStart + 2);

				if (Condition.substr(0, 1) == " ")
					Condition = Condition.substr(1);

				auto LastResult = Result;

				Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

				if (!LastResult.bMatch || !Result.bMatch)
					Result.bMatch = false;

				goto loop;
			}
			else if (Start == "||")
			{
				Condition = Condition.substr(Result.NextStart + 2);

				if (Condition.substr(0, 1) == " ")
					Condition = Condition.substr(1);

				auto LastResult = Result;

				Result = ParseCondition(Condition, TargetTags, SourceTags, ContextTags);

				if (LastResult.bMatch || Result.bMatch)
					Result.bMatch = true;

				goto loop;
			}
			else
			{
				return Result.bMatch;
			}
		}

		return Result.bMatch;
	}

}

template<typename T>
std::vector<T*> GetObjectsOfClass2(UClass* Class = T::StaticClass())
{
	std::vector<T*> ArrayOfObjects;
	for (int i = 0; i < UObject::GObjects->Num(); i++)
	{
		UObject* Object = UObject::GObjects->GetByIndex(i);

		if (!Object)
			continue;

		if (Object->IsA(Class))
		{
			ArrayOfObjects.push_back(Utils::Cast<T>(Object));
		}
	}

	return ArrayOfObjects;
}

inline static void (*FGameplayAbilitySpecCtor)(FGameplayAbilitySpec*, UGameplayAbility*, int, int, UObject*) = decltype(FGameplayAbilitySpecCtor)(__int64(GetModuleHandleW(0)) + 0xa1a2d0);
inline static __int64(__fastcall* GiveAbilityAndActivateOnce)(void*, FGameplayAbilitySpecHandle*, FGameplayAbilitySpec) = decltype(GiveAbilityAndActivateOnce)(__int64(GetModuleHandleW(0)) + 0xa3e3c0);

inline void PlayRiftAnim(AFortPlayerControllerAthena* PC)
{
	UClass* TestAbility = Utils::StaticLoadObject<UClass>("/Game/Athena/Items/ForagedItems/Rift/GA_Rift_Athena_Skydive.GA_Rift_Athena_Skydive_C");

	FGameplayAbilitySpec Spec{};
	FGameplayAbilitySpecCtor(&Spec, (UGameplayAbility*)TestAbility->DefaultObject, 1, -1, nullptr);

	GiveAbilityAndActivateOnce(PC->MyFortPawn->AbilitySystemComponent, &Spec.Handle, Spec);
}
inline TArray<FVector> PickedllamaLocations;
inline std::map<std::string, FVector> Waypoints;
static FVector* (*PickSupplyDropLocationOG)(AFortAthenaMapInfo* MapInfo, FVector* outLocation, __int64 Center, float Radius) = decltype(PickSupplyDropLocationOG)(__int64(GetModuleHandleA(0)) + 0x2033D00);
inline FVector PickSupplyDropLocation(AFortAthenaMapInfo* MapInfo, FVector Center, float Radius)
{
	if (!PickSupplyDropLocationOG)
		return FVector(0, 0, 0);

	const float MinDistance = 10000.0f; //idk whats the best distance

	for (int i = 0; i < 20; i++)
	{
		FVector loc = FVector(0, 0, 0);
		PickSupplyDropLocationOG(MapInfo, &loc, (__int64)&Center, Radius);

		bool bTooClose = false;
		for (const auto& other : PickedllamaLocations)
		{
			float dx = loc.X - other.X;
			float dy = loc.Y - other.Y;
			float dz = loc.Z - other.Z;

			float distSquared = dx * dx + dy * dy + dz * dz;

			if (distSquared < MinDistance * MinDistance)
			{
				bTooClose = true;
				break;
			}
		}

		if (!bTooClose)
		{
			PickedllamaLocations.Add(loc);
			return loc;
		}
	}

	return FVector(0, 0, 0);
}

inline UFortPlaylistAthena* GetPlaylist() {
	auto Playlist = Utils::Cast<AFortGameStateAthena>(UWorld::GetWorld()->GameState)->CurrentPlaylistInfo.BasePlaylist;

	if (Playlist)
		return Playlist;

	return nullptr;
}


static inline AFortGameStateAthena* GetGameState()
{
	return (AFortGameStateAthena*)UWorld::GetWorld()->GameState;
}
static inline AFortGameModeAthena* GetGameMode()
{
	return (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
}

static inline UFortKismetLibrary* GetFortKismet()
{
	return (UFortKismetLibrary*)UFortKismetLibrary::StaticClass()->DefaultObject;
}

static inline UGameplayStatics* GetStatics()
{
	return (UGameplayStatics*)UGameplayStatics::StaticClass()->DefaultObject;
}

static inline UKismetMathLibrary* GetMath()
{
	return (UKismetMathLibrary*)UKismetMathLibrary::StaticClass()->DefaultObject;
}

inline float FRotator::ClampAxis(float Angle)
{
	Angle = fmod(Angle, 360.f); // rat

	if (Angle < 0.)
		Angle += 360.;
	return Angle;
}

inline FRotator FQuat::Rotator()
{
	const float SingularityTest = Z * X - W * Y;
	const float YawY = 2.f * (W * Z + X * Y);
	const float YawX = (1.f - 2.f * ((Y * Y) + (Z * Z)));

	const float SINGULARITY_THRESHOLD = 0.4999995f;
	const float RAD_TO_DEG = 57.29577951308232f;
	FRotator RotatorFromQuat{};

	if (SingularityTest < -SINGULARITY_THRESHOLD)
	{
		RotatorFromQuat.Pitch = -90.f;
		RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = FRotator::NormalizeAxis(-RotatorFromQuat.Yaw - (2.f * atan2f(X, W) * RAD_TO_DEG));
	}
	else if (SingularityTest > SINGULARITY_THRESHOLD)
	{
		RotatorFromQuat.Pitch = 90.f;
		RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = FRotator::NormalizeAxis(RotatorFromQuat.Yaw - (2.f * atan2f(X, W) * RAD_TO_DEG));
	}
	else
	{
		RotatorFromQuat.Pitch = asinf(2.f * SingularityTest) * RAD_TO_DEG;
		RotatorFromQuat.Yaw = atan2f(YawY, YawX) * RAD_TO_DEG;
		RotatorFromQuat.Roll = atan2f(-2.f * (W * X + Y * Z), (1.f - 2.f * ((X * X) + (Y * Y)))) * RAD_TO_DEG;
	}

	return RotatorFromQuat;
}
static double precision(double f, double places)
{
	double n = pow(10., places);
	return round(f * n) / n;
}
inline FVector_NetQuantize100 FVector::Quantize100()
{
	FVector_NetQuantize100 ret;
	ret.X = precision(X, 2);
	ret.Y = precision(Y, 2);
	ret.Z = precision(Z, 2);
	return ret;
}



template<typename UEType>
UEType* SDK::TSoftObjectPtr<UEType>::NewGet() const
{
	std::string String = UKismetStringLibrary::Conv_NameToString(this->ObjectID.AssetPathName).ToString();
	//printf("Getting %s\n", String.c_str());
	if (this->WeakPtr.ObjectIndex != -1)
		return Utils::Cast<UEType>(UObject::GObjects->GetByIndex(this->WeakPtr.ObjectIndex));
	else if (this->ObjectID.IsValid())
		return Utils::StaticLoadObject<UEType>(String);

	return nullptr;
}


template<typename UEType>
UEType* SDK::TSoftClassPtr<UEType>::NewGet() const
{
	std::string String = UKismetStringLibrary::Conv_NameToString(this->ObjectID.AssetPathName).ToString();
	//printf("Getting %s\n", String.c_str());
	if (this->WeakPtr.ObjectIndex != -1)
		return Utils::Cast<UEType>(UObject::GObjects->GetByIndex(this->WeakPtr.ObjectIndex));
	else if (this->ObjectID.IsValid())
		return Utils::StaticLoadObject<UEType>(String);

	return nullptr;
}

inline UFortWorldItem* FindItemInstance(AFortInventory* inv, UFortItemDefinition* ItemDefinition)
{
	auto& ItemInstances = inv->Inventory.ItemInstances;

	for (int i = 0; i < ItemInstances.Num(); i++)
	{
		auto ItemInstance = ItemInstances[i];

		if (ItemInstance->ItemEntry.ItemDefinition == ItemDefinition)
			return ItemInstance;
	}

	return nullptr;
}

inline EAthenaGamePhaseStep GetCurrentGamePhaseStep(AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState) {
	float CurrentTime = UGameplayStatics::GetTimeSeconds(UWorld::GetWorld());

	if (GameState->GamePhase == EAthenaGamePhase::Setup) {
		return EAthenaGamePhaseStep::Setup;
	}
	else if (GameState->GamePhase == EAthenaGamePhase::Warmup) {
		if (GameState->WarmupCountdownEndTime > CurrentTime + 10.f) {
			return EAthenaGamePhaseStep::Warmup;
		}
		else {
			return EAthenaGamePhaseStep::GetReady;
		}
	}
	else if (GameState->GamePhase == EAthenaGamePhase::Aircraft) {
		if (!GameState->bAircraftIsLocked) {
			return EAthenaGamePhaseStep::BusFlying;
		}
		else {
			return EAthenaGamePhaseStep::BusLocked;
		}
	}
	else if (GameState->GamePhase == EAthenaGamePhase::SafeZones) {
		if (!GameState->SafeZoneIndicator) {
			return EAthenaGamePhaseStep::StormForming;
		}
		else if (GameState->SafeZoneIndicator->bPaused) {
			return EAthenaGamePhaseStep::StormHolding;
		}
		else {
			return EAthenaGamePhaseStep::StormShrinking;
		}
	}
	else if (GameState->GamePhase == EAthenaGamePhase::EndGame) {
		return EAthenaGamePhaseStep::EndGame;
	}
	else if (GameState->GamePhase == EAthenaGamePhase::Count) {
		return EAthenaGamePhaseStep::Count;
	}
	else {
		return EAthenaGamePhaseStep::EAthenaGamePhaseStep_MAX;
	}
}

inline void UpdateBotBlackboard(AFortAthenaAIBotController* bot, AFortGameModeAthena* GameMode, AFortGameStateAthena* GameState) {
	if (!bot) return;
	if (!bot->Blackboard) return;
	if (!GameState) return;

	bot->Blackboard->SetValueAsEnum(UKismetStringLibrary::GetDefaultObj()->Conv_StringToName(L"AIEvaluator_Global_GamePhaseStep"), (int)GameState->GamePhaseStep);
}

template<typename UEType>
UEType* TSoftClassPtr<UEType>::LoadAsset() const
{
	if (!this->ObjectID.AssetPathName.IsValid())
		return nullptr;
	return Utils::StaticLoadObject<UEType>(UKismetStringLibrary::Conv_NameToString(this->ObjectID.AssetPathName).ToString());
}

enum ELoadFlags
{
	LOAD_None = 0x00000000,	///< No flags.
	LOAD_Async = 0x00000001,	///< Loads the package using async loading path/ reader
	LOAD_NoWarn = 0x00000002,	///< Don't display warning if load fails.
	LOAD_EditorOnly = 0x00000004,	///< Load for editor-only purposes and by editor-only code
	LOAD_ResolvingDeferredExports = 0x00000008,	///< Denotes that we should not defer export loading (as we're resolving them)
	LOAD_Verify = 0x00000010,	///< Only verify existance; don't actually load.
	//	LOAD_Unused						= 0x00000020,	///< Allow plain DLLs.
	//	LOAD_Unused						= 0x00000040
	LOAD_NoVerify = 0x00000080,   ///< Don't verify imports yet.
	LOAD_IsVerifying = 0x00000100,	///< Is verifying imports
	LOAD_SkipLoadImportedPackages = 0x00000200,	///< Assume that all import packages are already loaded and don't call LoadPackage when creating imports 
	//	LOAD_Unused						= 0x00000400,
	//	LOAD_Unused						= 0x00000800,
	LOAD_DisableDependencyPreloading = 0x00001000,	///< Bypass dependency preloading system
	LOAD_Quiet = 0x00002000,   ///< No log warnings.
	LOAD_FindIfFail = 0x00004000,	///< Tries FindObject if a linker cannot be obtained (e.g. package is currently being compiled)
	LOAD_MemoryReader = 0x00008000,	///< Loads the file into memory and serializes from there.
	LOAD_NoRedirects = 0x00010000,	///< Never follow redirects when loading objects; redirected loads will fail
	LOAD_ForDiff = 0x00020000,	///< Loading for diffing in the editor
	LOAD_PackageForPIE = 0x00080000,   ///< This package is being loaded for PIE, it must be flagged as such immediately
	LOAD_DeferDependencyLoads = 0x00100000,   ///< Do not load external (blueprint) dependencies (instead, track them for deferred loading)
	LOAD_ForFileDiff = 0x00200000,	///< Load the package (not for diffing in the editor), instead verify at the two packages serialized output are the same, if they are not then debug break so that you can get the callstack and object information
	LOAD_DisableCompileOnLoad = 0x00400000,	///< Prevent this load call from running compile on load for the loaded blueprint (intentionally not recursive, dependencies will still compile on load)
	LOAD_DisableEngineVersionChecks = 0x00800000,   ///< Prevent this load call from running engine version checks
};

template <typename T>
static T* StaticLoadObjectGay(const TCHAR* Name, const TCHAR* Filename = nullptr, uint32 LoadFlags = LOAD_None, UPackageMap* Sandbox = nullptr, bool bAllowObjectReconciliation = true)
{
	// 7FF697676B08
	UObject* (*StaticLoadObjectw312321)(UClass * Class, UObject * InOuter, const TCHAR * Name, const TCHAR * Filename, uint32 LoadFlags, UPackageMap * Sandbox, bool bAllowObjectReconciliation, __int64 a6) = decltype(StaticLoadObjectw312321)(Addresses::StaticLoadObject + uintptr_t(GetModuleHandle(0)));
	return (T*)StaticLoadObjectw312321(T::StaticClass(), nullptr, Name, Filename, LoadFlags, Sandbox, bAllowObjectReconciliation, 0);
}

inline UClass* LoadClass(TSoftClassPtr<UClass> SoftClass)
{
	UClass* Class = SoftClass.Get();

	if (!Class && SoftClass.ObjectID.AssetPathName.IsValid())
	{
		const FString& AssetPathName = UKismetStringLibrary::Conv_NameToString(SoftClass.ObjectID.AssetPathName);
		Class = StaticLoadObjectGay<UClass>(AssetPathName.CStr());
	}

	return Class;
}

template<typename T>
T* GetInterface(UObject* Object) {
	void* (*GetInterface_)(UObject*, UClass*) = decltype(GetInterface_)(InSDKUtils::GetImageBase() + 0x3764A90);//48 89 5C 24 ? 48 89 74 24 ? 57 48 83 EC 20 33 DB 48 8B FA 48 8B F1 48 85 D2 0F 84 ? ? ? ? 8B 82 ? ? ? ? C1 E8
	return (T*)GetInterface_(Object, T::StaticClass());
}
