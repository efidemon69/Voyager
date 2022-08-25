#pragma once
#include "mm.h"
#include "debug.h"

namespace vmexit
{
	auto get_command(guest_virt_t command_ptr) -> command_t;
	auto set_command(guest_virt_t command_ptr, command_t& command_data) -> void;
}