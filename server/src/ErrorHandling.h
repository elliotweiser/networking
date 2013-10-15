#ifndef ERROR_HANDLING_H
#define ERROR_HANDLING_H

#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <unistd.h>

/*
    Print an error message (with formatted string and optional arguments) and
    exit with given code
    Parameter(s): int, code with which to call exit
                  const char*, the format string
                  ..., the optional arguments
*/
extern void error(int code, const char* format, ...);

#endif
