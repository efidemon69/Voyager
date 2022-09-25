#include "InlineHook.h"

// x64-86 Compatible

VOID MakeInlineHook(PINLINE_HOOK_T Hook, VOID* HookFrom, VOID* HookTo, BOOLEAN Install)
{
	unsigned char JmpCode[14] =
	{
		0xff, 0x25, 0x0, 0x0, 0x0, 0x0,		// jmp    QWORD PTR[rip + 0x0]

		// jmp address...
		0x0, 0x0, 0x0, 0x0,
		0x0, 0x0, 0x0, 0x0
	};

	// save original bytes, and hook related addresses....
	Hook->Address = HookFrom;
	Hook->HookAddress = HookTo;
	MemCopy(Hook->Code, HookFrom, sizeof Hook->Code);

	// setup hook...
	MemCopy(JmpCode + 6, &HookTo, sizeof HookTo);
	MemCopy(Hook->JmpCode, JmpCode, sizeof JmpCode);
	if (Install) EnableInlineHook(Hook);
}

VOID EnableInlineHook(PINLINE_HOOK_T Hook)
{
	MemCopy(Hook->Address, Hook->JmpCode, sizeof Hook->JmpCode);
}

VOID DisableInlineHook(PINLINE_HOOK_T Hook)
{
	MemCopy(Hook->Address, Hook->Code, sizeof Hook->Code);
}
