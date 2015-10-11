// cmm_value_list.h

#pragma once

#include "std_template/simple_hash_set.h"

namespace cmm
{

struct ReferenceValue;

// Values list for thread/domain, using by GC
class ValueList
{
public:
    ValueList()
    {
        reset();
    }

public:
    // Bind value to list
    void append_value(ReferenceValue *value);

    // Concat two lists
    void concat_list(ValueList *list);

    // Free all linked values in list
    void free();

    // Return the count of total values
    size_t get_count() { return m_count; }

    // Return the head of list
    ReferenceValue *get_list() { return m_list; }

    // Reset the list (don't free linked values)
    void reset()
    {
        m_list = 0;
        m_pp_last = &m_list;
        m_count = 0;
    }

private:
    // List of all reference values
    ReferenceValue *m_list;
    ReferenceValue **m_pp_last;
    size_t m_count;
};

// Strcuture using by GC
struct MarkValueState
{
public:
    simple::hash_set<struct ReferenceValue *> set;
    ValueList *list;

public:
    MarkValueState(ValueList *_list) :
        set(_list->get_count())
    {
        list = _list;
    }
};

} // End of namespace: cmm
