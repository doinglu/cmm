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

    String& format = (String&)_thread->get_arg(0);
    Output output;
    auto r = ReserveStack(1, _thread);
    Value& ret = r[0];
    ret = output.format_output(format.c_str(), &_thread->get_arg(1), __n - 1);
    throw_error("%s", ret.m_string->c_str());
}

// Efun: printf(string format, ...)
DEFINE_EFUN(void, printf, (string format, ...))
{
    String& format = (String&)_thread->get_arg(0);
    Output output;
    auto r = ReserveStack(1, _thread);
    Value& ret = r[0];
    ret = output.format_output(format.c_str(), &_thread->get_arg(1), __n - 1);
    printf("%s", ret.m_string->c_str());
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

    auto r = ReserveStack(1);
    String& temp = (String&)r[0];
    Efun::add_efuns(temp = "system.core", core_efuns);
    return 0;
}

void shutdown_efun_core()
{
}

}
