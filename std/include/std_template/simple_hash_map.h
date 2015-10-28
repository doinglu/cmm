// simple_hash.h

#pragma once

#include "std_port/std_port_util.h"
#include "simple_vector.h"
#include "simple_string.h"
#include "simple_pair.h"
#include <string.h>

namespace simple
{

template <typename K, typename V, typename F>
class hash_map_iterator;

template <typename T>
struct hash_func;

// hash map
template <typename K, typename V, typename F = hash_func<K> >
class hash_map
{
    enum { MinCapacity = 4 };
    enum { BadIndex = -1 };
    typedef unsigned int index_t;

public:
    typedef hash_map_iterator<K, V, F> iterator;
    friend iterator;

public:
    hash_map(size_t capacity = MinCapacity) :
        m_pairs(capacity),
        m_list(capacity),
        m_size(0),
        m_table(round_to_2n(capacity))
    {
        m_table_mask = (index_t) round_to_2n(capacity) - 1;
        m_table.push_backs(BadIndex, m_table_mask + 1);
    }

    ~hash_map()
    {
    }

    // Clear the content
    void clear()
    {
        // Clear all keys, values & index list
        m_pairs.clear();
        m_list.clear();

        // Reset table empty entries
        m_table.clear();
        m_table.push_backs(BadIndex, MinCapacity * 2);
        m_table_mask = (index_t)m_table.size() - 1;
    }

    // Is this hash map contains the key?
    bool contains_key(const K& key) const
    {
        index_t index;
        return try_get_index(key, &index);
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
            if (m_pairs[index].first == key)
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
    iterator find(const K& key) const
    {
        index_t index;
        if (!try_get_index(key, &index))
            return ((hash_map *)this)->end();

        return iterator(*(hash_map *)this, index);
    }

    // Put key-value pair into map, replace if existed
    index_t put(const K& key, const V& value)
    {
        // Get the location (index != bad when found or be bad when not found)
        index_t index;
        if (try_get_index(key, &index))
        {
            // Found, just replace
            m_pairs[index].second = value;
            return index;
        }

        // Not found, insert this new key-value pair
        return insert(key, value);
    }

    // Try to get the value by key, return false if not found
    bool try_get(const K& key, V* ptr_value) const
    {
        index_t index;
        if (!try_get_index(key, &index))
            // Failed to get the element in hash map
            return false;

        *ptr_value = m_pairs[index].second;
        return true;
    }

    size_t size() const
    {
        return m_size;
    }

    V& operator [] (const K& key)
    {
        // Get the location (index != bad when found or be bad when not found)
        index_t index;
        if (try_get_index(key, &index))
            // Found
            return m_pairs[index].second;

        // Not found, insert new value
        index = insert(key, V());
        return m_pairs[index].second;
    }

public:
    // Generate vector of keys
    vector<K> keys() const
    {
        vector<K> vec(m_size);
        // Lookup entire table to add all keys
        for (size_t i = 0; i < m_size; i++)
            vec.push_back(m_pairs[i].first);
        return vec;
    }

    // Generate vector of values
    vector<V> values() const
    {
        vector<V> vec(m_size);
        // Lookup entire table to add all keys
        for (size_t i = 0; i < m_size; i++)
            vec.push_back(m_pairs[i].second);
        return simple::move(vec);
    }

private:
    // Get index
    index_t hash_key(const K& key) const
    {
        index_t hash_value = (index_t) m_hash_func(key) & m_table_mask;
        return hash_value;
    }

    // Allocate free node t add new key/value pair
    index_t allocate_node(const K& key, const V& value)
    {
        if (m_size >= m_pairs.size())
        {
            // Create new node in pairs array
            STD_ASSERT(m_size == m_pairs.size());
            m_pairs.push_back(pair<K, V>(key, value));
            m_list.push_back(BadIndex); // No following node

            // Should I extend the hash table?
            if (m_pairs.size() > (m_table.size() >> 1) /* /2 */)
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
                for (index_t i = 0; i < m_size; i++)
                {
                    // Put key to new slot
                    index_t hash_value = hash_key(m_pairs[i].first);
                    m_list[i] = m_table[hash_value];
                    m_table[hash_value] = i;
                }
                // End of rehash
            }
        } else
        {
            // Pick free node @ tail
            m_pairs[m_size] = pair<K, V>(key, value);
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
            // Not last one, swap with the tail pair
            pair<K, V>& last = m_pairs[last_index];
            index_t hash_value = hash_key(last.first);
            m_pairs[index] = simple::move(last); // last will be dropped after assign
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

    // Search key, if found, return index of element location in pairs
    bool try_get_index(const K& key, index_t *index) const
    {
        index_t hash_value = hash_key(key);
        index_t element_index = m_table[hash_value];
        while (element_index != BadIndex)
        {
            // Get element's key & compare
            const K& element_key = m_pairs[element_index].first;
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

    // Insert new value into map
    index_t insert(const K& key, const V& value)
    {
        STD_ASSERT(! contains_key(key));

        // Add the pair into table list
        index_t index;
        index = allocate_node(key, value);
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
    simple::unsafe_vector<pair<K, V> > m_pairs;
    simple::unsafe_vector<index_t> m_table;
    simple::unsafe_vector<index_t> m_list;
    index_t m_size;
    index_t m_table_mask; // = m_table.size() - 1, should be 111...111B
    F       m_hash_func;
};

// Iterator of container
template<typename K, typename V, typename F = hash_func<K> >
class hash_map_iterator
{
    typedef hash_map<K, V, F> hash_map_type;
    typedef typename hash_map_type::index_t index_t;
    friend hash_map_type;

public:
    hash_map_iterator() :
#ifdef _DEBUG
        m_map(0),
        m_size(0),
#endif
        m_index(0),
        m_cursor_ptr(0)
    {
    }

private:
    // Construct iterator by mapping & type (Begin, End)
    hash_map_iterator(hash_map_type& m, index_t index)
    {
#ifdef _DEBUG
        m_map = &m;
        m_size = (index_t) m.size();
#endif
        m_index = index;
        m_cursor_ptr = m.m_pairs.get_array_address(index);
    }

public:
    // Get index of iterator
    index_t get_index()
    {
        return m_index;
    }

public:
    pair<K, V>& operator * ()
    {
        STD_ASSERT(m_size == m_map->size());
        STD_ASSERT(*this < m_map->end());
        return *m_cursor_ptr;
    }

    pair<K, V> *operator -> ()
    {
        STD_ASSERT(m_size == m_map->size());
        STD_ASSERT(*this < m_map->end());
        return m_cursor_ptr;
    }

    // Move to next
    hash_map_iterator& operator ++ ()
    {
        m_index++;
        m_cursor_ptr++;
        return *this;
    }

    hash_map_iterator operator ++ (int)
    {
        hash_map_iterator tmp(*this);
        operator++();
        return tmp;
    }

    bool operator == (const hash_map_iterator& it) const
    {
        return m_index == it.m_index;
    }

    bool operator < (const hash_map_iterator& it) const
    {
        return m_index < it.m_index;
    }

private:
    // Iterator of map
#ifdef _DEBUG
    hash_map_type *m_map;
    index_t m_size;
#endif
    index_t m_index;
    pair<K, V> *m_cursor_ptr;
};

// hash function of key in hash map
template<typename T = void>
struct hash_func
{
    size_t operator()(const T& key) const
    {
        return 0;
    }
};

// Hash routines for int
template<> struct hash_func<int>    { size_t operator()(int x)    const { return (size_t)x; } }; 
template<> struct hash_func<Int8>   { size_t operator()(Int8 x)   const { return (size_t)x; } }; 
template<> struct hash_func<Int16>  { size_t operator()(Int16 x)  const { return (size_t)x; } }; 
template<> struct hash_func<Int32>  { size_t operator()(Int32 x)  const { return (size_t)x; } }; 
template<> struct hash_func<Int64>  { size_t operator()(Int64 x)  const { return (size_t)x; } };
template<> struct hash_func<Uint>   { size_t operator()(Uint x)   const { return (size_t)x; } }; 
template<> struct hash_func<Uint8>  { size_t operator()(Uint8 x)  const { return (size_t)x; } }; 
template<> struct hash_func<Uint16> { size_t operator()(Uint16 x) const { return (size_t)x; } }; 
template<> struct hash_func<Uint32> { size_t operator()(Uint32 x) const { return (size_t)x; } }; 
template<> struct hash_func<Uint64> { size_t operator()(Uint64 x) const { return (size_t)x; } };
template<typename T>
struct hash_func<T *>
{
    size_t operator()(T *x) const
    {
        return (size_t)x / sizeof(T *);
    }
};

// Hash routines for simple::string
template<> struct hash_func<simple::string>
{
    size_t operator()(const simple::string& s) const { return s.hash_value(); }
};

} // End of namespace: simple
