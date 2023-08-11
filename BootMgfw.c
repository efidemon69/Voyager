#include "BootMgfw.h"
#include "SplashScreen.h"

INLINE_HOOK BootMgfwShitHook;
EFI_STATUS EFIAPI RestoreBootMgfw(VOID)
{
	UINTN HandleCount = NULL;
	EFI_STATUS Result;
	EFI_HANDLE* Handles = NULL;
	EFI_FILE_HANDLE VolumeHandle;
	EFI_FILE_HANDLE BootMgfwHandle;
	EFI_FILE_IO_INTERFACE* FileSystem = NULL;

	if (EFI_ERROR((Result = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles))))
	{
		Print(L"error getting file system handles -> 0x%p\n", Result);
		return Result;
	}

	for (UINT32 Idx = 0u; Idx < HandleCount; ++Idx)
	{
		if (EFI_ERROR((Result = gBS->OpenProtocol(Handles[Idx], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FileSystem, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL))))
		{
			Print(L"error opening protocol -> 0x%p\n", Result);
			return Result;
		}

		if (EFI_ERROR((Result = FileSystem->OpenVolume(FileSystem, &VolumeHandle))))
		{
			Print(L"error opening file system -> 0x%p\n", Result);
			return Result;
		}

		if (!EFI_ERROR((Result = VolumeHandle->Open(VolumeHandle, &BootMgfwHandle, WINDOWS_BOOTMGFW_PATH, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY))))
		{
			VolumeHandle->Close(VolumeHandle);
			EFI_FILE_PROTOCOL* BootMgfwFile = NULL;
			EFI_DEVICE_PATH* BootMgfwPathProtocol = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_PATH);

			// open bootmgfw as read/write then delete it...
			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&BootMgfwPathProtocol, &BootMgfwFile, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, NULL))))
			{
				Print(L"error opening bootmgfw... reason -> %r\n", Result);
				return Result;
			}

			if (EFI_ERROR((Result = BootMgfwFile->Delete(BootMgfwFile))))
			{
				Print(L"error deleting bootmgfw... reason -> %r\n", Result);
				return Result;
			}

			// open bootmgfw.efi.backup
			BootMgfwPathProtocol = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_BACKUP_PATH);
			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&BootMgfwPathProtocol, &BootMgfwFile, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, NULL))))
			{
				Print(L"failed to open backup file... reason -> %r\n", Result);
				return Result;
			}

			EFI_FILE_INFO* FileInfoPtr = NULL;
			UINTN FileInfoSize = NULL;

			// get the size of bootmgfw.efi.backup...
			if (EFI_ERROR((Result = BootMgfwFile->GetInfo(BootMgfwFile, &gEfiFileInfoGuid, &FileInfoSize, NULL))))
			{
				if (Result == EFI_BUFFER_TOO_SMALL)
				{
					gBS->AllocatePool(EfiBootServicesData, FileInfoSize, &FileInfoPtr);
					if (EFI_ERROR(Result = BootMgfwFile->GetInfo(BootMgfwFile, &gEfiFileInfoGuid, &FileInfoSize, FileInfoPtr)))
					{
						Print(L"get backup file information failed... reason -> %r\n", Result);
						return Result;
					}
				}
				else
				{
					Print(L"Failed to get file information... reason -> %r\n", Result);
					return Result;
				}
			}

			VOID* BootMgfwBuffer = NULL;
			UINTN BootMgfwSize = FileInfoPtr->FileSize;
			gBS->AllocatePool(EfiBootServicesData, FileInfoPtr->FileSize, &BootMgfwBuffer);

			// read the backup file into an allocated pool...
			if (EFI_ERROR((Result = BootMgfwFile->Read(BootMgfwFile, &BootMgfwSize, BootMgfwBuffer))))
			{
				Print(L"Failed to read backup file into buffer... reason -> %r\n", Result);
				return Result;
			}

			// delete the backup file...
			if (EFI_ERROR((Result = BootMgfwFile->Delete(BootMgfwFile))))
			{
				Print(L"unable to delete backup file... reason -> %r\n", Result);
				return Result;
			}

			// create a new bootmgfw file...
			BootMgfwPathProtocol = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_PATH);
			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&BootMgfwPathProtocol, &BootMgfwFile, EFI_FILE_MODE_CREATE | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, EFI_FILE_SYSTEM))))
			{
				Print(L"unable to create new bootmgfw on disk... reason -> %r\n", Result);
				return Result;
			}

			// write the data from the backup file to the new bootmgfw file...
			BootMgfwSize = FileInfoPtr->FileSize;
			if (EFI_ERROR((Result = BootMgfwFile->Write(BootMgfwFile, &BootMgfwSize, BootMgfwBuffer))))
			{
				Print(L"unable to write to newly created bootmgfw.efi... reason -> %r\n", Result);
				return Result;
			}

			BootMgfwFile->Close(BootMgfwFile);
			gBS->FreePool(FileInfoPtr);
			gBS->FreePool(BootMgfwBuffer);
			return EFI_SUCCESS;
		}

		if (EFI_ERROR((Result = gBS->CloseProtocol(Handles[Idx], &gEfiSimpleFileSystemProtocolGuid, gImageHandle, NULL))))
		{
			Print(L"error closing protocol -> 0x%p\n", Result);
			return Result;
		}
	}

	gBS->FreePool(Handles);
	return EFI_ABORTED;
}

EFI_STATUS EFIAPI GetBootMgfwPath(EFI_DEVICE_PATH** BootMgfwDevicePath)
{
	UINTN HandleCount = NULL;
	EFI_STATUS Result;
	EFI_HANDLE* Handles = NULL;
	EFI_FILE_HANDLE VolumeHandle;
	EFI_FILE_HANDLE BootMgfwHandle;
	EFI_FILE_IO_INTERFACE* FileSystem = NULL;

	if (EFI_ERROR((Result = gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles))))
	{
		Print(L"error getting file system handles -> 0x%p\n", Result);
		return Result;
	}

	for (UINT32 Idx = 0u; Idx < HandleCount; ++Idx)
	{
		if (EFI_ERROR((Result = gBS->OpenProtocol(Handles[Idx], &gEfiSimpleFileSystemProtocolGuid, (VOID**)&FileSystem, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL))))
		{
			Print(L"error opening protocol -> 0x%p\n", Result);
			return Result;
		}

		if (EFI_ERROR((Result = FileSystem->OpenVolume(FileSystem, &VolumeHandle))))
		{
			Print(L"error opening file system -> 0x%p\n", Result);
			return Result;
		}

		if (!EFI_ERROR(VolumeHandle->Open(VolumeHandle, &BootMgfwHandle, WINDOWS_BOOTMGFW_PATH, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY)))
		{
			VolumeHandle->Close(BootMgfwHandle);
			*BootMgfwDevicePath = FileDevicePath(Handles[Idx], WINDOWS_BOOTMGFW_PATH);
			return EFI_SUCCESS;
		}

		if (EFI_ERROR((Result = gBS->CloseProtocol(Handles[Idx], &gEfiSimpleFileSystemProtocolGuid, gImageHandle, NULL))))
		{
			Print(L"error closing protocol -> 0x%p\n", Result);
			return Result;
		}
	}
	return EFI_NOT_FOUND;
}

EFI_STATUS EFIAPI InstallBootMgfwHooks(EFI_HANDLE ImageHandle)
{
	EFI_STATUS Result = EFI_SUCCESS;
	EFI_LOADED_IMAGE* BootMgfw = NULL;
	
	if (EFI_ERROR(Result = gBS->HandleProtocol(ImageHandle, &gEfiLoadedImageProtocolGuid, (VOID**)&BootMgfw)))
		return Result;

	Print(L"BootMgfw Image Base -> 0x%p\n", BootMgfw->ImageBase);
	Print(L"BootMgfw Image Size -> 0x%x\n", BootMgfw->ImageSize);

	VOID* ArchStartBootApplication =
		FindPattern(
			BootMgfw->ImageBase,
			BootMgfw->ImageSize,
			START_BOOT_APPLICATION_SIG,
			START_BOOT_APPLICATION_MASK
		);

	if (!ArchStartBootApplication)
		return EFI_NOT_FOUND;

#if WINVER >= 1703
	Print(L"BootMgfw.BlImgStartBootApplication -> 0x%p\n", ArchStartBootApplication);
	MakeInlineHook(&BootMgfwShitHook, ArchStartBootApplication, &ArchStartBootApplicationHook, TRUE);
#else
	Print(L"BootMgfw.BlImgStartBootApplication -> 0x%p\n", RESOLVE_RVA(ArchStartBootApplication, 5, 1));
	MakeInlineHook(&BootMgfwShitHook, RESOLVE_RVA(ArchStartBootApplication, 5, 1), &ArchStartBootApplicationHook, TRUE);
#endif
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI ArchStartBootApplicationHook(VOID* AppEntry, VOID* ImageBase, UINT32 ImageSize, UINT8 BootOption, VOID* ReturnArgs)
{
	// disable ArchStartBootApplication shithook
	DisableInlineHook(&BootMgfwShitHook);

	// on 1703 and below, winload does not export any functions
	if (!GetExport(ImageBase, "BlLdrLoadImage"))
	{
		VOID* ImgLoadPEImageEx =
			FindPattern(
				ImageBase,
				ImageSize,
				LOAD_PE_IMG_SIG,
				LOAD_PE_IMG_MASK
			);

		gST->ConOut->ClearScreen(gST->ConOut);
		gST->ConOut->OutputString(gST->ConOut, AsciiArt);
		Print(L"\n");

		Print(L"Hyper-V PayLoad Size -> 0x%x\n", PayLoadSize());
		Print(L"winload.BlImgLoadPEImageEx -> 0x%p\n", RESOLVE_RVA(ImgLoadPEImageEx, 10, 6));
		MakeInlineHook(&WinLoadImageShitHook, RESOLVE_RVA(ImgLoadPEImageEx, 10, 6), &BlImgLoadPEImageEx, TRUE);
	}
	else // else the installed windows version is between 2004 and 1709
	{
		VOID* LdrLoadImage = GetExport(ImageBase, "BlLdrLoadImage");
		VOID* ImgAllocateImageBuffer =
			FindPattern(
				ImageBase,
				ImageSize,
				ALLOCATE_IMAGE_BUFFER_SIG,
				ALLOCATE_IMAGE_BUFFER_MASK
			);

		gST->ConOut->ClearScreen(gST->ConOut);
		gST->ConOut->OutputString(gST->ConOut, AsciiArt);
		Print(L"\n");

		Print(L"Hyper-V PayLoad Size -> 0x%x\n", PayLoadSize());
		Print(L"winload.BlLdrLoadImage -> 0x%p\n", LdrLoadImage);
		Print(L"winload.BlImgAllocateImageBuffer -> 0x%p\n", RESOLVE_RVA(ImgAllocateImageBuffer, 5, 1));

		MakeInlineHook(&WinLoadImageShitHook, LdrLoadImage, &BlLdrLoadImage, TRUE);
		MakeInlineHook(&WinLoadAllocateImageHook, RESOLVE_RVA(ImgAllocateImageBuffer, 5, 1), &BlImgAllocateImageBuffer, TRUE);
	}
	return ((IMG_ARCH_START_BOOT_APPLICATION)BootMgfwShitHook.Address)(AppEntry, ImageBase, ImageSize, BootOption, ReturnArgs);
}
