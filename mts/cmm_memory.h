// cmm_memory.h
// Memory management

#pragma once

#include "std_port/std_port_type.h"

void *operator new(size_t size);
void  operator delete(void *p) throw();

namespace cmm
{

class AutoInitMemoryManager
{
public:
    AutoInitMemoryManager();
    ~AutoInitMemoryManager();
};

} // End of namespace: cmm