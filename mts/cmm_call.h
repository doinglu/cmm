// cmm_call.h

#pragma once

#include "cmm_domain.h"
#include "cmm_object.h"
#include "cmm_program.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

class Object;
class Thread;

// Get member offset of a class
#define MEMBER_OFFSET(m)     ((MemberOffset) (size_t) &(((Self *)0)->m))

// Call function in other component in this object (without parameter)
inline Value call_far(Thread *__thread, ComponentNo __component_no, FunctionNo __function_no, Value *__params = 0, ArgNo __n = 0)
{
    auto *object = __thread->get_this_object();
    auto *program = object->get_program();
    auto mapped_component_no = program->get_mapped_component_no(__thread->get_this_component_no(), __component_no);
    auto *call_component = program->get_component(mapped_component_no);
    auto offset = program->get_component_offset(mapped_component_no);
    auto *function = call_component->get_function(__function_no);

    auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
    Function::Entry func = function->get_func_entry();
    Value ret = (component_impl->*func)(__thread, object, mapped_component_no, __params, __n);
    return ret;
}

// Call function in other component in this object (with parameter)
template<class... Types>
inline Value call_far(Thread *__thread, ComponentNo __component_no, FunctionNo __function_no, Types&&... args)
{
    Value __params[] = { args... };
    ArgNo __n = sizeof(__params) / sizeof(__params[0]);
    __thread->transfer_values_to_current_domain();
    return call_far(__thread, __component_no, __function_no, __params, __n);
}

// Call function in same componenet (without parameter)
template<typename F>
inline Value call_near(Thread *__thread, AbstractComponent *component, F fptr, Value *__params = 0, ArgNo __n = 0)
{
    auto *object = __thread->get_this_object();
    auto component_no = __thread->get_this_component_no();
    Function::Entry func = (Function::Entry) fptr;
    Value ret = (component->*func)(__thread, object, component_no, __params, __n);
    return ret;
};

// Call function in same componenet (with parameter)
template<typename F, class... Types>
inline Value call_near(Thread *__thread, AbstractComponent *component, F fptr, Types&&... args)
{
    Value __params[] = { args... };
    ArgNo __n = sizeof(__params) / sizeof(__params[0]);
    __thread->transfer_values_to_current_domain();
    return call_near(__thread, fptr, __params, __n);
};

// Call function in other object (without parameter)
inline Value call_other(Thread *__thread, ObjectId __oid, const Value& __function_name, Value *__params = 0, ArgNo __n = 0)
{
    STD_ASSERT(__function_name.m_type == Value::STRING);
    auto *entry = Object::get_entry_by_id(__oid);
    auto *program = entry->program;
    if (entry->domain == __thread->get_current_domain())
    {
        STD_ASSERT(("Program shouldn't be null when the object is in same domain.", program));

        // In the same domain, just do normal call
        Program::CalleeInfo callee;
        if (!program->get_callee_by_name((String **)&__function_name.m_string, &callee))
            return Value();

        auto *object = entry->object;
        auto component_no = callee.component_no;
        auto offset = program->get_component_offset(component_no);
        auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
        Function::Entry func = callee.function->get_func_entry();
        return (component_impl->*func)(__thread, object, component_no, __params, __n);
    }

    // Call into other domain
    if (!program)
        return Value();
    Value ret = program->invoke(__thread, __oid, __function_name, __params, __n);
    return ret;
}

// Call function in other object (with parameter)
template<class... Types>
inline Value call_other(Thread *__thread, ObjectId __oid, const Value& __function_name, Types&&... args)
{
    Value __params[] = { args... };
    ArgNo __n = sizeof(__params) / sizeof(__params[0]);
    return call_other(__thread, __oid, __function_name, __params, __n);
}

}