// cmm_error.h

#pragma once

#include "cmm_basic_types.h"

namespace cmm
{

class Variables;
class Value;

// Print variables of function
void print_variables(const Variables& variables, const char *type, Value *arr, ArgNo n);

// Throw error
ANALYZER_NO_RETURN
extern void throw_error(const char *msg, ...);

}
