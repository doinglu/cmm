// cmm_efun.cpp

#include <stdio.h>
#include "std_template/simple_string.h"
#include "cmm_efun.h"
#include "cmm_efun_core.h"
#include "cmm_operate.h"
#include "cmm_program.h"
#include "cmm_prototype_grammar.h"

namespace cmm
{

Efun::EfunMap *Efun::m_efun_map = 0;

int Efun::init()
{
    m_efun_map = new Efun::EfunMap();
    
    // Initialize all efun modules
    init_efun_core();
    return 0;
}

void Efun::shutdown()
{
    // Shutdown all efun modules
    shutdown_efun_core();

    delete m_efun_map;
}

// Add an efun
bool Efun::add_efun(const String& prefix, EfunEntry entry, const String& prototype_text)
{
    bool is_system_package = (prefix.sub_string(0, 7) == "system.");

    Function *function;
    if (parse_efun_prototype(prototype_text, &function))
        // Failed to add efun
        return false;

#if 0
    String fun_name(function->get_name());
    fun_name = Program::find_or_add_string(fun_name);
    auto *function = new Function(0, fun_name);
    String fun_name_with_prefix = prefix + function->get_name();
    fun_name_with_prefix = Program::find_or_add_string(fun_name_with_prefix);
    m_efun_map->put(fun_name, function);
    m_efun_map->put(fun_name_with_prefix, function);
#endif
    return true;
}

// Add multiple efuns
void Efun::add_efuns(const String& package_name, EfunDef *efun_def_array)
{
    // Is package_name lead by "system."?
    String prefix = package_name + ".";

    for (size_t i = 0; efun_def_array[i].entry; ++i)
        add_efun(prefix, efun_def_array[i].entry, efun_def_array[i].prototype);
}

// Parse prototype & generate function
// Sample:
// int add(int a, int b)
// void printf(string format, ...)
// mapping mapping_query(mapping a, mixed key = "/")
// Grammar:
// prototype = type function_name(argument_list)
// type = "int" | "real" | "object" | "string" | "buffer" | "array" | "mapping"
// function_name = {word}
// argument_list = argument ["," argument_list]*
// argument = type ["?"] argument_name [optional_assign]
// argument_name = {word}
// optional_assign = "=" constant
// constant = {json}
bool Efun::parse_efun_prototype(const String& text, Function **ptr_function)
{
    PrototypeGrammar::TokenState state(text);
    PrototypeGrammar::Prototype prototype;
    
    if (!PrototypeGrammar::match_prototype(state, &prototype))
    {
        // Failed to match
        STD_TRACE("Failed to parse efun prototype: %s\n%s",
                  text.c_str(), state.error_msg.c_str());
        return false;
    }

    printf("Function name:%s\n", prototype.fun_name.c_str());////----
    return true;
}

} // End of namespace: cmm
