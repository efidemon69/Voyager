#include <iostream>
#include "libvoyager.hpp"
#include "util/util.hpp"
#include "vdm_ctx/vdm_ctx.hpp"

int __cdecl main(int argc, char** argv)
{
	vdm::read_phys_t _read_phys = 
		[&](void* addr, void* buffer, std::size_t size) -> bool
	{
		const auto read_result = 
			voyager::read_phys((u64)addr, (u64)buffer, size);

		return read_result == 
			voyager::vmxroot_error_t::error_success;
	};

	vdm::write_phys_t _write_phys =
		[&](void* addr, void* buffer, std::size_t size) -> bool
	{
		const auto write_result = 
			voyager::write_phys((u64)addr, (u64)buffer, size);

		return write_result ==
			voyager::vmxroot_error_t::error_success;
	};

	std::printf("[+] voyager init result -> 0x%x (0x0 == success)\n", voyager::init());
	std::printf("[+] current dirbase -> 0x%p\n", voyager::current_dirbase());
	std::printf("[+] please wait... this may take up to 30 seconds...\n");

	const auto nt_shutdown_system = 
		util::get_kmodule_export(
			"ntoskrnl.exe", vdm::syscall_hook.first);
	
	const auto nt_shutdown_phys = 
		voyager::translate(reinterpret_cast<
			voyager::guest_virt_t>(nt_shutdown_system));

	std::printf("NtShutdownSystem -> 0x%p\n", nt_shutdown_system);
	std::printf("NtShutdownSystem (phys) -> 0x%p\n", nt_shutdown_phys);
	vdm::syscall_address.store(reinterpret_cast<void*>(nt_shutdown_phys));

	vdm::vdm_ctx vdm(_read_phys, _write_phys);
	const auto ntoskrnl_base =
		reinterpret_cast<void*>(
			util::get_kmodule_base("ntoskrnl.exe"));

	const auto ntoskrnl_memcpy =
		util::get_kmodule_export("ntoskrnl.exe", "memcpy");

	std::printf("[+] %s physical address -> 0x%p\n", vdm::syscall_hook.first, vdm::syscall_address.load());
	std::printf("[+] %s page offset -> 0x%x\n", vdm::syscall_hook.first, vdm::nt_page_offset);
	std::printf("[+] ntoskrnl base address -> 0x%p\n", ntoskrnl_base);
	std::printf("[+] ntoskrnl memcpy address -> 0x%p\n", ntoskrnl_memcpy);

	short mz_bytes = 0;
	vdm.syscall<decltype(&memcpy)>(
		ntoskrnl_memcpy,
		&mz_bytes,
		ntoskrnl_base,
		sizeof mz_bytes
	);

	std::printf("[+] ntoskrnl MZ -> 0x%x\n", mz_bytes);
	const auto explorer_dirbase = 
		vdm.get_dirbase(util::get_pid("explorer.exe"));

	const auto ntdll_base = 
		reinterpret_cast<std::uintptr_t>(
			GetModuleHandleA("ntdll.dll"));

	std::printf("explorer.exe dirbase -> 0x%p\n", explorer_dirbase);
	std::printf("current process dirbase -> %p\n", voyager::current_dirbase());
	std::printf("ntdll.dll base -> 0x%p\n", ntdll_base);
	std::printf("ntdll.dll MZ in explorer.exe -> 0x%x\n",
		voyager::rpm<short>(explorer_dirbase, ntdll_base));

	std::printf("[+] press any key to close...\n");
	std::getchar();
}
