#pragma once
#include "Utils.h"

#define MAP_PHYSICAL_SIG "\xE8\x00\x00\x00\x00\x85\xC0\x0F\x88\x00\x00\x00\x00\x48\x8B\xBC\x24"
#define MAP_PHYSICAL_MASK "x????xxxx????xxxx"
static_assert(sizeof(MAP_PHYSICAL_SIG) == sizeof(MAP_PHYSICAL_MASK), "signature and mask do not patch sizes...\n");

typedef EFI_STATUS(EFIAPI* MAP_PHYSICAL)
(
    VOID** VirtualAddress, 
    VOID* PhysicalAddress,
    UINTN Size,
    VOID* Unknown,
    VOID* Unknown2
);
extern MAP_PHYSICAL MmMapPhysicalMemory;

typedef union _CR3
{
    UINTN Value;
    struct
    {
        UINTN reserved1 : 3;
        UINTN PageLevelWriteThrough : 1;
        UINTN PageLevelCacheDisable : 1;
        UINTN reserved2 : 7;
        UINTN Pml4Pfn : 36;
        UINTN reserved3 : 16;
    };
} CR3;

typedef union _VIRT_ADDR_T
{
    void* Value;
    struct
    {
        UINT64 offset : 12;
        UINT64 PtIdx : 9;
        UINT64 PdIdx : 9;
        UINT64 PdptIdx : 9;
        UINT64 Pml4Idx : 9;
        UINT64 reserved : 16;
    };
} VIRT_ADDR_T, *PVIRT_ADDR_T;


typedef union _PML4E_T
{
    UINT64 Value;
    struct
    {
        UINT64 Present : 1;          // Must be 1, region invalid if 0.
        UINT64 ReadWrite : 1;        // If 0, writes not allowed.
        UINT64 UserSuperVisor : 1;   // If 0, user-mode accesses not allowed.
        UINT64 page_write_through : 1; // Determines the memory type used to access PDPT.
        UINT64 page_cache : 1; // Determines the memory type used to access PDPT.
        UINT64 accessed : 1;         // If 0, this entry has not been used for translation.
        UINT64 ignore_1 : 1;
        UINT64 LargePage : 1;         // Must be 0 for PML4E.
        UINT64 ignore_2 : 4;
        UINT64 Pfn : 36; // The page frame number of the PDPT of this PML4E.
        UINT64 reserved : 4;
        UINT64 ignore_3 : 11;
        UINT64 nx : 1; // If 1, instruction fetches not allowed.
    };
} PML4E_T, * PPML4E_T;

typedef union _PDPTE
{
    UINT64 Value;
    struct
    {
        UINT64 Present : 1;          // Must be 1, region invalid if 0.
        UINT64 ReadWrite : 1;        // If 0, writes not allowed.
        UINT64 UserSuperVisor : 1;   // If 0, user-mode accesses not allowed.
        UINT64 page_write_through : 1; // Determines the memory type used to access PD.
        UINT64 page_cache : 1; // Determines the memory type used to access PD.
        UINT64 accessed : 1;         // If 0, this entry has not been used for translation.
        UINT64 ignore_1 : 1;
        UINT64 LargePage : 1;         // If 1, this entry maps a 1GB page.
        UINT64 ignore_2 : 4;
        UINT64 Pfn : 36; // The page frame number of the PD of this PDPTE.
        UINT64 reserved : 4;
        UINT64 ignore_3 : 11;
        UINT64 nx : 1; // If 1, instruction fetches not allowed.
    };
} PDPTE_T, * PPDPTE_T;

typedef union _PDE
{
    UINT64 Value;
    struct
    {
        UINT64 Present : 1;          // Must be 1, region invalid if 0.
        UINT64 ReadWrite : 1;        // If 0, writes not allowed.
        UINT64 UserSuperVisor : 1;   // If 0, user-mode accesses not allowed.
        UINT64 page_write_through : 1; // Determines the memory type used to access PT.
        UINT64 page_cache : 1; // Determines the memory type used to access PT.
        UINT64 accessed : 1;         // If 0, this entry has not been used for translation.
        UINT64 ignore_1 : 1;
        UINT64 LargePage : 1; // If 1, this entry maps a 2MB page.
        UINT64 ignore_2 : 4;
        UINT64 Pfn : 36; // The page frame number of the PT of this PDE.
        UINT64 reserved : 4;
        UINT64 ignore_3 : 11;
        UINT64 nx : 1; // If 1, instruction fetches not allowed.
    };
} PDE_T, * PPDE_T;

typedef union _PTE
{
    UINT64 Value;
    struct
    {
        UINT64 Present : 1;          // Must be 1, region invalid if 0.
        UINT64 ReadWrite : 1;        // If 0, writes not allowed.
        UINT64 UserSuperVisor : 1;   // If 0, user-mode accesses not allowed.
        UINT64 page_write_through : 1; // Determines the memory type used to access the memory.
        UINT64 page_cache : 1; // Determines the memory type used to access the memory.
        UINT64 accessed : 1;         // If 0, this entry has not been used for translation.
        UINT64 dirty : 1;            // If 0, the memory backing this page has not been written to.
        UINT64 access_type : 1;   // Determines the memory type used to access the memory.
        UINT64 global : 1;           // If 1 and the PGE bit of CR4 is set, translations are global.
        UINT64 ignore_2 : 3;
        UINT64 Pfn : 36; // The page frame number of the backing physical page.
        UINT64 reserved : 4;
        UINT64 ignore_3 : 7;
        UINT64 pk : 4;  // If the PKE bit of CR4 is set, determines the protection key.
        UINT64 nx : 1; // If 1, instruction fetches not allowed.
    };
} PTE_T, *PPTE_T;

typedef struct _TABLE_ENTRIES
{
    PML4E_T Pml4Entry;
    PDPTE_T PdptEntry;
    PDE_T PdEntry;
    PTE_T PtEntry;
} TABLE_ENTRIES, *PTABLE_ENTRIES;