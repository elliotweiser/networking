#include "ErrorHandling.h"

void error(int code, const char* format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(code);
}
