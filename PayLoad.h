#pragma once
#include "Utils.h"
#include "PagingTables.h"
#include <Library/ShellLib.h>

#define NT_HEADER(x) ((EFI_IMAGE_NT_HEADERS64*)(((UINT64)(x)) + ((EFI_IMAGE_DOS_HEADER*)(x))->e_lfanew))

#define SECTION_RWX ((EFI_IMAGE_SCN_MEM_WRITE | \
	EFI_IMAGE_SCN_CNT_CODE | \
	EFI_IMAGE_SCN_CNT_UNINITIALIZED_DATA | \
	EFI_IMAGE_SCN_MEM_EXECUTE | \
	EFI_IMAGE_SCN_CNT_INITIALIZED_DATA | \
	EFI_IMAGE_SCN_MEM_READ))

// Source: https://blogs.oracle.com/jwadams/macros-and-powers-of-two
// align x down to the nearest multiple of align. align must be a power of 2.
#define P2ALIGNDOWN(x, align) ((x) & -(align))
// align x up to the nearest multiple of align. align must be a power of 2.
#define P2ALIGNUP(x, align) (-(-(x) & -(align)))

extern VOID* PayLoad;
#pragma pack(push, 1)
typedef struct _VOYAGER_T
{
	UINT64 VmExitHandlerRva;
	UINT64 HypervModuleBase;
	UINT64 HypervModuleSize;
	UINT64 ModuleBase;
	UINT64 ModuleSize;
} VOYAGER_T, *PVOYAGER_T;
#pragma pack(pop)

#define WINDOWS_BOOTMGFW_PATH L"\\efi\\microsoft\\boot\\bootmgfw.efi"
#define PAYLOAD_PATH L"\\efi\\microsoft\\boot\\payload.dll"

UINT32 PayLoadSize(VOID);
VOID* PayLoadEntry(VOID* ModuleBase);
EFI_STATUS LoadPayLoadFromDisk(VOID** PayLoadBufferPtr);
VOID* AddSection(VOID* ImageBase, CHAR8* SectionName, UINT32 VirtualSize, UINT32 Characteristics);