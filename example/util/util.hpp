#pragma once
#include <Windows.h>
#include <ntstatus.h>

#include <cstdint>
#include <string_view>
#include <algorithm>
#include <string_view>
#include <tlhelp32.h>
#include <map>
#include "nt.hpp"

namespace util
{
	inline std::map<std::uintptr_t, std::size_t> pmem_ranges{};
	__forceinline auto is_valid(std::uintptr_t addr) -> bool
	{
		for (auto range : pmem_ranges)
			if (addr >= range.first && addr <= range.first + range.second)
				return true;

		return false;
	}

#pragma pack (push, 1)
	struct PhysicalMemoryPage//CM_PARTIAL_RESOURCE_DESCRIPTOR
	{
		uint8_t type;
		uint8_t shareDisposition;
		uint16_t flags;
		uint64_t pBegin;
		uint32_t sizeButNotExactly;
		uint32_t pad;

		static constexpr uint16_t cm_resource_memory_large_40{ 0x200 };
		static constexpr uint16_t cm_resource_memory_large_48{ 0x400 };
		static constexpr uint16_t cm_resource_memory_large_64{ 0x800 };

		uint64_t size()const noexcept
		{
			if (flags & cm_resource_memory_large_40)
				return uint64_t{ sizeButNotExactly } << 8;
			else if (flags & cm_resource_memory_large_48)
				return uint64_t{ sizeButNotExactly } << 16;
			else if (flags & cm_resource_memory_large_64)
				return uint64_t{ sizeButNotExactly } << 32;
			else
				return uint64_t{ sizeButNotExactly };
		}
	};
	static_assert(sizeof(PhysicalMemoryPage) == 20);
#pragma pack (pop)

	inline const auto init_ranges = ([&]() -> bool
	{
			HKEY h_key;
			DWORD type, size;
			LPBYTE data;
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, "HARDWARE\\RESOURCEMAP\\System Resources\\Physical Memory", 0, KEY_READ, &h_key);
			RegQueryValueEx(h_key, ".Translated", NULL, &type, NULL, &size); //get size
			data = new BYTE[size];
			RegQueryValueEx(h_key, ".Translated", NULL, &type, data, &size);
			DWORD count = *(DWORD*)(data + 16);
			auto pmi = data + 24;
			for (int dwIndex = 0; dwIndex < count; dwIndex++)
			{
#if 0
				pmem_ranges.emplace(*(uint64_t*)(pmi + 0), *(uint64_t*)(pmi + 8));
#else
				const PhysicalMemoryPage& page{ *(PhysicalMemoryPage*)(pmi - 4) };
				pmem_ranges.emplace(page.pBegin, page.size());
#endif
				pmi += 20;
			}
			delete[] data;
			RegCloseKey(h_key);
			return true;
	})();

	__forceinline auto get_file_header(void* base_addr) -> PIMAGE_FILE_HEADER
	{
		PIMAGE_DOS_HEADER dos_headers =
			reinterpret_cast<PIMAGE_DOS_HEADER>(base_addr);

		PIMAGE_NT_HEADERS nt_headers =
			reinterpret_cast<PIMAGE_NT_HEADERS>(
				reinterpret_cast<DWORD_PTR>(base_addr) + dos_headers->e_lfanew);

		return &nt_headers->FileHeader;
	}

	__forceinline auto get_kmodule_base(const char* module_name) -> std::uintptr_t
	{
		void* buffer = nullptr;
		DWORD buffer_size = NULL;

		auto status = NtQuerySystemInformation(
			static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
				buffer, buffer_size, &buffer_size);

		while (status == STATUS_INFO_LENGTH_MISMATCH)
		{
			VirtualFree(buffer, NULL, MEM_RELEASE);
			buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			status = NtQuerySystemInformation(
				static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), 
					buffer, buffer_size, &buffer_size);
		}

		if (!NT_SUCCESS(status))
		{
			VirtualFree(buffer, NULL, MEM_RELEASE);
			return NULL;
		}

		const auto modules = static_cast<PRTL_PROCESS_MODULES>(buffer);
		for (auto idx = 0u; idx < modules->NumberOfModules; ++idx)
		{
			const std::string current_module_name = std::string(reinterpret_cast<char*>(modules->Modules[idx].FullPathName) + modules->Modules[idx].OffsetToFileName);
			if (!_stricmp(current_module_name.c_str(), module_name))
			{
				const uint64_t result = reinterpret_cast<uint64_t>(modules->Modules[idx].ImageBase);
				VirtualFree(buffer, NULL, MEM_RELEASE);
				return result;
			}
		}

		VirtualFree(buffer, NULL, MEM_RELEASE);
		return NULL;
	}

	__forceinline auto get_kmodule_export(const char* module_name, const char* export_name, bool rva = false) -> void*
	{
		void* buffer = nullptr;
		DWORD buffer_size = NULL;

		NTSTATUS status = NtQuerySystemInformation(
			static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation),
			buffer,
			buffer_size,
			&buffer_size
		);

		while (status == STATUS_INFO_LENGTH_MISMATCH)
		{
			VirtualFree(buffer, 0, MEM_RELEASE);
			buffer = VirtualAlloc(nullptr, buffer_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
			status = NtQuerySystemInformation(
				static_cast<SYSTEM_INFORMATION_CLASS>(SystemModuleInformation), 
				buffer, 
				buffer_size, 
				&buffer_size
			);
		}

		if (!NT_SUCCESS(status))
		{
			VirtualFree(buffer, 0, MEM_RELEASE);
			return nullptr;
		}

		const auto modules = static_cast<PRTL_PROCESS_MODULES>(buffer);
		for (auto idx = 0u; idx < modules->NumberOfModules; ++idx)
		{
			// find module and then load library it
			const std::string current_module_name = 
				std::string(reinterpret_cast<char*>(
					modules->Modules[idx].FullPathName) +
					modules->Modules[idx].OffsetToFileName
				);

			if (!_stricmp(current_module_name.c_str(), module_name))
			{
				std::string full_path = reinterpret_cast<char*>(modules->Modules[idx].FullPathName);
				full_path.replace(full_path.find("\\SystemRoot\\"), 
					sizeof("\\SystemRoot\\") - 1, std::string(getenv("SYSTEMROOT")).append("\\"));

				const auto module_base = 
					LoadLibraryEx(
						full_path.c_str(),
						NULL, 
						DONT_RESOLVE_DLL_REFERENCES
					);

				PIMAGE_DOS_HEADER p_idh;
				PIMAGE_NT_HEADERS p_inh;
				PIMAGE_EXPORT_DIRECTORY p_ied;

				PDWORD addr, name;
				PWORD ordinal;

				p_idh = (PIMAGE_DOS_HEADER)module_base;
				if (p_idh->e_magic != IMAGE_DOS_SIGNATURE)
					return NULL;

				p_inh = (PIMAGE_NT_HEADERS)((LPBYTE)module_base + p_idh->e_lfanew);
				if (p_inh->Signature != IMAGE_NT_SIGNATURE)
					return NULL;

				if (p_inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress == 0)
					return NULL;

				p_ied = (PIMAGE_EXPORT_DIRECTORY)((LPBYTE)module_base +
					p_inh->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

				addr = (PDWORD)((LPBYTE)module_base + p_ied->AddressOfFunctions);
				name = (PDWORD)((LPBYTE)module_base + p_ied->AddressOfNames);
				ordinal = (PWORD)((LPBYTE)module_base + p_ied->AddressOfNameOrdinals);

				// find exported function
				for (auto i = 0; i < p_ied->AddressOfFunctions; i++)
				{
					if (!strcmp(export_name, (char*)module_base + name[i]))
					{
						if (!rva)
						{
							auto result = (void*)((std::uintptr_t)modules->Modules[idx].ImageBase + addr[ordinal[i]]);
							VirtualFree(buffer, NULL, MEM_RELEASE);
							return result;
						}
						else
						{
							auto result = (void*)addr[ordinal[i]];
							VirtualFree(buffer, NULL, MEM_RELEASE);
							return result;
						}
					}
				}
			}
		}

		VirtualFree(buffer, NULL, MEM_RELEASE);
		return nullptr;
	}

	__forceinline auto get_pid(const char* proc_name) -> std::uint32_t
	{
		PROCESSENTRY32 proc_info;
		proc_info.dwSize = sizeof(proc_info);

		HANDLE proc_snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (proc_snapshot == INVALID_HANDLE_VALUE)
			return NULL;

		Process32First(proc_snapshot, &proc_info);
		if (!strcmp(proc_info.szExeFile, proc_name))
		{
			CloseHandle(proc_snapshot);
			return proc_info.th32ProcessID;
		}

		while (Process32Next(proc_snapshot, &proc_info))
		{
			if (!strcmp(proc_info.szExeFile, proc_name))
			{
				CloseHandle(proc_snapshot);
				return proc_info.th32ProcessID;
			}
		}

		CloseHandle(proc_snapshot);
		return NULL;
	}
}