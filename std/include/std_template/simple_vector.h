// simple_vector.h

#pragma once

#include "simple.h"
#include "std_port/std_port.h"
#include "std_port/std_port_util.h"
////----#include <vector>////----

namespace simple
{

template <typename T>
class vector_iterator;

template <typename T>
class vector
{
public:
    typedef vector_iterator<T> iterator;
    typedef int difference_type;
    friend iterator;

public:
    vector(const vector &vec) :
        vector(vec.m_space, vec.m_allocator)
    {
        *this = vec;
    }

    vector(vector&& vec, Allocator* allocator = &Allocator::g)
    {
        m_allocator = allocator;
        m_array = 0;
        *this = simple::move(vec);
    }

    // Construct from T[] with size
    vector(const T *arr, size_t count, Allocator* allocator = &Allocator::g)
    {
        m_allocator = allocator;
        m_space = count;
        m_size = count;
        m_array = (T*)m_allocator->alloc(__FILE__, __LINE__, count * sizeof(T));
        for (size_t i = 0; i < count; i++)
            new (&m_array[i])T(arr[i]);
    }

    vector(size_t capacity = 8, Allocator* allocator = &Allocator::g)
    {
        m_allocator = allocator;

        // Allocate an empty array with reserved space
        if (capacity < 1)
            capacity = 1;
        m_space = capacity;
        m_array = (T*)m_allocator->alloc(__FILE__, __LINE__, m_space * sizeof(T));
        m_size = 0;
    }

    ~vector()
    {
        if (m_array)
            free_array();
    }

    vector& operator = (const vector& vec)
    {
        if (!m_array || m_space < vec.size())
        {
            // Reallocate array
            if (m_array)
                free_array();

            m_space = vec.size();
            m_array = (T*)m_allocator->alloc(__FILE__, __LINE__, m_space * sizeof(T));
        }

        m_size = vec.size();
        for (size_t i = 0; i < m_size; i++)
            new (&m_array[i])T(vec.m_array[i]);

        return *this;
    }

    vector& operator = (vector&& vec)
    {
        if (m_allocator != vec.m_allocator)
            // Not same allocator, do copy
            return *this = vec;

        if (m_array)
            free_array();

        // Steal m_array from vec
        m_space = vec.m_space;
        m_size = vec.m_size;
        m_array = vec.m_array;

        // Rsh won't hold the array any longer
        vec.m_array = 0;

        return *this;
    }

    T& operator [](size_t index) const
    {
        if (index >= m_size)
            throw "Index is out of range in vector.\n";

        return m_array[index];
    }

    // Clear all elements
    void clear()
    {
        resize(0);
    }

    // Enlarge space to double size
    void extend()
    {
        T *new_array;
        m_space *= 2;
        if (m_space < 8)
            m_space = 8;
        new_array = (T*)m_allocator->alloc(__FILE__, __LINE__, m_space * sizeof(T));
        if (m_array)
        {
            for (size_t i = 0; i < m_size; i++)
                new (&new_array[i])T(simple::move(m_array[i]));
            free_array();
        }
        m_array = new_array;
    }

    // Reserve size
    void reserve(size_t to)
    {
        if (to < m_size)
        {
            // Do nothing
            STD_ASSERT(("Reseve size is smaller than existed size.\n", 0));
            return;
        }

        if (to == m_space)
            // Size ok
            return;

        T *new_array;
        m_space = to;
        new_array = (T*)m_allocator->alloc(__FILE__, __LINE__, m_space * sizeof(T));
        if (m_array)
        {
            for (size_t i = 0; i < m_size; i++)
                new (&new_array[i])T(simple::move(m_array[i]));
            free_array();
        }
        m_array = new_array;
    }

    // resize
    void resize(size_t to)
    {
        if (to > m_size)
        {
            // Extend size
            reserve(to);
            init_class(m_array + m_size, to - m_size, m_size);
            return;
        } else
        if (to < m_size)
        {
            // Don't use shrink since it may reallocate memory
            // Erase elements
            for (size_t i = to; i < m_size; i++)
                m_array[i].~T();
            m_size = to;
        }
    }

    // Shrink size
    void shrink(size_t to)
    {
        if (to > m_size)
        {
            // Do nothing
            STD_ASSERT(("Shrink size is bigger than existed size.\n", 0));
            return;
        }

        m_size = to;
        if (m_space > 32 && to < m_space / 2)
        {
            // Resize
            T *new_array;
            if (to < 8)
                to = 8;
            m_space = to;
            new_array = (T*)m_allocator->alloc(__FILE__, __LINE__, m_space * sizeof(T));
            for (size_t i = 0; i < m_size; i++)
                new (&new_array[i])T(simple::move(m_array[i]));
            free_array();
            m_array = new_array;
        }
    }

    // Find element in vector
    iterator find(const T& element)
    {
        for (auto i = 0; i < m_size; i++)
            if (m_array[i] == element)
                return iterator(*this, i);

        // Not found
        return end();
    }

    // Insert an element
    void insert(iterator it, const T& element)
    {
        insert(it - begin(), element);
    }

    // Insert an element
    void insert(size_t index, const T& element)
    {
        if (index > m_size)
            throw "Out of range for insertion.\n";

        if (m_size >= m_space)
        {
            // Allocate new array with 2* size & move all elements
            STD_ASSERT(m_size == m_space);
            extend();
        }
        new (&m_array[m_size])T();
        m_size++;
        for (size_t i = m_size - 1; i > index; i--)
            m_array[i] = simple::move(m_array[i - 1]);
        m_array[index] = element;
    }

    // Append an element
    inline void push_back(const T& element)
    {
        if (m_size >= m_space)
        {
            // Allocate new array with 2* size & move all elements
            STD_ASSERT(m_size == m_space);
            extend();
        }

        new (&m_array[m_size])T(element);
        m_size++;
    }

    // Append an element
    inline void push_back(T&& element)
    {
        if (m_size >= m_space)
        {
            // Allocate new array with 2* size & move all elements
            STD_ASSERT(m_size == m_space);
            extend();
        }

        new (&m_array[m_size])T(simple::move(element));
        m_size++;
    }

    // Append an element N times
    void push_backs(const T& e, size_t count)
    {
        STD_ASSERT(count >= 0);
        size_t new_size = m_size + count;
        // Extend size when necessary
        while (new_size > m_space)
            extend();
        // Put all values
        while (m_size < new_size)
            new (&m_array[m_size++])T(e);
    }

    // Append an array
    void push_back_array(const T *ptr, size_t count)
    {
        STD_ASSERT(count >= 0);
        size_t new_size = m_size + count;
        // Extend size when necessary
        while (new_size > m_space)
            extend();
        // Put all values
        size_t i = 0;
        while (m_size < new_size)
            new (&m_array[m_size++])T(ptr[i++]);
    }

    // Remove an element @ index
    void remove(size_t index)
    {
        if (index >= m_size)
            throw "Element is out of range when remove from array.\n";
        for (size_t i = index; i < m_size; i++)
            m_array[i] = simple::move(m_array[i + 1]);
        m_size--;
        m_array[m_size].~T();
    }

    // Remove an element @ it
    void remove(iterator& it)
    {
        remove((size_t) (it->m_cursor_ptr - m_array));
    }

    // Get length of existed elements
    size_t size() const
    {
        return m_size;
    }

protected:
    // Query array for unsafe operating
    T *get_array_unsafe() const
    {
        return m_array;
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
    // Free the array
    void free_array()
    {
        for (size_t i = 0; i < m_size; i++)
            m_array[i].~T();
        m_allocator->free(__FILE__, __LINE__, m_array);
        m_array = 0;

        // ATTENTION: KEEP the m_size/m_space, DON'T CHANGE THEM
        // NO: m_size = 0;  
        // NO: m_space = 0;
    }

private:
    // hash table
    Allocator *m_allocator;
    T         *m_array;
    size_t     m_size, m_space;
};

// Iterator of container
template<typename T>
class vector_iterator
{
public:
    typedef vector<T> vector_type;
    typedef T value_type;
    typedef T* pointer;
    typedef T& reference;
    typedef ptrdiff_t difference_type;
////----    typedef typename std::random_access_iterator_tag iterator_category; // Let it suitable for STL usage

    friend vector_type;

public:
    vector_iterator()
#ifdef _DEBUG
      : m_vec(0),
        m_size(0),
        m_index(0),
        m_cursor_ptr(0),
#endif
    {
    }

private:
    // Construct iterator by mapping & type (Begin, End)
    vector_iterator(vector_type& vec, size_t index)
    {
#ifdef _DEBUG
        m_vec = &vec;
        m_size = vec.size();
        m_index = index;
#endif
        m_cursor_ptr = &vec.m_array[index];
    }

public:
    value_type& operator * ()
    {
        STD_ASSERT(m_size == m_vec->size());
        STD_ASSERT(m_index < m_size);
        return *m_cursor_ptr;
    }

    value_type *operator -> ()
    {
        STD_ASSERT(m_size == m_vec->size());
        STD_ASSERT(m_index < m_size);
        return m_cursor_ptr;
    }

    // Move to next
    vector_iterator& operator ++ ()
    {
#ifdef _DEBUG
        m_index++;
#endif
        m_cursor_ptr++;
        return *this;
    }

    vector_iterator operator ++ (int)
    {
        vector_iterator tmp(*this);
        operator++();
        return tmp;
    }

    vector_iterator operator + (difference_type offset)
    {
        return vector_iterator(*this) += offset;
    }

    vector_iterator operator += (difference_type offset)
    {
#ifdef _DEBUG
        m_index += offset;
#endif
        m_cursor_ptr += offset;
        return *this;
    }

    vector_iterator operator - (difference_type offset)
    {
        return vector_iterator(*this) -= offset;
    }

    vector_iterator operator -= (difference_type offset)
    {
#ifdef _DEBUG
        m_index -= offset;
#endif
        m_cursor_ptr -= offset;
        return *this;
    }

    difference_type operator - (vector_iterator other)
    {
        return (difference_type)(m_cursor_ptr - other.m_cursor_ptr);
    }

    bool operator == (const vector_iterator& it) const
    {
        return m_cursor_ptr == it.m_cursor_ptr;
    }

    bool operator < (const vector_iterator& it) const
    {
        return m_cursor_ptr < it.m_cursor_ptr;
    }

private:
    // Iterator of map
#ifdef _DEBUG
    vector_type *m_vec;
    size_t m_size;
    size_t m_index;
#endif
    value_type *m_cursor_ptr;
};

// Unsafe vector (don't do bound check when index)
template<typename T>
class unsafe_vector : public vector<T>
{
public:
    unsafe_vector(size_t capacity = 8, Allocator* allocator = &Allocator::g) :
        vector<T>(capacity, allocator)
    {
    }

    T& operator [] (size_t index) const
    {
        STD_ASSERT(index < size());
        return vector<T>::get_array_unsafe()[index];
    }

    // Return the address of element index
    T *get_array_address(size_t index) const
    {
        STD_ASSERT(index <= size());
        return &vector<T>::get_array_unsafe()[index];
    }
};

} // End of namespace: simple
