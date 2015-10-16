// cmm_efun_core.cpp

#include "std_port/std_port.h"
#include "cmm_efun.h"
#include "cmm_efun_core.h"
#include "cmm_value.h"

namespace cmm
{

#define DECLARE_EFUN(type, name, arguments) \
    const char *efun_##name##_prototype = #type " " #name #arguments; \
    Value efun_##name(Thread *_thread, Value *__args, ArgNo __n)

#define ADD_EFUN(name) \
    efun_##name, efun_##name##_prototype

// Efun: printf(string format, ...)
DECLARE_EFUN(void, printf, (string format, ...))
{
    return Value();
}

int init_efun_core()
{
    // Efun definitions
    EfunDef core_efuns[] =
    {
        { ADD_EFUN(printf) },
        { 0, 0 }
    };
    Efun::add_efuns("system.core", core_efuns);
    return 0;
}

void shutdown_efun_core()
{
}

}
