// cmm_error.h

#pragma once

#include "cmm_basic_types.h"

namespace cmm
{

class Variables;
class Value;

// All error codes
enum ErrorCode
{
    OK                          = 0,
    UNKNOWN_ERROR               = -1,
    CAN_NOT_CREATE_MUTEX        = -2,
    CAN_NOT_GET_MUTEX           = -3,
    CAN_NOT_INIT                = -4,
    FAILED_TO_EXECUTE_SCRIPT    = -5,
    CONFIG_WHEN_ALREADY_INIT    = -6,
    BAD_CONFIGURATION           = -7,
    CAN_NOT_OPEN_INPUT          = -101,
    CAN_NOT_OPEN_OUTPUT         = -102,
    CAN_NOT_ACCESS_INTERNAL     = -103,
    CAN_NOT_ACCESS_COMPILER     = -104,
    CAN_NOT_BUILD_SCRATCHPAD    = -105,
    RUNNING_IN_MAIN_LOOP        = -106,
    NOT_ENOUGH_MEMORY           = -107,
    IN_SCHEDULER_TASK           = -108,
    RUNNING_IN_SCHEDULER        = -109,
    PATH_NAME_TOO_LONG          = -201,
    PATH_OUT_OF_ROOT            = -202,
    FILE_NAME_WAS_OUTPUT_FILE   = -203,
    CAN_NOT_CREATE_DIRECTORY    = -204,
    NO_RUNNING_PROCESS          = -301,
    CAN_NOT_PREPARE_TO_RUN      = -302,
    CAN_NOT_CREATE_PROCESS      = -303,
    CAN_NOT_START_PROCESS       = -304,
    NO_SUCH_PROCESS             = -305,
    CALLSTACK_OVERFLOW          = -306,
    STACK_OVERFLOW              = -307,
    CAN_NOT_CREATE_DEBUG_CONTEXT= -308,
    ERROR_OCCURED_OF_PROCESS    = -309,
    NO_SUCH_FUNCTION            = -401,
    CAN_NOT_CREATE_SEMAPHORE    = -402,
    CAN_NOT_CREATE_VALUE        = -403,
    BAD_TYPE_OF_ARGUMENT        = -404,
    BAD_VALUE_OF_ARGUMENT       = -405,
    FAILED_TO_BUILD_ARGUMENT    = -406,
    MOUNT_POINT_EXISTED         = -501,
    BAD_MOUNT_POINT             = -502,
    NOT_FOUND_MOUNT_POINT       = -503,
    ASSEMBLE_FAILED             = -504,
    COMPILE_ERROR               = -601,
    PASS1_ERROR                 = -603,
    PASS2_ERROR                 = -604,

    // Compiling error codes (with prefix C_)
    C_PARSER                    = 1999,
    C_REDEFINITION              = 2051,
    C_UNDECLARED_IDENTIFER      = 2065,
    C_NOT_LVALUE                = 2106,
    C_NOT_CONTAINER             = 2109,
    C_CANNOT_OPER               = 2110,
    C_CAST_TO_NON_CONST         = 2900,
    C_ASSIGN_TO_CONST           = 3892,

    // Compiling warning codes (with prefix C_)
    C_CAST_USELESS_QMARK        = 3001,
};

// Print variables of function
void print_variables(const Variables& variables, const char *type, Value *arr, ArgNo n);

// Throw error
ANALYZER_NO_RETURN
extern void throw_error(const char *msg, ...);

}
