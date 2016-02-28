// cmm_efun_core.cpp

#include <stdio.h>
#include "std_port/std_port.h"
#include "cmm_efun.h"
#include "cmm_efun_core.h"
#include "cmm_output.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Efun: error(string format, ...)
DEFINE_EFUN(void, error, (string format, ...))
{
    // Update argument no for RANDOM_ARG function
    _thread->get_this_call_context()->m_arg_no = __n;

    String& format = (String&)__args[0];
    Output output;
    String ret = output.format_output(format.c_str(), &__args[1], __n - 1);
    throw_error("%s", ret.c_str());
}

// Efun: printf(string format, ...)
DEFINE_EFUN(void, printf, (string format, ...))
{
    String& format = (String&)__args[0];
    Output output;
    String ret = output.format_output(format.c_str(), &__args[1], __n - 1);
    printf("%s", ret.c_str());
    return NIL;
}

int init_efun_core()
{
    // Efun definitions
    EfunDef core_efuns[] =
    {
        { EFUN_ITEM(error) },
        { EFUN_ITEM(printf) },
        { 0, 0 }
    };
    Efun::add_efuns("system.core", core_efuns);
    return 0;
}

void shutdown_efun_core()
{
}

}
