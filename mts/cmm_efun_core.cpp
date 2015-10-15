// cmm_efun_core.cpp

#include "std_port/std_port.h"
#include "cmm_efun.h"
#include "cmm_value.h"

namespace cmm
{

// Efun definitions
EfunDef core_efuns[] =
{
//    { efun_core_printf, "void printf(string format, ...)" },
    { 0, 0 }
};

void init_efun_core()
{
    Efun::add_efuns("system.core", core_efuns);
}

}
