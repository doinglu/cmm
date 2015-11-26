// cmm_shell.h
// Created by doing Nov/10/2015

#pragma once

#include <stdarg.h>

namespace cmm
{

void cmm_printf(const char *format, ...);
void cmm_errprintf(const char *format, ...);
void cmm_vprintf(const char *format, va_list args);

}
