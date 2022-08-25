#pragma once
#include "PayLoad.h"

extern PVOYAGER_T PayLoadDataPtr;
#if WINVER == 2004
#define INTEL_VMEXIT_HANDLER_SIG "\x65\xC6\x04\x25\x6D\x00\x00\x00\x00\x48\x8B\x4C\x24\x00\x48\x8B\x54\x24\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xxxxxxxxxxxxx?xxxx?x????x"
#elif WINVER == 1909
#define INTEL_VMEXIT_HANDLER_SIG "\x48\x8B\x4C\x24\x00\xEB\x07\xE8\x00\x00\x00\x00\xEB\xF2\x48\x8B\x54\x24\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xxxx?xxx????xxxxxx?x????x"
#elif WINVER == 1903
#define INTEL_VMEXIT_HANDLER_SIG "\x48\x8B\x4C\x24\x00\xEB\x07\xE8\x00\x00\x00\x00\xEB\xF2\x48\x8B\x54\x24\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xxxx?xxx????xxxxxx?x????x"
#elif WINVER == 1809
#define INTEL_VMEXIT_HANDLER_SIG "\x48\x8B\x4C\x24\x00\xEB\x07\xE8\x00\x00\x00\x00\xEB\xF2\x48\x8B\x54\x24\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xxxx?xxx????xxxxxx?x????x"
#elif WINVER == 1803
#define INTEL_VMEXIT_HANDLER_SIG "\xF2\x80\x3D\xFC\x12\x46\x00\x00\x0F\x84\x00\x00\x00\x00\x48\x8B\x54\x24\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xxxxxxx?xx????xxxx?x????x"
#elif WINVER <= 1709 && WINVER != 1507
#define INTEL_VMEXIT_HANDLER_SIG "\xD0\x80\x00\x00\x00\x00\x00\x00\x0F\x84\x00\x00\x00\x00\x48\x8B\x54\x24\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xx????x?xx????xxxx?x????x"
#elif WINVER == 1507
#define INTEL_VMEXIT_HANDLER_SIG "\x60\xC0\x0F\x29\x68\xD0\x80\x3D\x7E\xAF\x49\x00\x01\x0F\x84\x00\x00\x00\x00\xE8\x00\x00\x00\x00\xE9"
#define INTEL_VMEXIT_HANDLER_MASK "xxxxxxxxxxxxxxx????x????x"
#endif

#define AMD_VMEXIT_HANDLER_SIG "\xE8\x00\x00\x00\x00\x48\x89\x04\x24\xE9"
#define AMD_VMEXIT_HANDLER_MASK "x????xxxxx"

static_assert(sizeof(AMD_VMEXIT_HANDLER_SIG) == sizeof(AMD_VMEXIT_HANDLER_MASK), "signature does not match mask size!");

#define HV_ALLOC_SIZE 0x1400000
static_assert(sizeof(INTEL_VMEXIT_HANDLER_SIG) == sizeof(INTEL_VMEXIT_HANDLER_MASK), "signature does not match mask size!");
static_assert(sizeof(INTEL_VMEXIT_HANDLER_SIG) == 26, "signature is invalid length!");

/// <summary>
/// manually map module into hyper-v's extended relocation section...
/// </summary>
/// <param name="VoyagerData">all the data needed to map the module...</param>
/// <param name="ImageBase">base address of the payload...</param>
/// <returns></returns>
VOID* MapModule(PVOYAGER_T VoyagerData, UINT8* ImageBase);

/// <summary>
/// hook vmexit handler...
/// </summary>
/// <param name="HypervBase">base address of hyper-v</param>
/// <param name="HypervSize">hyper-v size (SizeOfImage in memory)</param>
/// <param name="VmExitHook">vmexit hook function address (where to jump too)</param>
/// <returns></returns>
VOID* HookVmExit(VOID* HypervBase, VOID* HypervSize, VOID* VmExitHook);

/// <summary>
/// populates a VOYAGER_T structure passed by reference...
/// </summary>
/// <param name="VoyagerData">pass by ref VOYAGER_T...</param>
/// <param name="HypervAlloc">hyper-v module base...</param>
/// <param name="HypervAllocSize">hyper-v module size...</param>
/// <param name="PayLoadBase">payload base address...</param>
/// <param name="PayLoadSize">payload module size...</param>
VOID MakeVoyagerData
(
	PVOYAGER_T VoyagerData,
	VOID* HypervAlloc,
	UINT64 HypervAllocSize,
	VOID* PayLoadBase,
	UINT64 PayLoadSize
);