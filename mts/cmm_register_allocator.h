// cmm_register_allocator.h
// Initial version 2011.7.1 by doing
// Immigrated 2015.11.5 by doing

#pragma once

#include "cmm.h"

namespace cmm
{

struct AstFunction;

IntR cmm_alloc_register(AstFunction *func, ValueType type);
IntR cmm_alloc_fixed_register(AstFunction *func, ValueType type);
void cmm_free_register(AstFunction *func, IntR reg_index);
void cmm_free_ret_registers(AstFunction *func);
void cmm_set_register_fixed(AstFunction *func, IntR reg_index, bool fiexed);

}
