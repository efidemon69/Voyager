#include "vmexit.h"
#include "debug.h"

namespace vmexit
{
	auto get_command(guest_virt_t command_ptr) -> svm::command_t
	{
		const auto vmcb = svm::get_vmcb();
		const auto guest_dirbase = cr3{ vmcb->cr3 }.pml4_pfn << 12;

		const auto command_page = 
			mm::map_guest_virt(guest_dirbase, command_ptr);

		return *reinterpret_cast<svm::command_t*>(command_page);
	}

	auto set_command(guest_virt_t command_ptr, svm::command_t& command_data) -> void
	{
		const auto vmcb = svm::get_vmcb();
		const auto guest_dirbase = cr3{ vmcb->cr3 }.pml4_pfn << 12;

		const auto command_page =
			mm::map_guest_virt(guest_dirbase, command_ptr);

		*reinterpret_cast<svm::command_t*>(command_page) = command_data;
	}
}