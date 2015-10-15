// cmm_efun.cpp

#include "std_template/simple_string.h"
#include "cmm_efun.h"
#include "cmm_operate.h"
#include "cmm_program.h"

namespace cmm
{

Efun::EfunMap *Efun::m_efun_map = 0;

int Efun::init()
{
    m_efun_map = new Efun::EfunMap();
    return 0;
}

void Efun::shutdown()
{
    delete m_efun_map;
}

// Add multiple efuns
void Efun::add_efuns(const Value& package_name_value, EfunDef *efun_def_array)
{
    // Is package_name_value lead by "system."?
    STD_ASSERT(("Bad type of package_name_value, expected STRING.",
                package_name_value.m_type == STRING));
    StringPtr prefix = StringPtr(package_name_value) + ".";
    bool is_system_package = (prefix.sub_string(0, 7) == "system.");

    for (size_t i = 0; efun_def_array[i].entry; ++i)
    {
        auto &def = efun_def_array[i];
        StringPtr fun_name(def.name);
        fun_name = Program::find_or_add_string(fun_name);
        auto *function = new Function(0, fun_name);
        parse_efun_prototype(function, def.prototype);
        StringPtr fun_name_with_prefix = prefix + function->get_name();
        fun_name_with_prefix = Program::find_or_add_string(fun_name_with_prefix);
        m_efun_map->put(fun_name, function);
        m_efun_map->put(fun_name_with_prefix, function);
    }
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
void Efun::parse_efun_prototype(Function *function, const Value& prototype)
{
    
}

// Parse string text to words
// Seperator by punctuation, space
ArrayPtr Efun::parse_words(StringPtr& text)
{
    const unsigned char *p = (const unsigned char *) text.c_str();
    ArrayPtr arr(text.length() / 4);
    size_t i = 0;
    while (i < text.length())
    {
        // Skip all spaces
        while (p[i] && isspace(p[i]))
            i++;

        if (!p[i])
            // End of parse
            break;

        // Lead by _ or alphabet
        if (isalpha(p[i]) || p[i] == '_')
        {
            size_t b = i;
            while (p[i] && (isalnum(p[i]) || p[i] == '_'))
                i++;
            arr.append(StringPtr((const char *)p + b, i - b));
        }
        i++;
    }
    return ArrayPtr();
}

}
