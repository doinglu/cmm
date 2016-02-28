// cmm_call.cpp
// Initial version Feb/13/2016 by doing

#include "cmm.h"
#include "cmm_call.h"

namespace cmm
{

// Call other component
Value& Call::invoke_far_impl(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value* ret, ArgNo n)
{
    auto *object = thread->get_this_object();
    auto *program = object->get_program();
    auto mapped_component_no = program->get_mapped_component_no(thread->get_this_component_no(), component_no);
    auto *call_component = program->get_component(mapped_component_no);
    auto offset = program->get_component_offset(mapped_component_no);
    auto *function = call_component->get_function(function_no);

    auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
    auto func = function->get_script_entry();
    thread->push_call_context(object, function, n, mapped_component_no);
    (component_impl->*func)(thread, n);
    thread->pop_call_context(ret);
    return *ret;
}

// Call other object
Value& Call::invoke_other_impl(Thread *thread, ObjectId oid, const Value& function_name, Value* ret, ArgNo n)
{
    STD_ASSERT(function_name.m_type == ValueType::STRING);
    auto *entry = Object::get_entry_by_id(oid);
    auto *program = entry->program;
    if (entry->domain == thread->get_current_domain())
    {
        STD_ASSERT(("Program shouldn't be null when the object is in same domain.", program));

        // In the same domain, just do normal call
        Program::CalleeInfo callee;
        if (!program->get_public_callee_by_name((String *)&function_name, &callee))
        {
            *ret = NIL;
            return *ret;
        }

        auto *object = entry->object;
        auto component_no = callee.component_no;
        auto offset = program->get_component_offset(component_no);
        auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
        auto func = callee.function->get_script_entry();
        thread->push_call_context(object, callee.function, n, component_no);
        (component_impl->*func)(thread, n);
        thread->pop_call_context(ret);
        return *ret;
    }

    // Call into other domain
    if (!program)
    {
        *ret = NIL;
        return *ret;
    }

    program->invoke(thread, oid, function_name, ret, n);
    return *ret;
}

}
