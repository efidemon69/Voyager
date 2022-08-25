#pragma once
#include "HvLoader.h"
#include "PayLoad.h"

extern INLINE_HOOK WinLoadImageShitHook;
extern INLINE_HOOK WinLoadAllocateImageHook;

// 2004-1511 winload.BlImgAllocateImageBuffer
#define ALLOCATE_IMAGE_BUFFER_SIG "\xE8\x00\x00\x00\x00\x8B\xD8\x85\xC0\x78\x7C\x21\x7C\x24\x00\x45\x33\xC0"
#define ALLOCATE_IMAGE_BUFFER_MASK "x????xxxxxxxxx?xxx"

// 1703-1511
//
// for 1703-1511, we are going to want to hook BlImgAllocateImageBuffer inside of hvloader.efi
// not winload. We will have to scan for BlImgLoadPEImageEx in winload and then wait for hvloader
// to be loaded to install hooks in hvloader...
#define LOAD_PE_IMG_SIG "\x48\x89\x44\x24\x00\xE8\x00\x00\x00\x00\x44\x8B\xF0\x85\xC0\x79\x11"
#define LOAD_PE_IMG_MASK "xxxx?x????xxxxxxx"

static_assert(sizeof(ALLOCATE_IMAGE_BUFFER_SIG) == sizeof(ALLOCATE_IMAGE_BUFFER_MASK), "signature and mask do not match size!");

typedef UINT64 (EFIAPI* ALLOCATE_IMAGE_BUFFER)(VOID** imageBuffer, UINTN imageSize, UINT32 memoryType, 
	UINT32 attributes, VOID* unused, UINT32 Value);

typedef EFI_STATUS(EFIAPI* LDR_LOAD_IMAGE)(VOID* a1, VOID* a2, CHAR16* ImagePath, UINT64* ImageBasePtr, UINT32* ImageSize,
	VOID* a6, VOID* a7, VOID* a8, VOID* a9, VOID* a10, VOID* a11, VOID* a12, VOID* a13, VOID* a14);

/// <summary>
/// for 1703-1507, we are going to want to install hooks inside 
/// of hvloader.efi... in order to know when hvloader.efi is loaded into memory
/// we are going to install a hook on winload.BlImgLoadPEImageEx...
/// </summary>
/// <param name="a1">unknown</param>
/// <param name="a2">unknown</param>
/// <param name="ImagePath">
/// unicode string path to image being loaded into memory...
/// </param>
/// <param name="ImageBasePtr">
/// pointer to a void pointer which will contain the base 
/// address of the module after its loaded...
/// </param>
/// <param name="ImageSize">pass by ref size of the image loaded into memory...</param>
/// <param name="a6">unknown</param>
/// <param name="a7">unknown</param>
/// <param name="a8">unknown</param>
/// <param name="a9">unknown</param>
/// <param name="a10">unknown</param>
/// <param name="a11">unknown</param>
/// <param name="a12">unknown</param>
/// <param name="a13">unknown</param>
/// <param name="a14">unknown</param>
/// <returns>status of image loaded...</returns>
EFI_STATUS EFIAPI BlImgLoadPEImageEx
(
	VOID* a1,
	VOID* a2,
	CHAR16* ImagePath, 
	UINT64* ImageBasePtr,
	UINT32* ImageSize,
	VOID* a6,
	VOID* a7,
	VOID* a8,
	VOID* a9, 
	VOID* a10, 
	VOID* a11,
	VOID* a12,
	VOID* a13, 
	VOID* a14
);

/// <summary>
/// for 2004-1709, winload exports a bunch of functions... specifically BlLdrLoadImage,
/// which hvloader calls to load hyper-v into memory... BlLdrLoadImage calls BlImgAllocateImageBuffer
/// to allocate memory for hyper-v's module, we are hooking BlImgAllocateImageBuffer to extend
/// the allocations size and to make the entire allocation RWX...
/// </summary>
/// <param name="imageBuffer">pass by ref of a pointer to the allocation base...</param>
/// <param name="imageSize">size of the allocation...</param>
/// <param name="memoryType"></param>
/// <param name="attributes"></param>
/// <param name="unused"></param>
/// <param name="flags"></param>
/// <returns></returns>
EFI_STATUS EFIAPI BlImgAllocateImageBuffer
(
	VOID** imageBuffer,
	UINTN imageSize,
	UINT32 memoryType,
	UINT32 attributes, 
	VOID* unused,
	UINT32 Value
);

/// <summary>
/// 2004-1709, BlLdrLoadImage is exported from winload... I shithook this and
/// when hyper-v is loaded I install my hooks/extend hyper-v's allocation...
/// </summary>
/// <param name="Arg1"></param>
/// <param name="ModulePath"></param>
/// <param name="ModuleName"></param>
/// <param name="Arg4"></param>
/// <param name="Arg5"></param>
/// <param name="Arg6"></param>
/// <param name="Arg7"></param>
/// <param name="lplpTableEntry"></param>
/// <param name="Arg9"></param>
/// <param name="Arg10"></param>
/// <param name="Arg11"></param>
/// <param name="Arg12"></param>
/// <param name="Arg13"></param>
/// <param name="Arg14"></param>
/// <param name="Arg15"></param>
/// <param name="Arg16"></param>
/// <returns></returns>
EFI_STATUS EFIAPI BlLdrLoadImage
(
	VOID* Arg1, 
	CHAR16* ModulePath,
	CHAR16* ModuleName,
	VOID* Arg4,
	VOID* Arg5, 
	VOID* Arg6,
	VOID* Arg7,
	PPLDR_DATA_TABLE_ENTRY lplpTableEntry,
	VOID* Arg9,
	VOID* Arg10, 
	VOID* Arg11,
	VOID* Arg12,
	VOID* Arg13,
	VOID* Arg14,
	VOID* Arg15, 
	VOID* Arg16
);