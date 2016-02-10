// simple_set.h

#pragma once

#include "std_port/std_port_util.h"
#include "simple_vector.h"
#include "simple_string.h"
#include "simple_hash_map.h"
#include <string.h>

namespace simple
{

template <typename K, typename F, typename Alloc>
class hash_set_iterator;

// hash set
template <typename K, typename F = hash_func<K>, typename Alloc = XAlloc>
class hash_set
{
    enum { MinCapacity = 4 };
    enum { BadIndex = -1 };
    typedef unsigned int index_t;

public:
    typedef hash_set_iterator<K, F, Alloc> iterator;
    friend iterator;

public:
    hash_set(size_t capacity = MinCapacity) :
        m_keys(capacity),
        m_list(capacity),
        m_size(0),
        m_table(round_to_2n(capacity))
    {
        m_table_mask = (index_t) round_to_2n(capacity) - 1;
        m_table.push_backs(BadIndex, m_table_mask + 1);
    }

    ~hash_set()
    {
    }

    // Clear the content
    void clear()
    {
        // Clear all keys, values & index list
        m_keys.clear();
        m_list.clear();

        // Reset table empty entries
        m_table.clear();
        m_table.push_backs(BadIndex, MinCapacity * 2);
        m_table_mask = (index_t)m_table.size() - 1;
        m_size = 0;
    }

    // Is this hash map contains the key?
    bool contains(const K& key)
    {
        index_t index;
        return try_get_index(key, &index);
    }

    // Erase pair by iterator
    void erase(iterator& it)
    {
        STD_ASSERT(it.m_size == size());
        STD_ASSERT(it.m_index < size());
        erase(*it.m_cursor_ptr);

#ifdef _DEBUG
        // Update m_size
        it.m_size = (index_t)size();
#endif
    }

    // Erase pair by key
    bool erase(const K& key)
    {
        // Search in table
        index_t hash_value = hash_key(key);
        index_t *p = &m_table[hash_value];
        index_t index;
        while ((index = *p) != BadIndex)
        {
            // Get element's key & compare
            if (m_keys[index] == key)
            {
                // Take off from the list
                *p = m_list[index];

                // Put to free list
                free_node(index);
                return true;
            }

            // Try to compare next element
            p = &m_list[index];
        }
        return false;
    }

    // Find the iterator by key
    iterator find(const K& key)
    {
        index_t index;
        if (!try_get_index(key, &index))
            return end();

        return iterator(*this, index);
    }

    // Find the iterator by key of other class type with methods:
    //   hash_value(),
    //   equals(const K&)
    template <typename KK>
    iterator find_ex(const KK& key)
    {
        index_t hash_value = (index_t)key.hash_value() & m_table_mask;
        index_t element_index = m_table[hash_value];
        while (element_index != BadIndex)
        {
            // Get element's key & compare
            const K& element_key = m_keys[element_index];
            if (key.equals(element_key))
                return iterator(*(hash_set*)this, element_index);

            // Try to compare next element
            element_index = m_list[element_index];
        }
        return ((hash_set*)this)->end();
    }

    // Put key pair into map, replace if existed
    index_t put(const K& key)
    {
        // Get the location (index != bad when found or be bad when not found)
        index_t index;
        if (try_get_index(key, &index))
            // Found, just return index
            return index;

        // Not found, insert this new node
        return insert(key);
    }

    size_t size()
    {
        return m_size;
    }

public:
    // Generate vector of keys
    vector<K, Alloc> to_array()
    {
        vector<K, Alloc> vec(m_size);
        // Lookup entire table to add all keys
        for (size_t i = 0; i < m_size; i++)
            vec.push_back(m_keys[i]);
        return vec;
    }

private:
    // Get index
    index_t hash_key(const K& key) const
    {
        index_t hash_value = (index_t) m_hash_func(key) & m_table_mask;
        return hash_value;
    }

    // Allocate free node t add new node
    index_t allocate_node(const K& key)
    {
        if (m_size >= m_keys.size())
        {
            // Create new node in array
            STD_ASSERT(m_size == m_keys.size());
            m_keys.push_back(key);
            m_list.push_back(BadIndex); // No following node

            // Should I extend the hash table?
            if (m_keys.size() > (m_table.size() >> 1) /* /2 */)
            {
                // Enlarge the hash table entries & rehash all elements
                size_t old_length = m_table.size();
                for (size_t i = 0; i < old_length; i++)
                    m_table[i] = BadIndex;
                m_table.push_backs(BadIndex, old_length);
                m_table_mask = (index_t)m_table.size() - 1;
                STD_ASSERT(m_table_mask == round_to_2n(m_table_mask) - 1);
                size_t new_length = m_table.size();
                STD_ASSERT(new_length == old_length * 2);

                // Rehash all the elements by hash_value
                // Example:
                //      A + B + + C D +                     <= [Old m_table]
                //          TO
                //      A + + + + + D + + + B + + C + +     <= [New m_table]
                for (index_t j = 0; j < m_table.size(); j++)
                    m_table[j] = BadIndex;
                for (index_t i = 0; i < m_size; i++)
                {
                    // Put key to new slot
                    index_t hash_value = hash_key(m_keys[i]);
                    m_list[i] = m_table[hash_value];
                    m_table[hash_value] = i;
                }
                // End of rehash
            }
        } else
        {
            // Pick free node @ tail
            m_keys[m_size] = key;
            m_list[m_size] = BadIndex; // No following node
        }

        return m_size++;
    }

    // Free node
    void free_node(index_t index)
    {
        m_size--;
        index_t last_index = (index_t) m_size;
        if (index != last_index)
        {
            // Not last one, swap with the tail node
            K& last = m_keys[last_index];
            index_t hash_value = hash_key(last);
            m_keys[index] = simple::move(last); // last will be dropped after assign
            m_list[index] = m_list[last_index];

            // Update last_index to index in hash list
            index_t *p = &m_table[hash_value];
            while (*p != last_index)
            {
                STD_ASSERT(*p != BadIndex);
                p = &m_list[*p];
            }
            *p = index;
        }
    }

    // Search key, if found, return index of element location in nodes
    bool try_get_index(const K& key, index_t *index) const
    {
        index_t hash_value = hash_key(key);
        index_t element_index = m_table[hash_value];
        while (element_index != BadIndex)
        {
            // Get element's key & compare
            const K& element_key = m_keys[element_index];
            if (element_key == key)
            {
                *index = element_index;
                return true;
            }

            // Try to compare next element
            element_index = m_list[element_index];
        }
        *index = BadIndex;
        return false;
    }

    // Insert new value into set
    index_t insert(const K& key)
    {
        STD_ASSERT(! contains(key));

        // Add the key into table list
        index_t index;
        index = allocate_node(key);
        index_t hash_value = hash_key(key);
        m_list[index] = m_table[hash_value];
        m_table[hash_value] = index;
        return index;
    }

    // Find the smallest 2^n which > size
    size_t round_to_2n(size_t size)
    {
        size |= size >> 1;
        size |= size >> 2;
        size |= size >> 4;
        size |= size >> 8;
        size |= size >> 16;
#ifdef PLATFORM64
        size |= size >> 32;
#endif
        return size + 1;
    }

    // Iterator relatives
public:
    // Begin of container
    iterator begin()
    {
        return iterator(*this, 0);
    }

    // End of container
    iterator end()
    {
        iterator it(*this, m_size);
        return it;
    }

private:
    // hash table
    simple::unsafe_vector<K, Alloc> m_keys;
    simple::unsafe_vector<index_t, Alloc> m_table;
    simple::unsafe_vector<index_t, Alloc> m_list;
    index_t m_size;
    index_t m_table_mask; // = m_table.size() - 1, should be 111...111B
    F       m_hash_func;
};

// Iterator of container
template<typename K, typename F = hash_func<K>, typename Alloc = XAlloc>
class hash_set_iterator
{
    typedef hash_set<K, F, Alloc> hash_set_type;
    typedef typename hash_set_type::index_t index_t;
    friend hash_set_type;

public:
    hash_set_iterator() :
#ifdef _DEBUG
        m_set(0),
        m_size(0),
#endif
        m_index(0),
        m_cursor_ptr(0)
    {
    }

private:
    // Construct iterator by set & type (Begin, End)
    hash_set_iterator(hash_set_type& set, index_t index)
    {
#ifdef _DEBUG
        m_set = &set;
        m_size = (index_t) set.size();
#endif
        m_index = index;
        m_cursor_ptr = set.m_keys.get_array_address(index);
    }

public:
    // Get index of iterator
    index_t get_index() const
    {
        return m_index;
    }

public:
    K& operator * ()
    {
        STD_ASSERT(m_size == (index_t)m_set->size());
        STD_ASSERT(*this < m_set->end());
        return *m_cursor_ptr;
    }

    K *operator -> ()
    {
        STD_ASSERT(m_size == (index_t)m_set->size());
        STD_ASSERT(*this < m_set->end());
        return m_cursor_ptr;
    }

    // Move to next
    hash_set_iterator& operator ++ ()
    {
        m_index++;
        m_cursor_ptr++;
        return *this;
    }

    hash_set_iterator operator ++ (int)
    {
        hash_set_iterator tmp(*this);
        operator++();
        return tmp;
    }

    bool operator == (const hash_set_iterator& it) const
    {
        return m_index == it.m_index;
    }

    bool operator < (const hash_set_iterator& it) const
    {
        return m_index < it.m_index;
    }

private:
    // Iterator of set
#ifdef _DEBUG
    hash_set_type *m_set;
    index_t m_size;
#endif
    index_t m_index;
    K *m_cursor_ptr;
};

} // End of namespace: simple
