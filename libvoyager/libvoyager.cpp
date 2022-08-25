#include "libvoyager.hpp"
#include <Windows.h>
#include <iostream>

// taken from hvpp (wbenny). this code runs specific code on each logical processor...
// this is required since hyper-v has its own PML4 for each core...
// https://github.com/wbenny/hvpp/blob/master/src/hvppctrl/lib/mp.cpp#L4
auto voyager::init() -> vmxroot_error_t
{
    GROUP_AFFINITY orig_group_affinity;
    GetThreadGroupAffinity(GetCurrentThread(), &orig_group_affinity);
    const auto group_count = GetActiveProcessorGroupCount();

    // each core we are going to vmexit on and ask the payload
    // to setup the mapping pml4e. for some reason each core on
    // hyper-v has its own pml4... Not sure why? just is...
    for (auto group_number = 0u; group_number < group_count; ++group_number)
    {
        const auto processor_count = GetActiveProcessorCount(group_number);
        for (auto processor_number = 0u; processor_number < processor_count; ++processor_number)
        {
            GROUP_AFFINITY group_affinity = { 0 };
            group_affinity.Mask = (KAFFINITY)(1) << processor_number;
            group_affinity.Group = group_number;
            SetThreadGroupAffinity(GetCurrentThread(), &group_affinity, NULL);

            auto result = hypercall(VMEXIT_KEY, vmexit_command_t::init_page_tables, nullptr);
            if (result != vmxroot_error_t::error_success)
                return result;
        }
    }

    SetThreadGroupAffinity(GetCurrentThread(), &orig_group_affinity, NULL);
    return vmxroot_error_t::error_success;
}

auto voyager::current_dirbase()->guest_phys_t
{
    command_t command;
    auto result = hypercall(VMEXIT_KEY, vmexit_command_t::get_dirbase, &command);

    if (result != vmxroot_error_t::error_success)
        return {};

    return command.dirbase;
}

auto voyager::translate(guest_virt_t virt_addr) -> guest_phys_t
{
    command_t command;
    command.translate_virt.virt_src = virt_addr;

    const auto result = hypercall(VMEXIT_KEY, vmexit_command_t::translate, &command);

    if (result != vmxroot_error_t::error_success)
        return {};

    return command.translate_virt.phys_addr;
}

auto voyager::read_phys(guest_phys_t phys_addr, guest_virt_t buffer, u64 size) -> vmxroot_error_t
{
    command_t command;
    command.copy_phys = { phys_addr, buffer, size };
    return hypercall(VMEXIT_KEY, vmexit_command_t::read_guest_phys, &command);
}

auto voyager::write_phys(guest_phys_t phys_addr, guest_virt_t buffer, u64 size) -> vmxroot_error_t
{
    command_t command;
    command.copy_phys = { phys_addr, buffer, size };
    return hypercall(VMEXIT_KEY, vmexit_command_t::write_guest_phys, &command);
}

auto voyager::copy_virt(guest_phys_t dirbase_src, guest_virt_t virt_src, guest_phys_t dirbase_dest,
    guest_virt_t virt_dest, u64 size) -> vmxroot_error_t
{
    command_t command;
    command.copy_virt = { dirbase_src, virt_src, dirbase_dest, virt_dest, size };
    return hypercall(VMEXIT_KEY, vmexit_command_t::copy_guest_virt, &command);
}