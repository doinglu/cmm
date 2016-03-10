// cmm_call.h

#pragma once

#include <stdarg.h>
#include <stddef.h>
#include "cmm_domain.h"
#include "cmm_efun.h"
#include "cmm_object.h"
#include "cmm_program.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

#define call_efun       Call::invoke_efun
#define call_near       Call::invoke_near
#define call_far        Call::invoke_far
#define call_other      Call::invoke_other

class Call
{

public:

// Call external function
static inline Value& invoke_efun(Thread *thread, const Value& function_name, Value* ret)
{
    Efun::invoke(thread, function_name, ret, 0);
    return *ret;
}

template<class... Types>
static inline Value& invoke_efun(Thread *thread, const Value& function_name, Value* ret, Types&&... args)
{
    ArgNo n = sizeof...(args);
    Value* ptr_args = thread->reserve_stack_without_init(n);
    push_args(ptr_args, args...);

    Efun::invoke(thread, function_name, ret, n);
    return *ret;
}

// Call function in other component
static Value& invoke_far_impl(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value* ret, ArgNo n);

// Call function in other component in this object (without parameter)
static inline Value& invoke_far(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value* ret)
{
    invoke_far_impl(thread, component_no, function_no, ret, 0);
    return *ret;
}

// Call function in other component in this object (with parameter)
template<class... Types>
static inline Value& invoke_far(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value* ret, Types&&... args)
{
    ArgNo n = sizeof...(args);
    Value* ptr_args = thread->reserve_stack_without_init(n);
    push_args(ptr_args, args...);

    invoke_far_impl(thread, component_no, function_no, ret, n);
    return *ret;
}

// Call function in same componenet (without parameter)
template<typename F>
static inline Value& invoke_near(Thread *thread, AbstractComponent *component, F fptr, Value* ret)
{
    auto func = (Function::ScriptEntry) fptr;
    // Use the function pointer instead of function to optimize
    // the speed of near call
    thread->push_call_context(thread->get_this_object(), *(void **)&func, 0,
                              thread->get_this_component_no());
    (component->*func)(thread, 0);
    thread->pop_call_context(ret);
    return *ret;
};

// Call function in same componenet (with parameter)
template<typename F, class... Types>
static inline Value& invoke_near(Thread *thread, AbstractComponent *component, F fptr, Value* ret, Types&&... args)
{
    ArgNo n = sizeof...(args);
    Value* ptr_args = thread->reserve_stack_without_init(n);
    push_args(ptr_args, args...);

    auto func = (Function::ScriptEntry) fptr;
    // Use the function pointer instead of function to optimize
    // the speed of near call
    thread->push_call_context(thread->get_this_object(), *(void **)&func, n,
                              thread->get_this_component_no());
    (component->*func)(thread, n);
    thread->pop_call_context(ret);
    return *ret;
};

// Call function in other object (without parameter)
static Value& invoke_other_impl(Thread *thread, ObjectId oid, const Value& function_name, Value* ret, ArgNo n);

static inline Value& invoke_other(Thread *thread, ObjectId oid, const Value& function_name, Value* ret)
{
    invoke_other_impl(thread, oid, function_name, ret, 0);
    return *ret;
}

// Call function in other object (with parameter)
template<class... Types>
static inline Value& invoke_other(Thread *thread, ObjectId oid, const Value& function_name, Value* ret, Types&&... args)
{
    ArgNo n = sizeof...(args);
    Value* ptr_args = thread->reserve_stack_without_init(n);
    push_args(ptr_args, args...);

    invoke_other_impl(thread, oid, function_name, ret, n);
    return *ret;
}

// Push arguments
template<typename T1>
static inline void push_args(Value* ptr_values, T1 arg1)
{
    ptr_values[0] = arg1;
}

template<typename T1, typename T2>
static inline void push_args(Value* ptr_values, T1 arg1, T2 arg2)
{
    ptr_values[0] = arg1;
    ptr_values[1] = arg2;
}

template<typename T1, typename T2, typename T3>
static inline void push_args(Value* ptr_values, T1 arg1, T2 arg2, T3 arg3)
{
    ptr_values[0] = arg1;
    ptr_values[1] = arg2;
    ptr_values[2] = arg3;
}

template<typename T1, typename T2, typename T3, typename T4>
static inline void push_args(Value* ptr_values, T1 arg1, T2 arg2, T3 arg3, T4 arg4)
{
    ptr_values[0] = arg1;
    ptr_values[1] = arg2;
    ptr_values[2] = arg3;
    ptr_values[3] = arg4;
}

template<typename T1, typename T2, typename T3, typename T4, typename T5>
static inline void push_args(Value* ptr_values, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5)
{
    ptr_values[0] = arg1;
    ptr_values[1] = arg2;
    ptr_values[2] = arg3;
    ptr_values[3] = arg4;
    ptr_values[4] = arg5;
}

template<typename T1, typename T2, typename T3, typename T4, typename T5, typename T6>
static inline void push_args(Value* ptr_values, T1 arg1, T2 arg2, T3 arg3, T4 arg4, T5 arg5, T6 arg6)
{
    ptr_values[0] = arg1;
    ptr_values[1] = arg2;
    ptr_values[2] = arg3;
    ptr_values[3] = arg4;
    ptr_values[4] = arg5;
    ptr_values[5] = arg6;
}
};

}