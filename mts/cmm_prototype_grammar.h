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

typedef simple::vector<simple::string> StringArray;

struct TokenState
{
public:
    TokenState(const simple::string& prototype);

    // Get next token & move cursor
    bool get_token(simple::string* token)
    {
        if (!peek_token(token))
            return false;

        skip();
        return true;
    }

    // Peek next token (cursor won't be changed)
    bool peek_token(simple::string* token)
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
    StringArray words;
    size_t cursor;
    size_t end;

    simple::string error_msg;
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
    simple::string name;

    Argument()
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
    simple::string fun_name;
    Type ret_type;
    ArgumentsList arguments_list;

    Prototype()
    {
    }
};

// Parse text to words array
StringArray parse_words(const simple::string& text);

// Grammar mathcing rules
bool match_prototype(TokenState& state, Prototype *ptr_prototype);
bool match_type(TokenState& state, Type *ptr_type);
bool match_ident_name(TokenState& state, simple::string *ident_name);
bool match_word(TokenState& state, const simple::string& expect_word);
bool match_arguments_list(TokenState& state, ArgumentsList *ptr_arguments);
bool match_argument(TokenState& state, Argument *ptr_argument);
bool match_constant(TokenState& state);

} // End of namespace: PrototypeGrammar

} // End of namespace: cmm
