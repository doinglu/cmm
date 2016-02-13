// cmm_init_mmgr.h
// Memory management

#pragma once

#include "std_port/std_port_type.h"
#include "std_memmgr/std_memmgr.h"

namespace cmm
{

class AutoInitMemoryManager
{
public:
    AutoInitMemoryManager();
    ~AutoInitMemoryManager();
};

} // End of namespace: cmm