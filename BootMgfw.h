#pragma once
#include "WinLoad.h"

#if WINVER > 1709
#define START_BOOT_APPLICATION_SIG "\x48\x8B\xC4\x48\x89\x58\x20\x44\x89\x40\x18\x48\x89\x50\x10\x48\x89\x48\x08\x55\x56\x57\x41\x54"
#define START_BOOT_APPLICATION_MASK "xxxxxxxxxxxxxxxxxxxxxxxx"
#elif WINVER == 1709
#define START_BOOT_APPLICATION_SIG "\x48\x8B\xC4\x48\x89\x58\x00\x44\x89\x40\x00\x48\x89\x50\x00\x48\x89\x48\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x68"
#define START_BOOT_APPLICATION_MASK "xxxxxx?xxx?xxx?xxx?xxxxxxxxxxxxxx"
#elif WINVER == 1703
#define START_BOOT_APPLICATION_SIG "\x48\x8B\xC4\x48\x89\x58\x00\x44\x89\x48\x00\x44\x89\x40\x00\x48\x89\x48\x00\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8D\x68\xA9"
#define START_BOOT_APPLICATION_MASK "xxxxxx?xxx?xxx?xxx?xxxxxxxxxxxxxxx"
#elif WINVER < 1703
#define START_BOOT_APPLICATION_SIG "\xE8\x00\x00\x00\x00\x48\x8B\xCE\x8B\xD8\xE8\x00\x00\x00\x00\x41\x8B\xCF"
#define START_BOOT_APPLICATION_MASK "x????xxxxxx????xxx"
#endif
static_assert(sizeof(START_BOOT_APPLICATION_SIG) == sizeof(START_BOOT_APPLICATION_MASK), "signature and mask size's dont match...");

#define WINDOWS_BOOTMGFW_PATH L"\\efi\\microsoft\\boot\\bootmgfw.efi"
#define PAYLOAD_PATH L"\\efi\\microsoft\\boot\\payload.dll"
#define WINDOWS_BOOTMGFW_BACKUP_PATH L"\\efi\\microsoft\\boot\\bootmgfw.efi.backup"

extern INLINE_HOOK BootMgfwShitHook;
typedef EFI_STATUS(EFIAPI* IMG_ARCH_START_BOOT_APPLICATION)(VOID*, VOID*, UINT32, UINT8, VOID*);

/// <summary>
/// restores bootmgfw on disk to its original...
/// </summary>
/// <param name=""></param>
/// <returns>status of completion</returns>
EFI_STATUS EFIAPI RestoreBootMgfw(VOID);

/// <summary>
/// gets bootmgfw device path...
/// </summary>
/// <param name="BootMgfwDevicePath">pointer to a device path pointer...</param>
/// <returns>status of completion...</returns>
EFI_STATUS EFIAPI GetBootMgfwPath(EFI_DEVICE_PATH** BootMgfwDevicePath);

/// <summary>
/// Called to install hooks on bootmgfw... specifically ArchStartBootApplication which
/// is called to load winload...
/// </summary>
/// <param name="ImageHandle">Handle to bootmgfw...</param>
/// <returns>efi success if hooks where installed...</returns>
EFI_STATUS EFIAPI InstallBootMgfwHooks(EFI_HANDLE BootMgfwPath);

/// <summary>
/// This function is called when winload is loaded into memory... all hooks related to winload will be put here...
/// </summary>
/// <param name="AppEntry">Unknown</param>
/// <param name="ImageBase">Base Address of winload...</param>
/// <param name="ImageSize">Size of winload (in memory not on disk)</param>
/// <param name="BootOption">Unknown</param>
/// <param name="ReturnArgs">Unknown</param>
/// <returns>original function's return value...</returns>
EFI_STATUS EFIAPI ArchStartBootApplicationHook(VOID* AppEntry, VOID* ImageBase, UINT32 ImageSize, UINT8 BootOption, VOID* ReturnArgs);