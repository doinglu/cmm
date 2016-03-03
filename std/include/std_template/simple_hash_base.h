// simple_hash_base.h
// Initial version by doing

#pragma once

#include "std_port/std_port_util.h"
#include "simple_vector.h"
#include "simple_string.h"
#include <string.h>

namespace simple
{

template <typename T, typename K, typename P, typename F>
class hash_base_iterator;

template <typename T>
struct hash_func;

template <typename T>
struct peek_func;

// hash map
template <typename T, typename K, typename P = peek_func<T>, typename F = hash_func<K> >
class hash_base
{
public:
    enum { MinCapacity = 4 };
    enum { BadIndex = -1 };
    typedef unsigned int index_t;

public:
    typedef hash_base_iterator<T, K, P, F> iterator;
    friend iterator;

public:
    hash_base(size_t capacity = MinCapacity, Allocator* allocator = &Allocator::g) :
        m_allocator(allocator),
        m_elements(capacity, allocator),
        m_list(capacity, allocator),
        m_size(0),
        m_table(round_to_2n(capacity))
    {
        m_table_mask = (index_t)round_to_2n(capacity) - 1;
        m_table.push_backs(BadIndex, m_table_mask + 1);
    }

    ~hash_base()
    {
    }

    // Clear the content
    void clear()
    {
        // Clear all keys, values & index list
        m_elements.clear();
        m_list.clear();

        // Reset table empty entries
        m_table.clear();
        m_table.push_backs(BadIndex, MinCapacity * 2);
        m_table_mask = (index_t)m_table.size() - 1;
        m_size = 0;
    }

    // Is this hash base contains the key?
    bool contains_key(const K& key) const
    {
        index_t index;
        return try_get_index(key, &index);
    }

    // Erase pair by iterator
    void erase(iterator& it)
    {
        STD_ASSERT(it.m_size == size());
        STD_ASSERT(it >= begin() && it < end());
        erase(m_peek_func(*it.m_cursor_ptr));

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
            if (m_peek_func(m_elements[index]) == key)
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
            return ((hash_base*)this)->end();

        return iterator(*(hash_base*)this, index);
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
            const K& element_key = m_peek_func(m_elements[element_index]);
            if (key.equals(element_key))
                return iterator(*(hash_base*)this, element_index);

            // Try to compare next element
            element_index = m_list[element_index];
        }
        return ((hash_base*)this)->end();
    }

    // Put key-value pair into map, replace if existed
    index_t put(const T& element)
    {
        // Get the location (index != bad when found or be bad when not found)
        index_t index;
        if (try_get_index(m_peek_func(element), &index))
        {
            // Found, just replace
            m_elements[index] = element;
            return index;
        }

        // Not found, insert this new key-value pair
        return insert(element);
    }

    size_t size() const
    {
        return m_size;
    }

protected:
    // Get index
    index_t hash_key(const K& key) const
    {
        index_t hash_value = (index_t)m_hash_func(key) & m_table_mask;
        return hash_value;
    }

    // Allocate free node t add new key/value pair
    index_t allocate_node(const T& element)
    {
        if (m_size >= m_elements.size())
        {
            // Create new node in pairs array
            STD_ASSERT(m_size == m_elements.size());
            m_elements.push_back(element);
            m_list.push_back(BadIndex); // No following node

            // Should I extend the hash table?
            if (m_elements.size() > (m_table.size() >> 1) /* /2 */)
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
                    index_t hash_value = hash_key(m_peek_func(m_elements[i]));
                    m_list[i] = m_table[hash_value];
                    m_table[hash_value] = i;
                }
                // End of rehash
            }
        } else
        {
            // Pick free node @ tail
            m_elements[m_size] = element;
            m_list[m_size] = BadIndex; // No following node
        }

        return m_size++;
    }

    // Free node
    void free_node(index_t index)
    {
        m_size--;
        index_t last_index = (index_t)m_size;
        if (index != last_index)
        {
            // Not last one, swap with the tail pair
            auto& last = m_elements[last_index];
            index_t hash_value = hash_key(m_peek_func(last));
            m_elements[index] = simple::move(last); // last will be dropped after assign
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
            const K& element_key = m_peek_func(m_elements[element_index]);
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
    index_t insert(const T& element)
    {
        const K& key = m_peek_func(element);
        STD_ASSERT(! contains_key(key));

        // Add the pair into table list
        index_t index;
        index = allocate_node(element);
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

protected:
    // hash table
    Allocator* m_allocator;
    unsafe_vector<T> m_elements;
    unsafe_vector<index_t> m_table;
    unsafe_vector<index_t> m_list;
    index_t m_size;
    index_t m_table_mask; // = m_table.size() - 1, should be 111...111B
    P       m_peek_func;
    F       m_hash_func;
};

// Iterator of container
template<typename T, typename K, typename P = peek_func<T>, typename F = hash_func<K> >
class hash_base_iterator
{
    typedef hash_base<T, K, P, F> hash_base_type;
    typedef typename hash_base_type::index_t index_t;
    friend hash_base_type;

public:
    hash_base_iterator()
#ifdef _DEBUG
      : m_map(0),
        m_size(0),
        m_index(0),
        m_cursor_ptr(0)
#endif
    {
    }

private:
    // Construct iterator by mapping & type (Begin, End)
    hash_base_iterator(hash_base_type& m, index_t index)
    {
#ifdef _DEBUG
        m_map = &m;
        m_size = (index_t)m.size();
        m_index = index;
#endif
        m_cursor_ptr = m.m_elements.get_array_address(index);
    }

public:
    T& operator * ()
    {
        STD_ASSERT(m_size == m_map->size());
        STD_ASSERT(m_index < m_size);
        return *m_cursor_ptr;
    }

    T *operator -> ()
    {
        STD_ASSERT(m_size == m_map->size());
        STD_ASSERT(m_index < m_size);
        return m_cursor_ptr;
    }

    // Move to next
    hash_base_iterator& operator ++ ()
    {
#ifdef _DEBUG
        m_index++;
#endif
        m_cursor_ptr++;
        return *this;
    }

    hash_base_iterator operator ++ (int)
    {
        hash_base_iterator tmp(*this);
        operator++();
        return tmp;
    }

    bool operator == (const hash_base_iterator& it) const
    {
        return m_cursor_ptr == it.m_cursor_ptr;
    }

    bool operator < (const hash_base_iterator& it) const
    {
        return m_cursor_ptr < it.m_cursor_ptr;
    }

private:
    // Iterator of map
#ifdef _DEBUG
    hash_base_type *m_map;
    index_t m_size;
    index_t m_index;
#endif
    T *m_cursor_ptr;
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
template<> struct hash_func<Int8>   { size_t operator()(Int8 x)   const { return (size_t)x; } }; 
template<> struct hash_func<Int16>  { size_t operator()(Int16 x)  const { return (size_t)x; } };
template<> struct hash_func<Int32>  { size_t operator()(Int32 x)  const { return (size_t)x; } };
template<> struct hash_func<Int64>  { size_t operator()(Int64 x)  const { return (size_t)x; } };
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
