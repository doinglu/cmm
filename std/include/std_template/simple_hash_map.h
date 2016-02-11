// simple_hash.h

#pragma once

#include "std_port/std_port_util.h"
#include "simple_vector.h"
#include "simple_string.h"
#include "simple_pair.h"
#include "simple_hash_base.h"
#include <string.h>

namespace simple
{

template <typename T, typename K>
struct peek_key_from_pair
{
    const K& operator()(const T& element) const { return element.first; }
};

// hash map
template <typename K, typename V, typename F = hash_func<K> >
class hash_map : public hash_base<simple::pair<K, V>, K, peek_key_from_pair<simple::pair<K, V>, K>, F>
{
    typedef hash_base<simple::pair<K, V>, K, peek_key_from_pair<simple::pair<K, V>, K>, F> base_type;
    typedef typename base_type::index_t index_t;

public:
    hash_map(size_t capacity = base_type::MinCapacity, Allocator* allocator = &Allocator::g) :
        base_type(capacity, allocator)
    {
    }

public:
    // Generate vector of keys
    vector<K> keys() const
    {
        vector<K> vec(base_type::m_size);
        // Lookup entire table to add all keys
        for (size_t i = 0; i < base_type::m_size; i++)
            vec.push_back(base_type::m_elements[i].first);
        return vec;
    }

    // Generate vector of values
    vector<V> values() const
    {
        vector<V> vec(base_type::m_size);
        // Lookup entire table to add all keys
        for (size_t i = 0; i < base_type::m_size; i++)
            vec.push_back(base_type::m_elements[i].second);
        return simple::move(vec);
    }

    // Try to get the value by key, return false if not found
    bool try_get(const K& key, V* ptr_value) const
    {
        index_t index;
        if (!base_type::try_get_index(key, &index))
            // Failed to get the element in hash map
            return false;

        *ptr_value = base_type::m_elements[index].second;
        return true;
    }

    // Put key & value
    index_t put(const K& key, const V& value)
    {
        return base_type::put(simple::pair<K, V>(key, value));
    }

    // Get position to put value by key
    V& operator [] (const K& key)
    {
        // Get the location (index != bad when found or be bad when not found)
        index_t index;
        if (base_type::try_get_index(key, &index))
            // Found
            return base_type::m_elements[index].second;

        // Not found, insert new value
        index = base_type::insert(simple::pair<K, V>(key, V()));
        return base_type::m_elements[index].second;
    }
};

} // End of namespace: simple
