// cmm_register_allocator.cpp
// Initial version 2011.7.1 by doing
// Immigrated 2015.11.5 by doing

#include "cmm_lang.h"
#include "cmm_register_allocator.h"

namespace cmm
{

/* allocate a register for a function */
IntR cmm_alloc_register(AstFunction *func, ValueType type)
{
    STD_ASSERT(func);

    /* try to find a free register */
    for (auto& it : func->registers)
    {
        if (! it.is_using && it.type == type)
        {
            it.is_using = true;
            it.is_fixed_reg = false;
            return it.index;
        }
    }

    /* no free register, allocate a new one */
    AstRegister new_reg;
    new_reg.index = (Uint16)func->registers.size();
    new_reg.type = type;
    new_reg.is_using = true;
    new_reg.is_fixed_reg = false;
    func->registers.push_back(new_reg);
    return new_reg.index;
}

/* allocate a fixed register for a function, fixed register won't be auto freed */
IntR cmm_alloc_fixed_register(AstFunction *func, ValueType type)
{
    STD_ASSERT(func);

    /* try to find a free register */
    for (auto& it : func->registers)
    {
        if (! it.is_using && it.type == type)
        {
            it.is_using = true;
            it.is_fixed_reg = true;
            return it.index;
        }
    }

    /* no free register, allocate a new one */
    AstRegister new_reg;
    new_reg.index = (Uint16)func->registers.size();
    new_reg.type = type;
    new_reg.is_using = true;
    new_reg.is_fixed_reg = true;
    func->registers.push_back(new_reg);
    return new_reg.index;
}

/* Make a register fixed */
void cmm_set_register_fixed(AstFunction *func, IntR reg_index, bool fixed)
{
    STD_ASSERT(func);
    STD_ASSERT(reg_index < (IntR)func->registers.size());

    func->registers[reg_index].is_fixed_reg = fixed;
}

/* free a register for a function */
void cmm_free_register(AstFunction *func, IntR reg_index)
{
    STD_ASSERT(func);
    STD_ASSERT(reg_index < (IntR)func->registers.size());

    func->registers[reg_index].is_using = false;
}

/* Free currently using return value registers */
void cmm_free_ret_registers(AstFunction *func)
{
    STD_ASSERT(func);

    /* make all return value registers free */
    for (auto& it : func->registers)
    {
        if (!it.is_fixed_reg)
            it.is_using = false;
    }
}

}
