// cmm_efun.cpp

#include <stdio.h>
#include "std_template/simple_string.h"
#include "cmm_efun.h"
#include "cmm_efun_core.h"
#include "cmm_program.h"
#include "cmm_prototype_grammar.h"
#include "cmm_thread.h"

namespace cmm
{

Efun::EfunPackage *Efun::m_efun_packages = 0;
Efun::EfunMap *Efun::m_efun_map = 0;

bool Efun::init()
{
    m_efun_packages = XNEW(EfunPackage);
    m_efun_map = XNEW(EfunMap);
    
    // Initialize all efun modules
    init_efun_core();
    return true;
}

void Efun::shutdown()
{
    // Shutdown all efun modules
    shutdown_efun_core();

    // Cleanup all efuns packages & maps
    XDELETE(m_efun_packages);
    XDELETE(m_efun_map);
}

// Add an efun
bool Efun::add_efun(Program *program, const String& prefix, EfunEntry entry, const String& prototype_text)
{
    const char *c_str = prefix.c_str();
    const char prefix_system[] = "system.";
    size_t prefix_system_len = strlen(prefix_system);
    bool is_system_package = strncmp(c_str, prefix_system, prefix_system_len) == 0;

    PrototypeGrammar::Prototype prototype;
    if (!parse_efun_prototype(prototype_text, &prototype))
        // Failed to add efun
        return false;

    // Create the function
    auto *function = program->define_function(prototype.fun_name, entry);

    // Set attrib of function
    function->m_attrib = Function::EXTERNAL;
    if (prototype.arguments_list.is_va_arg)
        // Accept random arguments
        function->m_attrib = (Function::Attrib) (function->m_attrib | Function::RANDOM_ARG);
    if (prototype.ret_type.is_nullable)
        // Accept random arguments
        function->m_attrib = (Function::Attrib) (function->m_attrib | Function::RET_NULLABLE);

    // Set return type
    function->m_ret_type = prototype.ret_type.basic_type;
    if (function->m_ret_type == ValueType::TVOID)
        // Use NIL instead of TVOID
        // TVOID is only used for compiling stage
        function->m_ret_type = ValueType::NIL;

    // Add all arguments
    for (auto &it : prototype.arguments_list.args)
    {
        // The parameter's type
        auto attrib = (Parameter::Attrib)0;
        if (it.type.is_nullable)
            attrib = (Parameter::Attrib)(attrib | Parameter::NULLABLE);
        if (it.has_default)
            attrib = (Parameter::Attrib)(attrib | Parameter::DEFAULT);

        function->define_parameter(it.name, it.type.basic_type, attrib);
    }
    function->finish_adding_parameters();

    auto* thread = Thread::get_current_thread();
    String fun_name_with_prefix = NIL;

    // Add name->function to efun map
    // For system package, add two entries, one for short name (without package prefix)
    auto *fun_name = function->get_name();
    fun_name_with_prefix = prefix.m_string->concat(fun_name);
    fun_name_with_prefix = Program::find_or_add_string(fun_name_with_prefix.m_string);
    m_efun_map->put(fun_name_with_prefix.ptr(), function);

    // Add short name for efun in system package
    if (is_system_package)
        m_efun_map->put(fun_name, function);
    return true;
}

// Add multiple efuns
void Efun::add_efuns(const String& package_name, EfunDef *efun_def_array)
{
    // Is package_name lead by "system."?
    String prefix = package_name;
    prefix.concat(".");

    // Create program for this package
    auto *program = XNEW(Program, package_name);
    m_efun_packages->put(program->get_name(), program);

    // Add all efuns
    for (size_t i = 0; efun_def_array[i].entry; ++i)
        add_efun(program, prefix, efun_def_array[i].entry, efun_def_array[i].prototype);
}

// Parse prototype & generate function
// Sample:
// int add(int a, int b)
// void printf(string format, ...)
// mapping mapping_query(mapping a, mixed key = "/")
// Grammar:
// prototype = type function_name(argument_list)
// type = "int" | "real" | "void" | "object" | "string" | "buffer" | "array" | "mapping"
// function_name = {word}
// argument_list = argument ["," argument_list]*
// argument = type ["?"] argument_name [optional_assign]
// argument_name = {word}
// optional_assign = "=" constant
// constant = {json}
bool Efun::parse_efun_prototype(const String& text, PrototypeGrammar::Prototype *ptr_prototype)
{
    PrototypeGrammar::TokenState state(text.c_str());
    PrototypeGrammar::Prototype prototype;

    if (!PrototypeGrammar::match_prototype(state, &prototype))
    {
        // Failed to match
        STD_TRACE("Failed to parse efun prototype: %s\n%s",
                  text.c_str(), state.error_msg.c_str());
        return false;
    }

    *ptr_prototype = simple::move(prototype);
    return true;
}

// Invoke an efun by name
// See ATTENTION of Program::invoke
Value Efun::invoke(Thread *thread, const Value& function_name, Value *args, ArgNo n)
{
    if (function_name.m_type != ValueType::STRING)
    {
        // Bad type of function name
        return NIL;
    }

    if (!Program::convert_to_shared((StringImpl**)&function_name.m_string))
    {
        // Not found name in shared string pool, no such efun
        return NIL;
    }

    Function *function;
    if (!m_efun_map->try_get(simple::raw_type(function_name.m_string), &function))
    {
        // Function is not found
        return NIL;
    }

    auto func_entry = function->get_efun_entry();
    thread->push_call_context(thread->get_this_object(), (void *)func_entry, args, n,
                              thread->get_this_component_no());
    Value ret = func_entry(thread, args, n);
    thread->pop_call_context();
    STD_ASSERT(("Bad return type of efun function.",
                (ret.m_type == function->m_ret_type) ||
                (ret.m_type == ValueType::NIL && (function->m_attrib & Function::Attrib::RET_NULLABLE))));
    return ret;
}

// Get an efun by name
// See ATTENTION of Program::invoke
Function* Efun::get_efun(const Value& function_name)
{
    if (function_name.m_type != ValueType::STRING)
        // Bad type of function name
        return NULL;

    if (!Program::convert_to_shared((StringImpl**)&function_name.m_string))
        // Not found name in shared string pool, no such efun
        return NULL;

    Function *function;
    if (!m_efun_map->try_get(simple::raw_type(function_name.m_string), &function))
        // Function is not found
        return NULL;

    return function;
}

} // End of namespace: cmm
