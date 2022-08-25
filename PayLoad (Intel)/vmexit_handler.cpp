#include "vmexit.h"

#if WINVER > 1803
void vmexit_handler(pcontext_t* context, void* unknown)
#else
void vmexit_handler(pcontext_t context, void* unknown)
#endif
{

#if WINVER > 1803
	pcontext_t guest_registers = *context;
#else
	pcontext_t guest_registers = context;
#endif

	size_t vmexit_reason;
	__vmx_vmread(VMCS_EXIT_REASON, &vmexit_reason);
	if (vmexit_reason == VMX_EXIT_REASON_EXECUTE_CPUID)
	{
		if (guest_registers->rcx == VMEXIT_KEY)
		{
			switch ((vmexit_command_t)guest_registers->rdx)
			{
			case vmexit_command_t::init_page_tables:
			{
				guest_registers->rax = (u64) mm::init();
				break;
			}
			case vmexit_command_t::get_dirbase:
			{
				auto command_data =
					vmexit::get_command(guest_registers->r8);

				u64 guest_dirbase;
				__vmx_vmread(VMCS_GUEST_CR3, &guest_dirbase);

				// cr3 can contain other high bits so just to be safe
				// get the pfn and bitshift it...
				guest_dirbase = cr3{ guest_dirbase }.pml4_pfn << 12;
				command_data.dirbase = guest_dirbase;
				guest_registers->rax = (u64) vmxroot_error_t::error_success;

				vmexit::set_command(
					guest_registers->r8, command_data);
				break;
			}
			case vmexit_command_t::read_guest_phys:
			{
				auto command_data =
					vmexit::get_command(guest_registers->r8);

				u64 guest_dirbase;
				__vmx_vmread(VMCS_GUEST_CR3, &guest_dirbase);
				// from 1809-1909 PCIDE is enabled in CR4 and so cr3 contains some other stuff...
				guest_dirbase = cr3{ guest_dirbase }.pml4_pfn << 12;

				guest_registers->rax =
					(u64) mm::read_guest_phys(
						guest_dirbase,
						command_data.copy_phys.phys_addr,
						command_data.copy_phys.buffer,
						command_data.copy_phys.size);

				vmexit::set_command(
					guest_registers->r8, command_data);
				break;
			}
			case vmexit_command_t::write_guest_phys:
			{
				auto command_data =
					vmexit::get_command(guest_registers->r8);

				u64 guest_dirbase;
				__vmx_vmread(VMCS_GUEST_CR3, &guest_dirbase);
				// from 1809-1909 PCIDE is enabled in CR4 and so cr3 contains some other stuff...
				guest_dirbase = cr3{ guest_dirbase }.pml4_pfn << 12;

				guest_registers->rax =
					(u64) mm::write_guest_phys(
						guest_dirbase,
						command_data.copy_phys.phys_addr,
						command_data.copy_phys.buffer,
						command_data.copy_phys.size);

				vmexit::set_command(
					guest_registers->r8, command_data);
				break;
			}
			case vmexit_command_t::copy_guest_virt:
			{
				auto command_data =
					vmexit::get_command(guest_registers->r8);

				auto virt_data = command_data.copy_virt;
				guest_registers->rax =
					(u64)mm::copy_guest_virt(
						virt_data.dirbase_src,
						virt_data.virt_src,
						virt_data.dirbase_dest,
						virt_data.virt_dest,
						virt_data.size);
				break;
			}
			case vmexit_command_t::translate:
			{
				auto command_data =
					vmexit::get_command(guest_registers->r8);

				u64 guest_dirbase;
				__vmx_vmread(VMCS_GUEST_CR3, &guest_dirbase);
				guest_dirbase = cr3{ guest_dirbase }.pml4_pfn << 12;

				command_data.translate_virt.phys_addr =
					mm::translate_guest_virtual(guest_dirbase, 
						command_data.translate_virt.virt_src);

				guest_registers->rax = 
					(u64) vmxroot_error_t::error_success;

				vmexit::set_command(
					guest_registers->r8, command_data);
				break;
			}
			default:
				break;
			}

			// advance instruction pointer...
			size_t rip, exec_len;
			__vmx_vmread(VMCS_GUEST_RIP, &rip);
			__vmx_vmread(VMCS_VMEXIT_INSTRUCTION_LENGTH, &exec_len);
			__vmx_vmwrite(VMCS_GUEST_RIP, rip + exec_len);
			return;
		}
	}

	// call original vmexit handler...
	reinterpret_cast<vmexit_handler_t>(
		reinterpret_cast<u64>(&vmexit_handler) -
			voyager_context.vmexit_handler_rva)(context, unknown);
}