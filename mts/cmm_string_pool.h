// cmm_string_pool.h

#pragma once

#include "std_template/simple_string.h"
#include "std_template/simple_hash_set.h"
#include "cmm_mmm_value.h"

namespace cmm
{

class StringPool
{
public:
    StringPool();
    ~StringPool();

public:
    StringImpl *find_or_insert(const String& str);
    StringImpl *find_or_insert(StringImpl* const str_impl);
    StringImpl *find_or_insert(const char* c_str, size_t len);
    StringImpl *find(const String& str);
    StringImpl *find(StringImpl* const str_impl);
    StringImpl *find(const char* c_str, size_t len);

private:
    std_spin_lock_t m_lock;

    // Use StringImpl::hash_func to compare content of StringImpl *
    typedef simple::hash_set<MMMString, String::hash_func> Container;
    Container m_pool;
};

}
