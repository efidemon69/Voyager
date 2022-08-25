#pragma once
#include <stdarg.h>
#include <stddef.h>
#include "types.h"

#define PORT_NUM 0x2F8
namespace dbg
{
	constexpr char alphabet[] = "0123456789ABCDEF";
	auto debug_print_decimal(long long number) -> void;
	auto debug_print_hex(u64 number, const bool show_zeros) -> void;
	auto print(const char* format, ...) -> void;
}