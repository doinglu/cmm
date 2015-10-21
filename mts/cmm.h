// cmm.h

#pragma once

#include "cmm_basic_types.h"

namespace cmm
{

// Throw error
ANALYZER_NO_RETURN
extern void throw_error(const char *msg, ...);

}
