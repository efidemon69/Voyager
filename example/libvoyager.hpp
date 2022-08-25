#pragma once
#include <intrin.h>
#include <type_traits>

#define VMEXIT_KEY 0xDEADBEEFDEADBEEF
#define PAGE_4KB 0x1000
#define PAGE_2MB PAGE_4KB * 512
#define PAGE_1GB PAGE_2MB * 512

using u8 = unsigned char;
using u16 = unsigned short;
using u32 = unsigned int;
using u64 = unsigned long long;

namespace voyager
{
	// code comments itself...
	using guest_virt_t = u64;
	using guest_phys_t = u64;
	using host_virt_t = u64;
	using host_phys_t = u64;

	enum class vmexit_command_t
	{
		init_page_tables,
		read_guest_phys,
		write_guest_phys,
		copy_guest_virt,
		get_dirbase,
		translate
	};

	enum class vmxroot_error_t
	{
		error_success,
		pml4e_not_present,
		pdpte_not_present,
		pde_not_present,
		pte_not_present,
		vmxroot_translate_failure,
		invalid_self_ref_pml4e,
		invalid_mapping_pml4e,
		invalid_host_virtual,
		invalid_guest_physical,
		invalid_guest_virtual,
		page_table_init_failed
	};

	typedef union _command_t
	{
		struct _copy_phys
		{
			host_phys_t  phys_addr;
			guest_virt_t buffer;
			u64 size;
		} copy_phys;

		struct _copy_virt
		{
			guest_phys_t dirbase_src;
			guest_virt_t virt_src;
			guest_phys_t dirbase_dest;
			guest_virt_t virt_dest;
			u64 size;
		} copy_virt;

		struct _translate_virt
		{
			guest_virt_t virt_src;
			guest_phys_t phys_addr;
		} translate_virt;

		guest_phys_t dirbase;

	} command_t, * pcommand_t;

	/// <summary>
	/// this function is used to cause a vmexit as though its calling a function...
	/// </summary>
	extern "C" auto hypercall(u64 key, vmexit_command_t, pcommand_t command)->vmxroot_error_t;

	/// <summary>
	/// gets the current cores CR3 value (current address space pml4)...
	/// </summary>
	/// <returns>returns the guest cr3 value...</returns>
	auto current_dirbase()->guest_phys_t;

	/// <summary>
	/// initalizies page tables for all cores...
	/// </summary>
	/// <returns>status of the initalization...</returns>
	auto init()->vmxroot_error_t;

	/// <summary>
	/// translate a linear virtual address of the current address space
	/// to a linear physical address...
	/// </summary>
	/// <param name="virt_addr">virtual address in the vmexiting core's address space...</param>
	/// <returns>guest physical address...</returns>
	auto translate(guest_virt_t virt_addr)->guest_phys_t;

	/// <summary>
	/// reads guest physical memory...
	/// </summary>
	/// <param name="phys_addr">physical address to read...</param>
	/// <param name="buffer">buffer (guest virtual address) to read into...</param>
	/// <param name="size">number of bytes to read (can only be 0x1000 or less)...</param>
	/// <returns>STATUS_SUCCESS if the read was successful...</returns>
	auto read_phys(guest_phys_t phys_addr, guest_virt_t buffer, u64 size)->vmxroot_error_t;

	/// <summary>
	/// write guest physical memory...
	/// </summary>
	/// <param name="phys_addr">physical address to read</param>
	/// <param name="buffer">guest virtual address to write from...</param>
	/// <param name="size">number of bytes to write</param>
	/// <returns></returns>
	auto write_phys(guest_phys_t phys_addr, guest_virt_t buffer, u64 size)->vmxroot_error_t;

	/// <summary>
	/// copy guest virtual memory between virtual address spaces...
	/// </summary>
	/// <param name="dirbase_src">dirbase of the source address space</param>
	/// <param name="virt_src">virtual address in the source address space</param>
	/// <param name="dirbase_dest">dirbase of the destination address space</param>
	/// <param name="virt_dest">virtual address of the destination address</param>
	/// <param name="size">size to copy between address spaces</param>
	/// <returns>returns error_success on successful copy and invalid_guest_virt when an address is invalid...</returns>
	auto copy_virt(guest_phys_t dirbase_src, guest_virt_t virt_src, guest_phys_t dirbase_dest,
		guest_virt_t virt_dest, u64 size)->vmxroot_error_t;

	template <class T>
	auto rpm(guest_phys_t dirbase, guest_virt_t virt_addr) -> T
	{
		T buffer;
		auto result = copy_virt(dirbase, virt_addr,
			current_dirbase(), (guest_virt_t)&buffer, sizeof T);

		if (result != vmxroot_error_t::error_success)
			return {};

		return buffer;
	}

	template <class T>
	auto wpm(guest_phys_t dirbase, guest_virt_t virt_addr, const T& data) -> void
	{
		copy_virt(current_dirbase(), (guest_virt_t)&data,
			dirbase, virt_addr, sizeof T);
	}
}