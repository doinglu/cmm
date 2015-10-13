// cmm_string_pool.cpp

#include "std_port/std_port_os.h"
#include "cmm_string_pool.h"

namespace cmm
{

StringPool::StringPool()
{
    std_init_spin_lock(&m_lock);
}

StringPool::~StringPool()
{
    for (auto it = m_pool.begin(); it != m_pool.end(); ++it)
        String::delete_string(it->get_string());
    std_destroy_spin_lock(&m_lock);
}
    
// Find string in pool, create new one if not found
// Once a string put in pool, it should be never deleted, so it should be
// a CONSTANT one
String *StringPool::find_or_insert(const StringPtr& string_ptr)
{
    String *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find(string_ptr);
    if (it == m_pool.end())
    {
        // Not found in pool, create new "CONSTANT" string
        string_in_pool = String::new_string(string_ptr.get_string());
        string_in_pool->attrib |= (ReferenceValue::CONSTANT | ReferenceValue::SHARED);
        // Attention: The key (StringPtr) must use the same String * as value
        m_pool.put(StringPtr(string_in_pool));
    } else
        string_in_pool = it->get_string();
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

// Find only
String *StringPool::find(const StringPtr& string_ptr)
{
    String *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find(string_ptr);
    if (it != m_pool.end())
        string_in_pool = it->get_string();
    else
        string_in_pool = 0;
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

}
