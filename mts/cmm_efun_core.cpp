// cmm_efun_core.cpp

#include <stdio.h>
#include "std_port/std_port.h"
#include "cmm_efun.h"
#include "cmm_efun_core.h"
#include "cmm_value.h"

namespace cmm
{

// Efun: printf(string format, ...)
DEFINE_EFUN(void, printf, (string format, ...))
{
    String& format = (String&)__args[0];
    printf(format.c_str(), __args[1].get_int());
    return Value();
}

int init_efun_core()
{
    // Efun definitions
    EfunDef core_efuns[] =
    {
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
