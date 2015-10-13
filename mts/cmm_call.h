// cmm_call.h

#pragma once

#include <stddef.h>
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
#define MEMBER_OFFSET(m)     ((MemberOffset)offsetof(Self, m))

// Call function in other component in this object (without parameter)
inline Value call_far(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value *params = 0, ArgNo n = 0)
{
    auto *object = thread->get_this_object();
    auto *program = object->get_program();
    auto mapped_component_no = program->get_mapped_component_no(thread->get_this_component_no(), component_no);
    auto *call_component = program->get_component(mapped_component_no);
    auto offset = program->get_component_offset(mapped_component_no);
    auto *function = call_component->get_function(function_no);

    auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
    Function::Entry func = function->get_func_entry();
    auto *context = &thread->get_this_context()->value;
    auto prev_component_no = context->m_this_component;
    context->m_this_component = component_no;
    Value ret = (component_impl->*func)(thread, params, n);
    context->m_this_component = prev_component_no;
    return ret;
}

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
inline Value call_near(Thread *thread, AbstractComponent *component, F fptr, Value *params = 0, ArgNo n = 0)
{
    auto *object = thread->get_this_object();
    auto component_no = thread->get_this_component_no();
    Function::Entry func = (Function::Entry) fptr;
    Value ret = (component->*func)(thread, params, n);
    return ret;
};

// Call function in same componenet (with parameter)
template<typename F, class... Types>
inline Value call_near(Thread *thread, AbstractComponent *component, F fptr, Types&&... args)
{
    Value params[] = { args... };
    ArgNo n = sizeof(params) / sizeof(params[0]);
    return call_near(thread, fptr, params, n);
};

// Call function in other object (without parameter)
inline Value call_other(Thread *thread, ObjectId oid, const Value& function_name, Value *params = 0, ArgNo n = 0)
{
    STD_ASSERT(function_name.m_type == Value::STRING);
    auto *entry = Object::get_entry_by_id(oid);
    auto *program = entry->program;
    if (entry->domain == thread->get_current_domain())
    {
        STD_ASSERT(("Program shouldn't be null when the object is in same domain.", program));

        // In the same domain, just do normal call
        Program::CalleeInfo callee;
        if (!program->get_public_callee_by_name((String **)&function_name.m_string, &callee))
            return Value();

        auto *object = entry->object;
        auto component_no = callee.component_no;
        auto offset = program->get_component_offset(component_no);
        auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
        Function::Entry func = callee.function->get_func_entry();
        auto *context = &thread->get_this_context()->value;
        auto *prev_object = context->m_this_object;
        auto prev_component_no = context->m_this_component;
        context->m_this_object = object;
        context->m_this_component = component_no;
        Value ret = (component_impl->*func)(thread, params, n);
        context->m_this_component = prev_component_no;
        context->m_this_object = prev_object;
        return ret;
    }

    // Call into other domain
    if (!program)
        return Value();
    Value ret = program->invoke(thread, oid, function_name, params, n);
    return ret;
}

// Call function in other object (with parameter)
template<class... Types>
inline Value call_other(Thread *thread, ObjectId oid, const Value& function_name, Types&&... args)
{
    Value params[] = { args... };
    ArgNo n = sizeof(params) / sizeof(params[0]);
    return call_other(thread, oid, function_name, params, n);
}

}