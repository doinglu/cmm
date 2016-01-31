// cmm_shell.c
// Created by doing Jan/26/2016

#include <stdarg.h>
#include <stdio.h>
#include "cmm.h"
#include "cmm_shell.h"

namespace cmm
{

// Send to current I/O
void cmm_printf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    cmm_vprintf(format, args);
    va_end(args);
}

// Send to error I/O
void cmm_errprintf(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    cmm_vprintf(format, args);
    va_end(args);
}

// Send to current I/O
void cmm_vprintf(const char *format, va_list args)
{
    char buf[4096];
    vsnprintf(buf, sizeof(buf), format, args);
    puts(buf);
}

}
