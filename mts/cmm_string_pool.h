// cmm_string_pool.h

#pragma once

#include "std_template/simple_string.h"
#include "std_template/simple_hash_set.h"
#include "cmm_string_ptr.h"
#include "cmm_value.h"

namespace cmm
{

class StringPool
{
public:
    StringPool();
    ~StringPool();

public:
    String *find_or_insert(const StringPtr& string_ptr);
    String *find(const StringPtr& string_ptr);

private:
    std_spin_lock_t m_lock;

    typedef simple::hash_set<StringPtr, hash_string_ptr_func> Container;
    Container m_pool;
};

}
