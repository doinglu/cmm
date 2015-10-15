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
    StringImpl *find_or_insert(const StringPtr& string_ptr);
    StringImpl *find(const StringPtr& string_ptr);

private:
    std_spin_lock_t m_lock;

    // Use StringImpl::hash_func to compare content of StringImpl *
    typedef simple::hash_set<StringPtr, StringPtr::hash_func> Container;
    Container m_pool;
};

}
