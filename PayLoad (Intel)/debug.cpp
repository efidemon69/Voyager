#include "debug.h"

auto dbg::debug_print_decimal(long long number) -> void
{
    if (number < 0) 
    {
        __outbyte(PORT_NUM, '-');
        number = -number;
    }

    for (auto d = 1000000000000000000; d != 0; d /= 10) 
        if ((number / d) != 0)
            __outbyte(PORT_NUM, alphabet[(number / d) % 10]);
}

auto dbg::debug_print_hex(u64 number, const bool show_zeros) -> void
{
    for (auto d = 0x1000000000000000; d != 0; d /= 0x10) 
        if (show_zeros || (number / d) != 0) 
            __outbyte(PORT_NUM, alphabet[(number / d) % 0x10]);
}

auto dbg::print(const char* format, ...) -> void
{
    va_list args;
    va_start(args, format);

    while (format[0]) 
    {
        if (format[0] == '%') 
        {
            switch (format[1]) 
            {
            case 'd':
                debug_print_decimal(va_arg(args, int));
                format += 2;
                continue;
            case 'x':
                debug_print_hex(va_arg(args, u32), false);
                format += 2;
                continue;
            case 'l':
                if (format[2] == 'l') 
                {
                    switch (format[3]) 
                    {
                    case 'd':
                        debug_print_decimal(va_arg(args, u64));
                        format += 4;
                        continue;
                    case 'x':
                        debug_print_hex(va_arg(args, u64), false);
                        format += 4;
                        continue;
                    }
                }
                break;
            case 'p':
                debug_print_hex(va_arg(args, u64), true);
                format += 2;
                continue;
            }
        }

        __outbyte(PORT_NUM, format[0]);
        ++format;
    }
    va_end(args);
}