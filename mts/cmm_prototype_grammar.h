// cmm_prototype_grammar.h

#pragma once

#include "std_template/simple.h"
#include "std_template/simple_vector.h"
#include "cmm.h"
#include "cmm_value.h"

namespace cmm
{

namespace PrototypeGrammar
{

struct TokenState
{
public:
    TokenState(const String& prototype);

    // Get next token & move cursor
    bool get_token(String *token)
    {
        if (!peek_token(token))
            return false;

        skip();
        return true;
    }

    // Peek next token (cursor won't be changed)
    bool peek_token(String *token)
    {
        if (cursor >= end)
            return false;

        *token = words[cursor];
        return true;
    }

    // Move cursor forward
    void skip()
    {
        STD_ASSERT(("Cursor is out of range.", cursor < end));
        cursor++;
    }

public:
    Array words;
    size_t cursor;
    size_t end;

    String error_msg;
};

// Advanced type
struct Type
{
    Type() :
        basic_type(NIL),
        is_nullable(false)
    {
    }

public:
    ValueType basic_type;
    bool is_nullable;
};

// Type argument
struct Argument
{
    Type type;
    bool has_default;
    String name;

    Argument() :
        name(EMPTY_STRING)
    {
    }
};

// Arguments list
struct ArgumentsList
{
    // Use GC allocator for this ////---- More to be added
    simple::vector<Argument> args; // Array of Argument
    bool is_va_arg;

    ArgumentsList() :
        args((size_t)0)
    {
        is_va_arg = false;
    }
};

// Prototype
struct Prototype
{
    String fun_name;
    Type ret_type;
    ArgumentsList arguments_list;

    Prototype() :
        fun_name(EMPTY_STRING)
    {
    }
};

// Parse text to words array
Array parse_words(const String& text);

// Grammar mathcing rules
bool match_prototype(TokenState& state, Prototype *ptr_prototype);
bool match_type(TokenState& state, Type *ptr_type);
bool match_ident_name(TokenState& state, String *ident_name);
bool match_word(TokenState& state, const String& expect_word);
bool match_arguments_list(TokenState& state, ArgumentsList *ptr_arguments);
bool match_argument(TokenState& state, Argument *ptr_argument);
bool match_constant(TokenState& state);

} // End of namespace: PrototypeGrammar

} // End of namespace: cmm
