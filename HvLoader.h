#pragma once
#include "PayLoad.h"
#include "Hv.h"
#include "InlineHook.h"
#include "PagingTables.h"

extern INLINE_HOOK HvLoadImageHook;
extern INLINE_HOOK HvLoadAllocImageHook;
extern INLINE_HOOK HvLoadImageBufferHook;
extern INLINE_HOOK TransferControlShitHook;

#define HV_ALLOCATE_IMAGE_BUFFER_SIG "\xE8\x00\x00\x00\x00\x8B\xF8\x85\xC0\x79\x0A"
#define HV_ALLOCATE_IMAGE_BUFFER_MASK "x????xxxxxx"
static_assert(sizeof(HV_ALLOCATE_IMAGE_BUFFER_SIG) == sizeof(HV_ALLOCATE_IMAGE_BUFFER_MASK), "signature and mask do not match size!");

#define HV_LOAD_PE_IMG_FROM_BUFFER_SIG "\xE8\x00\x00\x00\x00\x44\x8B\xAD"
#define HV_LOAD_PE_IMG_FROM_BUFFER_MASK "x????xxx"
static_assert(sizeof(HV_LOAD_PE_IMG_FROM_BUFFER_SIG) == sizeof(HV_LOAD_PE_IMG_FROM_BUFFER_MASK), "signature and mask do not match size!");

#define HV_LOAD_PE_IMG_SIG "\x48\x89\x44\x24\x00\xE8\x00\x00\x00\x00\x44\x8B\xF0\x85\xC0\x0F\x88\x00\x00\x00\x00\x4C\x8D\x45"
#define HV_LOAD_PE_IMG_MASK "xxxx?x????xxxxxxx????xxx"
static_assert(sizeof(HV_LOAD_PE_IMG_SIG) == sizeof(HV_LOAD_PE_IMG_MASK), "signature and mask do not match size...");

typedef EFI_STATUS(EFIAPI* ALLOCATE_IMAGE_BUFFER)(VOID** imageBuffer, UINTN imageSize, UINT32 memoryType, 
	UINT32 attributes, VOID* unused, UINT32 Value);

typedef EFI_STATUS(EFIAPI* HV_LDR_LOAD_IMAGE_BUFFER)(VOID* a1, VOID* a2, VOID* a3, VOID* a4, UINT64* ImageBase,
	UINT32* ImageSize, VOID* a7, VOID* a8, VOID* a9, VOID* a10, VOID* a11, VOID* a12, VOID* a13, VOID* a14, VOID* a15);

typedef EFI_STATUS(EFIAPI* HV_LDR_LOAD_IMAGE)(VOID* DeviceId, VOID* MemoryType, CHAR16* Path, VOID** ImageBase, UINT32* ImageSize,
	VOID* Hash, VOID* Flags, VOID* a8, VOID* a9, VOID* a10, VOID* a11, VOID* a12, VOID* a13);

/// <summary>
/// hook on hvloader.BlImgAllocateImageBuffer, hooked to extend the 
/// allocation size....
/// </summary>
/// <param name="imageBuffer">pass by ref pointer to allocations base...</param>
/// <param name="imageSize">size of allocation...</param>
/// <param name="memoryType"></param>
/// <param name="attributes"></param>
/// <param name="unused"></param>
/// <param name="flags"></param>
/// <returns></returns>
UINT64 EFIAPI HvBlImgAllocateImageBuffer
(
	VOID** imageBuffer,
	UINTN imageSize, 
	UINT32 memoryType,
	UINT32 attributes, 
	VOID* unused,
	UINT32 Value
);

/// <summary>
/// hook on hvloader.BlImgLoadPEImageEx... hyper-v is loaded into memory
/// by this function so im hooking it to extend it/inject into it.
/// </summary>
/// <param name="DeviceId"></param>
/// <param name="MemoryType"></param>
/// <param name="Path"></param>
/// <param name="ImageBase"></param>
/// <param name="ImageSize"></param>
/// <param name="Hash"></param>
/// <param name="Flags"></param>
/// <param name="a8"></param>
/// <param name="a9"></param>
/// <param name="a10"></param>
/// <param name="a11"></param>
/// <param name="a12"></param>
/// <param name="a13"></param>
/// <returns></returns>
EFI_STATUS EFIAPI HvBlImgLoadPEImageEx
(
	VOID* DeviceId, 
	VOID* MemoryType,
	CHAR16* Path,
	UINT64* ImageBase,
	UINT32* ImageSize,
	VOID* Hash,
	VOID* Flags,
	VOID* a8,
	VOID* a9,
	VOID* a10,
	VOID* a11,
	VOID* a12,
	VOID* a13
);

/// <summary>
/// for some reason 1703 hvloader.BlImgLoadPEImageFromSourceBuffer is called
/// to load hyper-v into memory... Hooking this like i hook hvloader.BlImgLoadPEImageEx...
/// </summary>
/// <param name="a1"></param>
/// <param name="a2"></param>
/// <param name="a3"></param>
/// <param name="a4"></param>
/// <param name="ImageBase"></param>
/// <param name="ImageSize"></param>
/// <param name="a7"></param>
/// <param name="a8"></param>
/// <param name="a9"></param>
/// <param name="a10"></param>
/// <param name="a11"></param>
/// <param name="a12"></param>
/// <param name="a13"></param>
/// <param name="a14"></param>
/// <param name="a15"></param>
/// <returns></returns>
EFI_STATUS EFIAPI HvBlImgLoadPEImageFromSourceBuffer
(
	VOID* a1,
	VOID* a2,
	VOID* a3, 
	VOID* a4,
	UINT64* ImageBase,
	UINT32* ImageSize,
	VOID* a7,
	VOID* a8,
	VOID* a9, 
	VOID* a10,
	VOID* a11,
	VOID* a12, 
	VOID* a13, 
	VOID* a14,
	VOID* a15
);