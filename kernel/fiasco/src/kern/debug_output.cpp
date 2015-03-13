INTERFACE:

void dbgprintf_impl(const char *format, ...);

#ifdef CONFIG_DEBUG_OUTPUT
#define dbgprintf(...) dbgprintf_impl(__VA_ARGS__)
#else
#define dbgprintf(...) do { } while (0)
#endif /* CONFIG_DEBUG_OUTPUT */

IMPLEMENTATION:

#include <stdio.h>
#include <stdarg.h>

void dbgprintf_impl(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}
