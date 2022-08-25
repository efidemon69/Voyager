#pragma once
#include <Uefi.h>
#include <Library/UefiLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Protocol/LoadedImage.h>
#include <IndustryStandard/PeImage.h>
#include <Guid/GlobalVariable.h>

#define PORT_NUM 0x2F8
#define BL_MEMORY_ATTRIBUTE_RWX 0x424000
#define SEC_TO_MS(seconds) seconds * 1000000
#define SECTION_RWX (EFI_IMAGE_SCN_MEM_READ | EFI_IMAGE_SCN_MEM_WRITE | EFI_IMAGE_SCN_MEM_EXECUTE)

void __outdword(unsigned short, unsigned long);
VOID __outbytestring(UINT16 Port, UINT8* Buffer, UINT32 Count);
void __outbyte(unsigned short Port, unsigned char Data);
#pragma intrinsic(__outbytestring)
#pragma intrinsic(__outbyte)

static CHAR8 dbg_buffer[0x100];
#define DBG_PRINT(...) \
	AsciiSPrint(dbg_buffer, sizeof dbg_buffer, __VA_ARGS__); \
	__outbytestring(PORT_NUM, dbg_buffer, AsciiStrLen(dbg_buffer))

#define RESOLVE_RVA(SIG_RESULT, RIP_OFFSET, RVA_OFFSET) \
	(*(INT32*)(((UINT64)SIG_RESULT) + RVA_OFFSET)) + ((UINT64)SIG_RESULT) + RIP_OFFSET

typedef struct _LDR_DATA_TABLE_ENTRY
{
	LIST_ENTRY InLoadOrderLinks;	// 16
	LIST_ENTRY InMemoryOrderLinks;	// 32
	LIST_ENTRY InInitializationOrderLinks; // 48
	UINT64 ModuleBase; // 56
	UINT64 EntryPoint; // 64
	UINTN SizeOfImage; // 72
} LDR_DATA_TABLE_ENTRY, *PLDR_DATA_TABLE_ENTRY, **PPLDR_DATA_TABLE_ENTRY;

// taken from umap (btbd)
BOOLEAN CheckMask(CHAR8* base, CHAR8* pattern, CHAR8* mask);
VOID* FindPattern(CHAR8* base, UINTN size, CHAR8* pattern, CHAR8* mask);
VOID* GetExport(UINT8* base, CHAR8* export);
VOID MemCopy(VOID* dest, VOID* src, UINTN size);