// simple_list.h

#ifndef _SIMPLE_LIST_H_
#define _SIMPLE_LIST_H_

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
    struct list_node **pp_prev;
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
public:
    typedef list_iterator<T> iterator;
    typedef list_node<T> node;
    friend iterator;

public:
    manual_list() :
        m_head(0),
        m_pp_tail(&m_head),
        m_size(0)
    {
    }

    manual_list& operator = (const manual_list& other)
    {
        m_size = 0;
        for (size_t i = 0, node *p = other.m_head;
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
            throw "Index is out of range in manual_list.";

        // Lookup element in manual_list by index
        node *p = m_head;
        while (index--)
            p = p->next;
        return p->value;
    }

    // Find element in manual_list
    iterator find(const T& element)
    {
        size_t i = 0;
        auto *p = m_head;
        for (;
             p != 0;
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
        *this_node->pp_prev = this_node->next;
        if (this_node->next)
        {
            this_node->next->pp_prev = this_node->pp_prev;
        } else
        {
            // This node is the last one, update pp_tail
            m_pp_tail = this_node->pp_prev;
        }

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
        return iterator(*this, 0, m_head);
    }

    // End of container
    iterator end()
    {
        iterator it(*this, 0, NULL);
        return it;
    }

public:
    void append_node(node *p)
    {
        // Link this node to manual_list
        *m_pp_tail = p;
        p->pp_prev = m_pp_tail;
        p->next = 0;

        // Move pp tail to next 
        m_pp_tail = &p->next;

        // Update size
        m_size++;
    }

    // Insert node p before node it. it can be end()
    void insert_node_before(iterator it, node *p)
    {
        node *this_node = it.m_cursor_ptr;
        if (!this_node)
        {
            // Insert before the NULL node (or the manual_list is empty)
            append_node(p);
            return;
        }

        // Before processed: Prev This Next
        // After processed:  Prev Node This Next

        // Link Prev <-> Node
        p->pp_prev = this_node->pp_prev;
        *this_node->pp_prev = p;

        // Link Node <-> This
        p->next = this_node;
        this_node->pp_prev = &p->next;

        // Update size
        m_size++;
    }

    // Insert node p after node it. it can't be end()
    void insert_node_after(iterator it, node *p)
    {
        STD_ASSERT(("Expect valid node to insert after.", it.m_cursor_ptr));
        insert_node_before(++it, p);

        // Update size
        m_size++;
    }

protected:
    // hash table
    node  *m_head, **m_pp_tail;
    size_t m_size;
};

template <typename T>
class list : public manual_list<T>
{
public:
    list()
    {
    }

    list(const list &other)
    {
        *this = other;
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
        m_size = 0;
        for (size_t i = 0, node *p = other.m_head;
        i < other.m_size;
            i++, p = p->next)
        {
            append(p->value);
        }

        return *this;
    }

    list& operator = (list&& other)
    {
        m_size = other.m_size;
        m_head = other.m_head;

        // Set pointer to ptr tail
        if (!m_head)
        {
            // The list is empty, set pp to m_head
            m_pp_tail == &m_head;
        }
        else
        {
            // The list is not empty
            m_pp_tail = other.m_pp_tail;
            // set pp to m_head
            m_head->pp_prev = &m_head;
        }

        // Reset other
        other.m_size = 0;
        other.m_head = 0;
        other.m_pp_tail = &other.m_head;
        return *this;
    }

    // Clear all elements
    void clear()
    {
        // Remove elements
        node *p = m_head;
        while (p)
        {
            node *this_node = p;
            p = p->next;
            delete(this_node);
        }

        // Reset list
        m_size = 0;
        m_head = 0;
        m_pp_tail = &m_head;
    }

    // Insert before
    void insert_before(iterator it, const T& element)
    {
        node *p = new node(element);
        insert_node_before(it, p);
    }

    // Insert before
    void insert_before(iterator it, T&& element)
    {
        node *p = new node(simple::move(element));
        insert_node_before(it, p);
    }

    // Insert after
    void insert_after(iterator it, const T& element)
    {
        node *p = new node(element);
        insert_node_after(it, p);
    }

    // Insert after
    void insert_after(iterator it, T&& element)
    {
        node *p = new node(simple::move(element));
        insert_node_after(it, p);
    }

    // Append element @ tail
    void append(const T& element)
    {
        node *p = new node(element);
        append_node(p);
    }

    // Append element @ tail
    void append(T&& element)
    {
        node *p = new node(simple::move(element));
        append_node(p);
    }

    // Remove an element @ index
    void remove_at(size_t index)
    {
        if (index >= m_size)
            throw "Element is out of range when remove from list.";

        remove(iterator(*this, index));
    }

    // Remove an element @ it
    void remove(iterator& it)
    {
        delete remove_node(it.m_cursor_ptr);
    }
};

// Iterator of container
template<typename T>
class list_iterator
{
    typedef manual_list<T> list_type;
    typedef T value_type;
    typedef list_node<T> node;
    friend list_type;
    friend list<T>;

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
    // Construct iterator by mapping & type (Begin, End)
    list_iterator(list_type& the_list, size_t index)
    {
#ifdef _DEBUG
        m_list = &the_list;
        m_size = the_list.size();
        STD_ASSERT(("Expect valid index [0..m_size].", index <= m_size));
#endif
        m_index = index;

        // Lookup the node by index
        node *p = the_list.m_head;
        while (index--)
            p = p->next;
        m_cursor_ptr = p;
    }

    // Construct iterator by mapping & type (Begin, End)
    list_iterator(list_type& the_list, size_t index, node *p)
    {
#ifdef _DEBUG
        m_list = &the_list;
        m_size = the_list.size();
        STD_ASSERT(("Expect valid index in [0..m_size].", index <= m_size));
#endif
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

#endif
