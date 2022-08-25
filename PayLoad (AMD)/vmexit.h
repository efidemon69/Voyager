#pragma once
#include "mm.h"

namespace vmexit
{
	auto get_command(guest_virt_t command_ptr) -> svm::command_t;
	auto set_command(guest_virt_t command_ptr, svm::command_t& command_data) -> void;
}