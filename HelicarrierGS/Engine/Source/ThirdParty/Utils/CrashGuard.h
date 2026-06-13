#pragma once
#include <Windows.h>
#include "pch.h"

// ============================================================
// CrashGuard — protecao global contra crashes do servidor
//
// Tres camadas:
//  1. SetUnhandledExceptionFilter  — captura qualquer crash
//     nao tratado antes que o Windows encerre o processo.
//  2. ProcessEvent hook com SEH    — envolve TODA chamada de
//     evento Blueprint/nativo do engine.
//  3. NEBULA_SEH_* macros          — para hookear funcoes
//     individuais que contenham objetos C++ com destrutor
//     (MSVC nao permite __try diretamente nelas).
// ============================================================

// ------------------------------------------------------------
// 1a. VEH — Vectored Exception Handler (roda ANTES do SEH e
//     antes do UnhandledExceptionFilter, inclusive antes do
//     handler interno do UE4 que chama TerminateProcess).
//
//     Registrado com prioridade 1 (primeiro da lista), entao
//     nosso handler roda antes do UE4.
//
//     Estrategia para null-deref (fault addr < 0x10000):
//       - Seta RAX = 0 (valor de retorno seguro)
//       - Le o return address do topo da pilha (RSP)
//       - Faz o RIP pular para o caller (fake return)
//     Isso faz a funcao crashante "retornar 0" ao seu caller
//     em vez de derrubar o processo.
//
//     Cobertura principal: crash 0x00000030 no FlushAsyncLoading
//     dentro de StartAircraftPhaseOG (Hightower building deref).
// ------------------------------------------------------------
static PVOID g_NebulaVehHandle = nullptr;

static LONG CALLBACK NebulaVEH(EXCEPTION_POINTERS* ep)
{
    if (ep->ExceptionRecord->ExceptionCode != EXCEPTION_ACCESS_VIOLATION)
        return EXCEPTION_CONTINUE_SEARCH;

    // Endereco da falha — se for regiao de null pointer (< 64KB)
    // e uma leitura ou escrita invalida, aplicamos o fake-return.
    ULONG_PTR faultAddr = ep->ExceptionRecord->ExceptionInformation[1];
    if (faultAddr >= 0x10000)
        return EXCEPTION_CONTINUE_SEARCH; // nao e null-deref, passa adiante

    // Loga para debug
    printf("[VEH] Null-deref capturado: RIP=0x%llX fault=0x%llX — aplicando fake-return\n",
           (unsigned long long)ep->ContextRecord->Rip,
           (unsigned long long)faultAddr);

    // Fake-return: pop return address da pilha e salta para ele.
    // Equivale a fazer a funcao crashante retornar 0 ao seu caller.
    __try
    {
        ULONG64 retAddr = *(ULONG64*)(ep->ContextRecord->Rsp);
        ep->ContextRecord->Rip = retAddr;   // salta para o caller
        ep->ContextRecord->Rsp += 8;        // remove o return addr da pilha
        ep->ContextRecord->Rax = 0;         // retorno = 0 / false / nullptr
        ep->ContextRecord->Rcx = 0;         // limpa arg1 por seguranca
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        // Se o proprio fake-return falhar (pilha corrompida),
        // nao fazemos nada — deixamos o processo crashar normalmente.
        printf("[VEH] Pilha corrompida, nao foi possivel aplicar fake-return\n");
        return EXCEPTION_CONTINUE_SEARCH;
    }

    return EXCEPTION_CONTINUE_EXECUTION;
}

// ------------------------------------------------------------
// 1b. Filtro global de excecao nao tratada (ultima linha de
//     defesa, caso o VEH nao tenha capturado).
// ------------------------------------------------------------
static LONG WINAPI NebulaExceptionFilter(struct _EXCEPTION_POINTERS*)
{
    return EXCEPTION_EXECUTE_HANDLER;
}

// Chame uma vez no init do servidor (antes de Listen).
static inline void CrashGuard_Install()
{
    // VEH com prioridade 1 = primeiro da lista, roda antes do UE4
    if (!g_NebulaVehHandle)
        g_NebulaVehHandle = AddVectoredExceptionHandler(1, NebulaVEH);

    // Filtro de ultima chance
    SetUnhandledExceptionFilter(NebulaExceptionFilter);
}

// ------------------------------------------------------------
// 2. Hook do ProcessEvent com SEH
//    ProcessEvent e o ponto central de despacho de todos os
//    eventos Blueprint e nativos do Unreal Engine.
//    Hookeando com SEH, qualquer crash dentro de qualquer
//    evento e capturado sem derrubar o processo.
//
//    Offset para Fortnite 14.60: 0x0376B220
// ------------------------------------------------------------
static void (*ProcessEventOG)(UObject*, UFunction*, void*) = nullptr;

static __declspec(noinline) void ProcessEventHook(UObject* Obj, UFunction* Func, void* Params)
{
    __try
    {
        ProcessEventOG(Obj, Func, Params);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static inline void CrashGuard_HookProcessEvent()
{
    LPVOID Addr = (LPVOID)(ImageBase + 0x0376B220);
    MH_CreateHook(Addr, ProcessEventHook, (LPVOID*)&ProcessEventOG);
    MH_EnableHook(Addr);
}

// ------------------------------------------------------------
// 3. Macros NEBULA_SEH para hooks com objetos C++ no escopo
//    MSVC nao compila __try/__except em funcoes que contenham
//    objetos com destrutor (TArray, std::string, etc).
//    A solucao e mover o corpo para uma funcao __cdecl
//    separada __declspec(noinline) sem objetos no escopo.
//
//    Uso:
//
//    void MinhHookVoid(AGameMode* GM) {
//        NEBULA_SEH_VOID(
//            (AGameMode* gm),   // campos do contexto
//            { GM },            // valores de init
//            {                  // corpo protegido
//                c->gm->DoSomething();
//            }
//        );
//    }
//
//    bool MeuHookBool(AGameMode* GM) {
//        bool result = false;
//        NEBULA_SEH_BOOL(result, false,
//            (AGameMode* gm),
//            { GM },
//            { c->gm->DoSomething(); return true; }
//        );
//        return result;
//    }
// ------------------------------------------------------------

typedef void(__cdecl* NebulaSehVoidFn)(void*);
typedef bool(__cdecl* NebulaSehBoolFn)(void*);
typedef int(__cdecl* NebulaSehIntFn)(void*);
typedef __int64(__cdecl* NebulaSehI64Fn)(void*);

static __declspec(noinline) void NebulaSehCallVoid(NebulaSehVoidFn fn, void* ctx)
{
    __try { fn(ctx); }
    __except (EXCEPTION_EXECUTE_HANDLER) {}
}

static __declspec(noinline) bool NebulaSehCallBool(NebulaSehBoolFn fn, void* ctx, bool fallback)
{
    bool r = fallback;
    __try { r = fn(ctx); }
    __except (EXCEPTION_EXECUTE_HANDLER) { r = fallback; }
    return r;
}

static __declspec(noinline) int NebulaSehCallInt(NebulaSehIntFn fn, void* ctx, int fallback)
{
    int r = fallback;
    __try { r = fn(ctx); }
    __except (EXCEPTION_EXECUTE_HANDLER) { r = fallback; }
    return r;
}

static __declspec(noinline) __int64 NebulaSehCallI64(NebulaSehI64Fn fn, void* ctx, __int64 fallback)
{
    __int64 r = fallback;
    __try { r = fn(ctx); }
    __except (EXCEPTION_EXECUTE_HANDLER) { r = fallback; }
    return r;
}

#define _NEBULA_CAT2(a, b) a##b
#define _NEBULA_CAT(a, b) _NEBULA_CAT2(a, b)
#define _NEBULA_ID(base) _NEBULA_CAT(base, __COUNTER__)

#define NEBULA_SEH_VOID(ctx_fields, ctx_init, body)                                     \
    do {                                                                                \
        struct _NEBULA_ID(_NCtx) { ctx_fields };                                        \
        _NEBULA_ID(_NCtx) _NEBULA_ID(_nctxv) = ctx_init;                               \
        struct _NEBULA_ID(_NThunk) {                                                    \
            static void __cdecl run(void* p) {                                         \
                auto* c = (_NEBULA_ID(_NCtx)*)p; body;                                \
            }                                                                           \
        };                                                                              \
        NebulaSehCallVoid(&_NEBULA_ID(_NThunk)::run, &_NEBULA_ID(_nctxv));             \
    } while (0)

#define NEBULA_SEH_BOOL(outvar, fallback, ctx_fields, ctx_init, body)                  \
    do {                                                                                \
        struct _NEBULA_ID(_NCtx) { ctx_fields };                                        \
        _NEBULA_ID(_NCtx) _NEBULA_ID(_nctxv) = ctx_init;                               \
        struct _NEBULA_ID(_NThunk) {                                                    \
            static bool __cdecl run(void* p) {                                         \
                auto* c = (_NEBULA_ID(_NCtx)*)p; body; return false;                  \
            }                                                                           \
        };                                                                              \
        (outvar) = NebulaSehCallBool(&_NEBULA_ID(_NThunk)::run,                        \
                                     &_NEBULA_ID(_nctxv), (fallback));                 \
    } while (0)

#define NEBULA_SEH_INT(outvar, fallback, ctx_fields, ctx_init, body)                   \
    do {                                                                                \
        struct _NEBULA_ID(_NCtx) { ctx_fields };                                        \
        _NEBULA_ID(_NCtx) _NEBULA_ID(_nctxv) = ctx_init;                               \
        struct _NEBULA_ID(_NThunk) {                                                    \
            static int __cdecl run(void* p) {                                          \
                auto* c = (_NEBULA_ID(_NCtx)*)p; body; return 0;                      \
            }                                                                           \
        };                                                                              \
        (outvar) = NebulaSehCallInt(&_NEBULA_ID(_NThunk)::run,                         \
                                    &_NEBULA_ID(_nctxv), (fallback));                  \
    } while (0)

#define NEBULA_SEH_I64(outvar, fallback, ctx_fields, ctx_init, body)                   \
    do {                                                                                \
        struct _NEBULA_ID(_NCtx) { ctx_fields };                                        \
        _NEBULA_ID(_NCtx) _NEBULA_ID(_nctxv) = ctx_init;                               \
        struct _NEBULA_ID(_NThunk) {                                                    \
            static __int64 __cdecl run(void* p) {                                      \
                auto* c = (_NEBULA_ID(_NCtx)*)p; body; return 0;                      \
            }                                                                           \
        };                                                                              \
        (outvar) = NebulaSehCallI64(&_NEBULA_ID(_NThunk)::run,                         \
                                    &_NEBULA_ID(_nctxv), (__int64)(fallback));         \
    } while (0)
