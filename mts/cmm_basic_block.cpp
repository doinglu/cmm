// cmm_basic_block.cpp
// Initial version by doing Mar/4/2016

#include "cmm_basic_block.h"

namespace cmm
{

// Print
const char* var_storage_to_c_str(VarStorage storage)
{
    switch (storage)
    {
        case VAR_NONE:        return "*";
        case VAR_ARGUMENT:    return "A";
        case VAR_OBJECT_VAR:  return "M";
        case VAR_LOCAL_VAR:   return "L";
        case VAR_VIRTUAL_REG: return "R";
        default:
            STD_ASSERT(0);
            return "Error";
    }
}

}