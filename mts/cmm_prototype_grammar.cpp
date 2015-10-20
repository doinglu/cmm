// cmm_prototype_grammar.cpp

#include <stdio.h>
#include "cmm_prototype_grammar.h"

namespace cmm
{

namespace PrototypeGrammar
{

// Grammar match
bool match_prototype(TokenState& state, Prototype *ptr_prototype)
{
    Type ret_type;
    if (! match_type(state, &ret_type))
    {
        state.error_msg += "Expected return type for prototype.\n";
        return false;
    }

    String fun_name;
    if (!match_ident_name(state, &fun_name))
    {
        state.error_msg += "Expected function name.\n";
        return false;
    }

    if (!match_word(state, "("))
    {
        state.error_msg += "Missed ( after function name.\n";
        return false;
    }

    ArgumentsList argument_list;
    if (!match_arguments_list(state, &argument_list))
        return false;

    // Skip ;
    String token;
    if (state.peek_token(&token) && token == ";")
        state.skip();

    if (state.peek_token(&token))
    {
        state.error_msg += String::snprintf("There are some words(\"%s\") following prototype.", 256,
                                            token.c_str());
        return false;
    }

    ptr_prototype->fun_name = simple::move(fun_name);
    ptr_prototype->ret_type = simple::move(ret_type);
    ptr_prototype->arguments_list = simple::move(argument_list);
    return true;
}

// Grammar match: type = int | real | .... | mixed [?]
bool match_type(TokenState& state, Type *ptr_type)
{
    String token;
    if (!state.get_token(&token))
    {
        state.error_msg += "End of line, missed type.\n";
        return false;
    }

    // Get the type
    auto basic_type = Value::name_to_type(token.c_str());
    if (basic_type == NIL)
    {
        state.error_msg += "Type can not be nil.\n";
        return false;
    }
    if (basic_type == BAD_TYPE)
    {
        state.error_msg += "Bad type.\n";
        return false;
    }

    bool is_nullable = false;
    if (state.peek_token(&token) && token == "?")
    {
        is_nullable = true;
        state.skip();
    }

    ptr_type->basic_type = simple::move(basic_type);
    ptr_type->is_nullable = simple::move(is_nullable);
    return true;
}

// Grammar match: [_,a-z,A-Z][_,a-z,A-Z,1-9]*
bool match_ident_name(TokenState& state, String *ident_name)
{
    String word;
    if (!state.get_token(&word))
    {
        state.error_msg += "Missed ident name.\n";
        return false;
    }
    if (!isalnum(word[0]) && word[0] != '_')
    {
        state.error_msg += String::snprintf("Bad ident name \"%s\", expected lead by [_,a-z,A-Z].\n", 256,
                                            word.c_str());
        return false;
    }

    *ident_name = simple::move(word);
    return true;
}

// Grammar match: == word
bool match_word(TokenState& state, const String& expect_word)
{
    String word;
    if (!state.get_token(&word))
        return false;

    if (word != expect_word)
        return false;

    return true;
}

// Grammar match: arguments_list = type argument [= const_value] [,arguments_list]
bool match_arguments_list(TokenState& state, ArgumentsList *ptr_arguments)
{
    ArgumentsList argument_list;

    String token;
    while (state.peek_token(&token) && token != ")")
    {
        if (token == "...")
        {
            // Matched ...
            state.skip();
            argument_list.is_va_arg = true;
            break;
        }

        // Get next argument
        Argument argument;
        if (!match_argument(state, &argument))
            return false;
        argument_list.args.push_back(simple::move(argument));

        if (state.peek_token(&token) && token == ",")
        {
            // Continue to match
            state.skip();
            continue;
        }
    }

    if (!match_word(state, ")"))
    {
        state.error_msg += "Missed ) for prototype.\n";
        return false;
    }

    *ptr_arguments = simple::move(argument_list);
    return true;
}

// Grammar match: argument = type argument [= const_value]
bool match_argument(TokenState& state, Argument *ptr_argument)
{
    Type type;
    if (!match_type(state, &type))
    {
        state.error_msg += "Expect type for argument.\n";
        return false;
    }

    String name;
    if (!match_ident_name(state, &name))
    {
        state.error_msg += "Expect name for argument.\n";
        return false;
    }

    bool has_default = false;
    String token;
    if (state.peek_token(&token) && token == "=")
    {
        if (!match_constant(state))
        {
            state.error_msg += "Expect constant as default value of argument.\n";
            return false;
        }
    }

    ptr_argument->type = type;
    ptr_argument->has_default = has_default;
    ptr_argument->name = name;
    return true;
}

// Grammar: constant = json
bool match_constant(TokenState& state)
{
    // More to be added
    return false;
}

TokenState::TokenState(const String& prototype) :
    words(parse_words(prototype))
{
    cursor = 0;
    end = words.size();

    for (auto it = words.begin(); it != words.end(); ++it)
        printf("%zd. %s\n", it.get_index(), it->as_string().m_string->c_str());////----
}

// Parse string text to words
// Seperator by punctuation, space
Array parse_words(const String& text)
{
    const unsigned char *p = (const unsigned char *) text.c_str();
    Array arr(text.length() / 4);
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
        size_t b = i;
        if (isalpha(p[i]) || p[i] == '_')
        {
            while (p[i] && (isalnum(p[i]) || p[i] == '_'))
                i++;
            arr.push_back(String((const char *)p + b, i - b));
            continue;
        }

        // Lead by number, looking number.number
        if (isdigit(p[i]))
        {
            bool has_dot = false;
            while (isdigit(p[i]) || (p[i] == '.' && !has_dot))
            {
                if (p[i] == '.')
                    has_dot = true;
                i++;
            }
            arr.push_back(String((const char *)p + b, i - b));
            continue;
        }

        // Lead by .
        if (p[i] == '.')
        {
            if (p[i + 1] == '.' && p[i + 2] == '.')
            {
                // Got ...
                arr.push_back("...");
                i += 3;
                continue;
            }

            if (p[i + 1] == '.')
            {
                // Got ..
                arr.push_back("..");
                i += 2;
                continue;
            }

            if (isdigit(p[i]))
            {
                // Got .number
                while (isdigit(p[i]))
                    i++;
                arr.push_back(String((const char *)p + b, i - b));
                continue;
            }

            // Got .
            arr.push_back(".");
            i++;
            continue;
        }

        // Got single char
        arr.push_back(String((const char *)p + i, 1));
        i++;
    }
    return arr;
}

} // End of namespace: PrototypeGrammar

} // End of namespace: cmm
