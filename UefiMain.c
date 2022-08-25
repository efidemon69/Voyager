#include "BootMgfw.h"
#include "SplashScreen.h"

CHAR8* gEfiCallerBaseName = "Voyager";
const UINT32 _gUefiDriverRevision = 0x200;

EFI_STATUS EFIAPI UefiUnload(EFI_HANDLE ImageHandle)
{
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI UefiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable)
{
    EFI_STATUS Result;
    EFI_HANDLE BootMgfwHandle;
    EFI_DEVICE_PATH* BootMgfwPath = NULL;

    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->OutputString(gST->ConOut, AsciiArt);
    Print(L"\n");

    // since we replaced bootmgfw on disk, we are going to need to restore the image back
    // this is simply just moving bootmgfw.efi.backup to bootmgfw.efi...
    if (EFI_ERROR((Result = RestoreBootMgfw())))
    {
        Print(L"unable to restore bootmgfw... reason -> %r\n", Result);
        gBS->Stall(SEC_TO_MS(5));
        return Result;
    }

    // the payload is sitting on disk... we are going to load it into memory then delete it...
    if (EFI_ERROR((Result = LoadPayLoadFromDisk(&PayLoad))))
    {
        Print(L"failed to read payload from disk... reason -> %r\n", Result);
        gBS->Stall(SEC_TO_MS(5));
        return Result;
    }

    // get the device path to bootmgfw...
    if (EFI_ERROR((Result = GetBootMgfwPath(&BootMgfwPath))))
    {
        Print(L"getting bootmgfw device path failed... reason -> %r\n", Result);
        gBS->Stall(SEC_TO_MS(5));
        return Result;
    }

    // load bootmgfw into memory...
    if (EFI_ERROR((Result = gBS->LoadImage(TRUE, ImageHandle, BootMgfwPath, NULL, NULL, &BootMgfwHandle))))
    {
        Print(L"failed to load bootmgfw.efi... reason -> %r\n", Result);
        gBS->Stall(SEC_TO_MS(5));
        return EFI_ABORTED;
    }

    // install hooks on bootmgfw...
    if (EFI_ERROR((Result = InstallBootMgfwHooks(BootMgfwHandle))))
    {
        Print(L"Failed to install bootmgfw hooks... reason -> %r\n", Result);
        gBS->Stall(SEC_TO_MS(5));
        return Result;
    }

    // wait 5 seconds then call the entry point of bootmgfw...
    gBS->Stall(SEC_TO_MS(5));
    if (EFI_ERROR((Result = gBS->StartImage(BootMgfwHandle, NULL, NULL))))
    {
        Print(L"Failed to start bootmgfw.efi... reason -> %r\n", Result);
        gBS->Stall(SEC_TO_MS(5));
        return EFI_ABORTED;
    }
    return EFI_SUCCESS;
}