// cmm_string_pool.h

#pragma once

#include "std_template/simple_string.h"
#include "std_template/simple_hash_map.h"
#include "cmm_value.h"

namespace cmm
{

class StringPool
{
public:
    StringPool();
    ~StringPool();

public:
    String *find_or_insert(const simple::string& str);
    String *find(const simple::string& str);

private:
    std_spin_lock_t m_lock;
    simple::hash_map<simple::string, String *> m_pool;
};

}
