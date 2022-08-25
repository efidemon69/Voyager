#include "vmexit.h"

namespace vmexit
{
	auto get_command(guest_virt_t command_ptr) -> command_t
	{
		u64 guest_dirbase;
		__vmx_vmread(VMCS_GUEST_CR3, &guest_dirbase);

		// cr3 can contain other high bits so just to be safe
		// get the pfn and bitshift it...
		guest_dirbase = cr3{ guest_dirbase }.pml4_pfn << 12;

		const auto command_page = 
			mm::map_guest_virt(guest_dirbase, command_ptr);

		return *reinterpret_cast<command_t*>(command_page);
	}

	auto set_command(guest_virt_t command_ptr, command_t& command_data) -> void
	{
		u64 guest_dirbase;
		__vmx_vmread(VMCS_GUEST_CR3, &guest_dirbase);

		// cr3 can contain other high bits so just to be safe
		// get the pfn and bitshift it...
		guest_dirbase = cr3{ guest_dirbase }.pml4_pfn << 12;

		const auto command_page =
			mm::map_guest_virt(guest_dirbase, command_ptr);

		*reinterpret_cast<command_t*>(command_page) = command_data;
	}
}