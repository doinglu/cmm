// cmm_domain.h

#pragma once

#include "std_template/simple_list.h"

#include "cmm_basic_types.h"
#include "cmm_value_list.h"
#include "cmm_thread.h"

namespace cmm
{

struct ReferenceImpl;
class Object;
class Thread;

// Domain can't be a virtual class
class Domain
{
friend class Thread;

public:
    enum Type
    {
        NORMAL = 0,      // Normal domain, require lock
        READ_ONLY = 1,   // Simple object, all members are readonly, enter without lock
    };

    struct Entry
    {
        DomainId gid;       // Domain ID
        Domain *domain;     // Domain
    };

    enum { MAX_ID_PAGES = 1024 };

public:
    // Initialize this module
    static int init();

    // Shutdown this moudule
    static void shutdown();

public:
    Domain();
    ~Domain();

public:
    // Enter domain
    void enter();

    // Leave domain
    void leave();

public:
    // Bind a value to this domain
    void bind_value(ReferenceImpl *value, size_t count = 1)
    {
        STD_ASSERT(("Value is already binded to domain.", !value->next));
        STD_ASSERT(("Value is already binded to domain.", !value->owner));
        m_value_list.append_value(value);

        // Should I need do a GC?
        if (!--m_gc_counter)
            gc();
    }

    // Concat a value list
    void concat_value_list(ValueList *list)
    {
        m_value_list.concat_list(list);
        
        // Should I need do a GC?
        if (!--m_gc_counter)
            gc();
    }

    // Garbage collect
    void gc();

    // Is this value in my value list?
    const ValueList *get_value_list()
    {
        return &m_value_list;
    }

    // Mark value
    static void mark_value(MarkValueState& state, ReferenceImpl *ptr_value);

public:
    // Let object join in domain
    void join_object(Object *ob);

    // Object was destructed, left domain
    void object_was_destructed(Object *ob);

private:
    // Get domain 0
    static Domain *get_domain_0() { return m_domain_0; }

private:
    Type        m_type;             // Type of this domain
    DomainId    m_id;               // Id of this domain
    AtomInt     m_running;          // Is this domain running?
    AtomInt     m_wait_counter;     // How many threads are waiting to take ownership
    Thread::Id  m_thread_holder_id; // Hold by which thread?

    // Event for synchronize
    std_event_id_t m_event_id;

    // All objects
    simple::hash_set<Object *> m_objects;

    // List of all reference value in this domain
    ValueList m_value_list;
    size_t m_gc_counter;

    // List of all contexts in threads
    simple::manual_list<CallContext> m_context_list;

private:
    typedef GlobalIdManager<Entry> DomainIdManager;
    static DomainIdManager *m_id_manager;

    typedef simple::hash_map<DomainId, Domain *, global_id_hash_func> DomainIdMap;
    static DomainIdMap *m_all_domains;
    static Domain *m_domain_0;

    // Critical Section to operate m_all_domains
    static struct std_critical_section *m_domain_cs;

    // Function routine to get stack pointer for GC
    typedef void *(*GetStackPointerFunc)();
    static GetStackPointerFunc m_get_stack_pointer_func;
};

} // End of namespace: cmm
