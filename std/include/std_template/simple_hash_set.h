// simple_set.h

#pragma once

#include "std_port/std_port_util.h"
#include "simple_vector.h"
#include "simple_string.h"
#include "simple_hash_base.h"
#include <string.h>

namespace simple
{

template <typename T>
struct peek_self
{
    const T& operator()(const T& element) const { return element; }
};

// hash set
template <typename K, typename F = hash_func<K> >
class hash_set : public hash_base<K, K, peek_self<K>, F>
{
    typedef hash_base<K, K, peek_self<K>, F> base_type;

public:
    hash_set(size_t capacity = base_type::MinCapacity, Allocator* allocator = &Allocator::g) :
        base_type(capacity, allocator)
    {
    }

    // Generate vector of keys
    vector<K> to_array() const
    {
        vector<K> vec(base_type::m_size);
        // Lookup entire table to add all keys
        for (size_t i = 0; i < base_type::m_size; i++)
            vec.push_back(base_type::m_elements[i]);
        return vec;
    }

    // Is this hash map contains the key?
    bool contains(const K& key)
    {
        return base_type::contains_key(key);
    }
};

} // End of namespace: simple
