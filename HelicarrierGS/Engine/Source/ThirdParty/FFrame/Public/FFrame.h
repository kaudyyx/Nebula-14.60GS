#pragma once
#include "pch.h"
#include <Engine/Source/ThirdParty/Utils/Utils.h>

class FOutputDevice
{
public:
    bool bSuppressEventTag;
    bool bAutoEmitLineTerminator;
    uint8_t _Padding1[0x6];
};

class FFrame : public FOutputDevice
{
public:
    void** VTable;
    UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;
    FProperty* MostRecentProperty;
    uint8_t* MostRecentPropertyAddress;
    uint8_t _Padding1[0x40];
    FField* PropertyChainForCompiledIn;

public:
    void StepCompiledIn(void* const Result, bool ForceExplicitProp = false);

    template <typename T>
    T& StepCompiledInRef() {
        T TempVal{};
        MostRecentPropertyAddress = nullptr;

        if (Code)
        {
            ((void (*)(FFrame*, UObject*, void* const)) (ImageBase + Addresses::Step))(this, Object, &TempVal);
        }
        else
        {
            FField* _Prop = PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = _Prop->Next;
            ((void (*)(FFrame*, void* const, FField*)) (ImageBase + Addresses::StepExplicitProperty))(this, &TempVal, _Prop);
        }

        return MostRecentPropertyAddress ? *(T*)MostRecentPropertyAddress : TempVal;
    }

    void IncrementCode();
};
static_assert(offsetof(FFrame, Object) == 0x18, "FFrame::Object offset is wrong!");
static_assert(offsetof(FFrame, Code) == 0x20, "FFrame::Code offset is wrong!");
static_assert(offsetof(FFrame, Object) == 0x18, "FFrame::Object offset is wrong!");

inline vector<void(*)()> _HookFuncs;
#define DefHookOg(_Rt, _Name, ...) static inline _Rt (*_Name##OG)(##__VA_ARGS__); static _Rt _Name(##__VA_ARGS__); 
#define callOG(_Tr, _Pt, _Th, ...) ([&](){ auto _Fn = Utils::StaticFindObject<UFunction>(_Pt "." # _Th); _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th##OG; _Tr->_Th(##__VA_ARGS__); _Fn->ExecFunction = (UFunction::FNativeFuncPtr) _Th; })()
#define DefUHookOg(_Name) static inline void (*_Name##OG)(UObject*, FFrame&); static void _Name(UObject*, FFrame&); 
#define InitMinHook auto _MHInitter = MH_Initialize();
#define InitHooks static void Hook(); static int _AddHook() { _HookFuncs.push_back(Hook); return 0; }; static inline auto _HookAdder = _AddHook();
#define EnableHooks auto _MHEnabler = MH_EnableHook(MH_ALL_HOOKS);
#define DispatchHooks for (auto& DispatchHook : _HookFuncs) DispatchHook(); MH_EnableHook(MH_ALL_HOOKS);
#define __runOnce(_V) static uint32_t _ROnce_##_V = 0; if (++_ROnce_##_V == 1)
#define _runOnce(_V) __runOnce(_V)
#define runOnce _runOnce(__COUNTER__)
#define UPROPERTY(TYPE, NAME) \
	TYPE NAME = {}; \
	Stack.StepCompiledIn(&NAME)