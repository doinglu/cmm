// cmm_call.cpp
// Initial version Feb/13/2016 by doing

#include "cmm.h"
#include "cmm_call.h"

namespace cmm
{

// Call function in other component in this object (without parameter)
Value call_far(Thread *thread, ComponentNo component_no, FunctionNo function_no, Value *args, ArgNo n)
{
    auto *object = thread->get_this_object();
    auto *program = object->get_program();
    auto mapped_component_no = program->get_mapped_component_no(thread->get_this_component_no(), component_no);
    auto *call_component = program->get_component(mapped_component_no);
    auto offset = program->get_component_offset(mapped_component_no);
    auto *function = call_component->get_function(function_no);

    auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
    auto func = function->get_script_entry();
    thread->push_call_context(object, function, args, n, mapped_component_no);
    Value ret = (component_impl->*func)(thread, args, n);
    thread->pop_call_context();
    return ret;
}

// Call function in other object (without parameter)
Value call_other(Thread *thread, ObjectId oid, const Value& function_name, Value *args, ArgNo n)
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
            return NIL;

        auto *object = entry->object;
        auto component_no = callee.component_no;
        auto offset = program->get_component_offset(component_no);
        auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
        auto func = callee.function->get_script_entry();
        thread->push_call_context(object, callee.function, args, n, component_no);
        Value ret = (component_impl->*func)(thread, args, n);
        thread->pop_call_context();
        return ret;
    }

    // Call into other domain
    if (!program)
        return NIL;

    Value ret = program->invoke(thread, oid, function_name, args, n);
    return ret;
}

}
