#include "pch.h"
#include "../Public/FFrame.h"

void FFrame::StepCompiledIn(void* const Result, bool ForceExplicitProp)
{
    if (Code && !ForceExplicitProp)
    {
        ((void (*)(FFrame*, UObject*, void* const)) (ImageBase + Addresses::Step))(this, Object, Result);
    }
    else
    {
        FField* _Prop = PropertyChainForCompiledIn;
        PropertyChainForCompiledIn = _Prop->Next;
        ((void (*)(FFrame*, void* const, FField*)) (ImageBase + Addresses::StepExplicitProperty))(this, Result, _Prop);
    }
}


void FFrame::IncrementCode() {
    Code = (uint8_t*)(__int64(Code) + (bool)Code);
}
