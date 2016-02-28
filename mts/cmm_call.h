// cmm_call.h

#pragma once

#include <stddef.h>
#include "cmm_domain.h"
#include "cmm_efun.h"
#include "cmm_object.h"
#include "cmm_program.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

// Get member offset of a class
#define MEMBER_OFFSET(m)     ((MemberOffset)offsetof(Self, m))

// Call external function
inline Value call_efun(Thread *thread, const Value& function_name, Value *args, ArgNo n)
{
    Value ret = Efun::invoke(thread, function_name, args, n);
    return ret;
}

template<class... Types>
inline Value call_efun(Thread *thread, const Value& function_name, Types&&... args)
{
    Value params[] = { args... };
    ArgNo n = sizeof(params) / sizeof(params[0]);
    return call_efun(thread, function_name, params, n);
}

// Call function in other component in this object (without parameter)
Value call_far(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value *args = 0, ArgNo n = 0);

// Call function in other component in this object (with parameter)
template<class... Types>
inline Value call_far(Thread *thread, ComponentNo component_no, FunctionNo function_no, Types&&... args)
{
    Value params[] = { args... };
    ArgNo n = sizeof(params) / sizeof(params[0]);
    return call_far(thread, component_no, function_no, params, n);
}

// Call function in same componenet (without parameter)
template<typename F>
inline Value call_near(Thread *thread, AbstractComponent *component, F fptr, Value *args = 0, ArgNo n = 0)
{
    auto func = (Function::ScriptEntry) fptr;
    // Use the function pointer instead of function to optimize
    // the speed of near call
    thread->push_call_context(thread->get_this_object(), *(void **)&func, args, n,
                              thread->get_this_component_no());
    Value ret = (component->*func)(thread, args, n);
    thread->pop_call_context();
    return ret;
};

// Call function in same componenet (with parameter)
template<typename F, class... Types>
inline Value call_near(Thread *thread, AbstractComponent *component, F fptr, Types&&... args)
{
    Value params[] = { args... };
    ArgNo n = sizeof(params) / sizeof(params[0]);
    return call_near(thread, component, fptr, params, n);
};

// Call function in other object (without parameter)
Value call_other(Thread *thread, ObjectId oid, const Value& function_name, Value *args = 0, ArgNo n = 0);

// Call function in other object (with parameter)
template<class... Types>
inline Value call_other(Thread *thread, ObjectId oid, const Value& function_name, Types&&... args)
{
    Value params[] = { args... };
    ArgNo n = sizeof(params) / sizeof(params[0]);
    return call_other(thread, oid, function_name, params, n);
}

}