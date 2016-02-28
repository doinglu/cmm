// cmm_error.cpp

#include <stdio.h>
#include <stdarg.h>
#include "cmm.h"
#include "cmm_error.h"
#include "cmm_output.h"
#include "cmm_program.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Print variables (arguments & local) of function
void print_variables(const Variables& variables, const char *type, Value *arr, ArgNo n)
{
    Output output;
    auto r = ReserveStack(1);
    Value& ret = r[0];

    ArgNo i = 0;
    for (auto &it : (Variables&)variables)
    {
        printf("%s %s(%d) = ", type, it->get_name()->c_str(), i + 1);
        ret = output.type_value(&arr[i]);
        printf("%s\n", ret.m_string->c_str());
        i++;
    }

    while (i < n)
    {
        printf("%s $%d = ", type, i + 1);
        ret = output.type_value(&arr[i]);
        printf("%s\n", ret.m_string->c_str());
        i++;
    }
}

// Throw exception
void throw_error(const char *msg, ...)
{
    // Create formatted string
    va_list va;
    va_start(va, msg);
    char buf[1024];
    STD_VSNPRINTF(buf, sizeof(buf), msg, va);
    buf[sizeof(buf) - 1] = 0;
    va_end(va);

    // Trace callstack
    Thread::get_current_thread()->trace_call_stack();

    // Throw the message
    printf("buf[%p] = %s\n", buf, buf);////----
    throw buf;
}

}