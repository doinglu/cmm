// cmm_basic_types.cpp

#include <stdio.h>
#include "cmm.h"
#include "cmm_basic_types.h"

namespace cmm
{

void GlobalId::print(char *buf, size_t size, const char *prefix) const
{
    snprintf(buf, size, "%s[%zu:v%zu@%zu]",
             prefix, index(), version, process_id);
    buf[size - 1] = 0;
}

}
