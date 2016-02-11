// simple_list.h

#pragma once

#include "simple.h"
#include "std_port/std_port.h"
#include "std_port/std_port_util.h"

namespace simple
{

template <typename T>
class list_iterator;

template <typename T>
struct list_node
{
    struct list_node *next;
    struct list_node *prev;
    T value;

    template <class... Types>
    list_node(Types&&... args) :
        value(simple::forward<Types>(args)...)
    {
    }
};

// The list node is manual list should be operated manually by user
// User create & free node
template <typename T>
class manual_list
{
    struct stub_node
    {
        struct stub_node *next;
        struct stub_node *prev;
    };

public:
    typedef list_iterator<T> iterator;
    typedef list_node<T> node;
    friend iterator;

public:
    manual_list() :
        m_head((node *)&m_begin_stub),
        m_tail((node *)&m_end_stub),
        m_size(0)
    {
        // Initialize stub nodes
        m_begin_stub.prev = 0;
        m_begin_stub.next = &m_end_stub;
        m_end_stub.prev = &m_begin_stub;
        m_end_stub.next = 0;
    }

    manual_list& operator = (const manual_list& other)
    {
        m_size = 0;
        node *p;
        size_t i;
        for (i = 0, p = other.m_head->next;
             i < other.m_size;
             i++, p = p->next)
        {
            append(p->value);
        }

        return *this;
    }

    T& operator [](size_t index) const
    {
        if (index >= m_size)
            throw "Index is out of range in manual_list.\n";

        // Lookup element in manual_list by index
        node *p = m_head->next;
        while (index--)
            p = p->next;
        return p->value;
    }

    // Find element in manual_list
    iterator find(const T& element)
    {
        size_t i = 0;
        auto *p = m_head->next;
        for (;
             p != m_tail;
             p = p->next, i++)
        {
            if (p->value == element)
                return iterator(*this, i, p);
        }

        // Not found
        return end();
    }

    // Remove an element
    node *remove_node(iterator& it)
    {
        return remove_node(it.m_cursor_ptr);
    }

    // Remove an element @ it
    node *remove_node(node *this_node)
    {
        STD_ASSERT(("Expected valid node to remove.", this_node));

        // Before processed: Prev This Next
        // After processed:  Prev Next
        auto *prev = this_node->prev;
        auto *next = this_node->next;
        prev->next = next;
        next->prev = prev;

        // Update size
        m_size--;

        // Return the token node
        return this_node;
    }

    // Get length of existed elements
    size_t size() const
    {
        return m_size;
    }

    // Iterator relatives
public:
    // Begin of container
    iterator begin()
    {
        return iterator(*this, 0, m_head->next);
    }

    // End of container
    iterator end()
    {
        iterator it(*this, m_size, m_tail);
        return it;
    }

public:
    void append_node(node *p)
    {
        // Link this node to manual_list
        auto *prev = m_tail->prev;
        prev->next = p;
        p->prev = prev;
        p->next = m_tail;
        m_tail->prev = p;

        // Update size
        m_size++;
    }

    // Insert node p before node it. it can be end()
    void insert_node_before(iterator it, node *p)
    {
        node *this_node = it.m_cursor_ptr;

        // Before processed: Prev This Next
        // After processed:  Prev Node This Next
        this_node->prev->next = p;
        p->prev = this_node->prev;
        p->next = this_node;
        this_node->prev = p;

        // Update size
        m_size++;
    }

    // Insert node p after node it. it can't be end()
    void insert_node_after(iterator it, node *p)
    {
        STD_ASSERT(("Expect valid node to insert after.", it.m_cursor_ptr != m_tail));
        insert_node_before(++it, p);

        // Update size
        m_size++;
    }

protected:
    // hash table
    stub_node m_begin_stub, m_end_stub;
    node *m_head, *m_tail;
    size_t m_size;
};

template <typename T>
class list : public manual_list<T>
{
    typedef manual_list<T> base;
    typedef typename base::iterator iterator;
    typedef typename base::node node;

public:
    list(Allocator* allocator = &Allocator::g) :
        m_allocator(allocator)
    {
    }

    list(const list &other)
    {
        m_allocator = other.m_allocator;
        *this = other;
    }

    list(list&& other)
    {
        m_allocator = other.m_allocator;
        *this = simple::move(other);
    }

    // Construct from T[] with size
    list(const T *arr, size_t count)
    {
        for (size_t i = 0; i < count; i++)
            append(arr[i]);
    }

    ~list()
    {
        clear();
    }

    list& operator = (const list& other)
    {
        base::m_size = 0;
        node *p;
        size_t i;
        for (i = 0, p = other.m_head->next;
             i < other.m_size;
             i++, p = p->next)
        {
            append(p->value);
        }

        return *this;
    }

    list& operator = (list&& other)
    {
        if (m_allocator != other.m_allocator)
            // Not same allocator, do copy
            return *this = other;

        base::m_size = other.m_size;
        if (base::m_size == 0)
        {
            base::m_head->next = base::m_tail;
            base::m_tail->prev = base::m_head;
        } else
        {
            // Steal list nodes between other.head & other.tail
            base::m_head->next = other.m_head->next;
            base::m_head->next->prev = base::m_head;
            base::m_tail->prev = other.m_tail->prev;
            base::m_tail->prev->next = base::m_tail;

            // Reset other
            other.m_size = 0;
            other.m_head->next = other.m_tail;
            other.m_tail->prev = other.m_head;
        }
        return *this;
    }

    // Clear all elements
    void clear()
    {
        // Remove elements
        node *p = base::m_head->next;
        while (p != base::m_tail)
        {
            node *this_node = p;
            p = p->next;
            m_allocator->template delete1<node>(__FILE__, __LINE__, this_node);
        }

        // Reset list
        base::m_size = 0;
        base::m_head->next = base::m_tail;
        base::m_tail->prev = base::m_head;
    }

    // Insert before
    void insert_before(iterator it, const T& element)
    {
        node *p = m_allocator->template new1<node>(__FILE__, __LINE__, element);
        insert_node_before(it, p);
    }

    // Insert before
    void insert_before(iterator it, T&& element)
    {
        node *p = m_allocator->template new1<node>(__FILE__, __LINE__, simple::move(element));
        insert_node_before(it, p);
    }

    // Insert after
    void insert_after(iterator it, const T& element)
    {
        node *p = m_allocator->template new1<node>(__FILE__, __LINE__, element);
        insert_node_after(it, p);
    }

    // Insert after
    void insert_after(iterator it, T&& element)
    {
        node *p = m_allocator->template new1<node>(__FILE__, __LINE__, simple::move(element));
        insert_node_after(it, p);
    }

    // Append element @ tail
    void append(const T& element)
    {
        node *p = m_allocator->template new1<node>(__FILE__, __LINE__, element);
        this->append_node(p);
    }

    // Append element @ tail
    void append(T&& element)
    {
        node *p = m_allocator->template new1<node>(__FILE__, __LINE__, simple::move(element));
        this->append_node(p);
    }

    // Remove an element @ index
    void remove_at(size_t index)
    {
        if (index >= base::m_size)
            throw "Element is out of range when remove from list.\n";

        this->remove(iterator(*this, index));
    }

    // Remove an element @ it
    void remove(iterator& it)
    {
        m_allocator->template delete1<node>(__FILE__, __LINE__, this->remove_node(it.get_node()));
    }

private:
    Allocator* m_allocator;
};

// Iterator of container
template<typename T>
class list_iterator
{
    typedef manual_list<T> list_type;
    typedef T value_type;
    typedef list_node<T> node;
    friend list_type;

public:
    list_iterator() :
#ifdef _DEBUG
        m_list(0),
        m_size(0),
#endif
        m_index(0),
        m_cursor_ptr(0)
    {
    }

private:
    // Construct iterator by list & type (Begin, End)
    list_iterator(list_type& the_list, size_t index, node *p)
    {
#ifdef _DEBUG
        m_list = &the_list;
        m_size = the_list.size();
        STD_ASSERT(("Expect valid index in [0..m_size].", index <= m_size));
#endif
        STD_ASSERT(index == 0 && p == the_list.m_head->next ||
                   index == the_list.size() && p == the_list.m_tail);
        m_index = index;
        m_cursor_ptr = p;
    }

public:
    value_type& operator * ()
    {
        STD_ASSERT(("List was changed.", m_size == m_list->size()));
        STD_ASSERT(("Iterator is end.", m_index < m_size));
        return m_cursor_ptr->value;
    }

    value_type *operator -> ()
    {
        STD_ASSERT(("List was changed.", m_size == m_list->size()));
        STD_ASSERT(("Iterator is end.", m_index < m_size));
        return &m_cursor_ptr->value;
    }

    // Move to next
    list_iterator& operator ++ ()
    {
        m_cursor_ptr = m_cursor_ptr->next;
        m_index++;
        return *this;
    }

    list_iterator operator ++ (int)
    {
        list_iterator tmp(*this);
        operator++();
        return tmp;
    }

    bool operator == (const list_iterator& it) const
    {
        return m_cursor_ptr == it.m_cursor_ptr;
    }

    bool operator < (const list_iterator& it) const
    {
        return m_index < it.m_index;
    }

public:
    node* get_node() const
    {
        return m_cursor_ptr;
    }

    size_t get_index() const
    {
        return m_index;
    }

private:
    // Iterator of map
#ifdef _DEBUG
    list_type *m_list;
    size_t m_size;
#endif
    size_t m_index;
    node  *m_cursor_ptr;
};

} // End of namespace: simple
