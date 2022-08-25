#include "types.h"
#include "mm.h"
#include "vmexit.h"
#include "debug.h"

auto vmexit_handler(void* unknown, svm::pguest_context context) -> svm::pgs_base_struct
{
	const auto vmcb = svm::get_vmcb();
	if (vmcb->exitcode == VMEXIT_CPUID && context->rcx == VMEXIT_KEY)
	{
		switch ((svm::vmexit_command_t)context->rdx)
		{
		case svm::vmexit_command_t::init_page_tables:
		{
			vmcb->rax = (u64) mm::init();
			break;
		}
		case svm::vmexit_command_t::get_dirbase:
		{
			auto command_data =
				vmexit::get_command(context->r8);

			command_data.dirbase = 
				cr3{ vmcb->cr3 }.pml4_pfn << 12;

			vmcb->rax = (u64)svm::vmxroot_error_t::error_success;

			vmexit::set_command(
				context->r8, command_data);
			break;
		}
		case svm::vmexit_command_t::read_guest_phys:
		{
			auto command_data =
				vmexit::get_command(context->r8);

			const auto guest_dirbase = 
				cr3{ vmcb->cr3 }.pml4_pfn << 12;

			vmcb->rax = (u64)mm::read_guest_phys(
				guest_dirbase,
				command_data.copy_phys.phys_addr,
				command_data.copy_phys.buffer,
				command_data.copy_phys.size);

			vmexit::set_command(
				context->r8, command_data);
			break;
		}
		case svm::vmexit_command_t::write_guest_phys:
		{
			auto command_data =
				vmexit::get_command(context->r8);

			const auto guest_dirbase =
				cr3{ vmcb->cr3 }.pml4_pfn << 12;

			vmcb->rax = (u64) mm::write_guest_phys(
				guest_dirbase,
				command_data.copy_phys.phys_addr,
				command_data.copy_phys.buffer,
				command_data.copy_phys.size);

			vmexit::set_command(
				context->r8, command_data);
			break;
		}
		case svm::vmexit_command_t::copy_guest_virt:
		{
			auto command_data =
				vmexit::get_command(context->r8);

			auto virt_data = command_data.copy_virt;
			vmcb->rax = (u64) mm::copy_guest_virt(
				virt_data.dirbase_src,
				virt_data.virt_src, 
				virt_data.dirbase_dest,
				virt_data.virt_dest, 
				virt_data.size);
			break;
		}
		case svm::vmexit_command_t::translate:
		{
			auto command_data =
				vmexit::get_command(context->r8);

			const auto guest_dirbase =
				cr3{ vmcb->cr3 }.pml4_pfn << 12;

			command_data.translate_virt.phys_addr =
				mm::translate_guest_virtual(guest_dirbase, 
					command_data.translate_virt.virt_src);

			vmcb->rax = (u64)svm::vmxroot_error_t::error_success;

			vmexit::set_command(
				context->r8, command_data);
			break;
		}
		default:
			break;
		}

		vmcb->rip = vmcb->nrip;
		return reinterpret_cast<svm::pgs_base_struct>(__readgsqword(0));
	}

	return reinterpret_cast<svm::vcpu_run_t>(
		reinterpret_cast<u64>(&vmexit_handler) -
			svm::voyager_context.vcpu_run_rva)(unknown, context);
}