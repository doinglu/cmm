// cmm_string_pool.cpp

#include "std_port/std_port_os.h"
#include "cmm_string_pool.h"

namespace cmm
{

// Wrapper class for find
class CStrKey
{
public:
    CStrKey(const char* c_str, size_t len) :
        m_c_str(c_str),
        m_len(len)
    {
        m_hash_cache = simple::string::hash_string(m_c_str, m_len) + 1;
    }

    size_t hash_value() const
    {
        return m_hash_cache;
    }

    bool equals(const MMMString& str) const
    {
        return str.m_string->len == m_len &&
               memcmp(str.m_string->c_str(), m_c_str, m_len) == 0;
    }

public:
    const char* m_c_str;
    size_t m_len;
    size_t m_hash_cache;
};

StringPool::StringPool()
{
    std_init_spin_lock(&m_lock);
}

StringPool::~StringPool()
{
    for (auto& it: m_pool)
        XDELETE(it.m_string);
    std_destroy_spin_lock(&m_lock);
}
    
// Find string in pool, create new one if not found
// Once a string put in pool, it should be never deleted, so it should be
// a CONSTANT value & should not be belogned to any GC domain
StringImpl *StringPool::find_or_insert(const String& str)
{
    return find_or_insert(str.m_string);
}

// Find string in pool, create new one if not found
// Once a string put in pool, it should be never deleted, so it should be
// a CONSTANT value & should not be belogned to any GC domain
StringImpl *StringPool::find_or_insert(StringImpl* const str_impl)
{
    StringImpl *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find(str_impl);
    if (it == m_pool.end())
    {
        // Not found in pool, create new "CONSTANT" string
        string_in_pool = STRING_ALLOC(str_impl);
        string_in_pool->attrib |= (ReferenceImpl::CONSTANT | ReferenceImpl::SHARED);
        // Attention: The key (String) must use the same StringImpl * as value
        m_pool.put(string_in_pool);
    }
    else
        string_in_pool = it->ptr();
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

// Find string in pool, create new one if not found
// Once a string put in pool, it should be never deleted, so it should be
// a CONSTANT value & should not be belogned to any GC domain
StringImpl *StringPool::find_or_insert(const char* c_str, size_t len)
{
    CStrKey key(c_str, len);
    StringImpl *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find_ex(key);
    if (it == m_pool.end())
    {
        // Not found in pool, create new "CONSTANT" string
        string_in_pool = STRING_ALLOC(c_str, len);
        string_in_pool->attrib |= (ReferenceImpl::CONSTANT | ReferenceImpl::SHARED);
        // Attention: The key (String) must use the same StringImpl * as value
        m_pool.put(string_in_pool);
    }
    else
        string_in_pool = it->ptr();
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

// Find & return the string in pool
StringImpl *StringPool::find(const String& str)
{
    return find(str.m_string);
}

// Find & return the string in pool
StringImpl *StringPool::find(StringImpl* const str_impl)
{
    StringImpl *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find(str_impl);
    if (it != m_pool.end())
        string_in_pool = it->ptr();
    else
        string_in_pool = 0;
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

// Find & return the string in pool
StringImpl *StringPool::find(const char* c_str, size_t len)
{
    CStrKey key(c_str, len);
    StringImpl *string_in_pool;

    std_get_spin_lock(&m_lock);
    auto it = m_pool.find_ex(key);
    if (it != m_pool.end())
        string_in_pool = STRING_ALLOC(c_str, len);
    else
        string_in_pool = 0;
    std_release_spin_lock(&m_lock);

    return string_in_pool;
}

}
