// cmm_domain.h

#pragma once

#include "std_template/simple_list.h"

#include "cmm.h"
#include "cmm_value.h"
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
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

public:
    Domain(const char *name);
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
#if USE_LIST_IN_VALUE_LIST
        STD_ASSERT(("Value is already binded to domain.", !value->next));
#endif
        STD_ASSERT(("Value is already binded to domain.", !value->owner));
        m_value_list.append_value(value);

        // Should I need do a GC?
        --m_gc_counter;
        check_gc();
    }

    // Do GC when necessary
    void check_gc()
    {
        if (m_gc_counter <= 0)
            gc();
    }

    // Concat a value list
    void concat_value_list(ValueList *list)
    {
        m_gc_counter -= (IntR)list->get_count();
        m_value_list.concat_list(list);
    }

    // Is this value in my value list?
    const ValueList *get_value_list()
    {
        return &m_value_list;
    }

public:
    // Garbage collect
    void gc();

private:
    // Internal routine called by gc()
    void gc_internal(Thread* thread);

public:
    // Let object join in domain
    void join_object(Object *ob);

    // Object was destructed, left domain
    void object_was_destructed(Object *ob);

public:
    // Get id of this domain
    DomainId get_id() const { return m_id; }

    // Get name of this domain
    const char *get_name() const { return m_name; }

    // Which thread hold this domain
    Thread::Id get_thread_holder() const { return m_thread_holder_id; }

    // Get type of this domain
    Type get_type() const { return m_type; }

    // How many threads in wait list
    size_t get_wait_counter() const { return m_wait_counter; }

    // Is this domain running?
    bool is_running() const { return m_running ? true : false; }

    // Return domain in a mapping value
    Map get_domain_detail();

private:
    // Get domain 0
    static Domain *get_domain_0() { return m_domain_0; }

private:
    char        m_name[32];         // Name
    Type        m_type;             // Type
    DomainId    m_id;               // Id
    AtomInt     m_running;          // Is this domain running?
    AtomInt     m_wait_counter;     // How many threads are waiting to take ownership
    Thread::Id  m_thread_holder_id; // Hold by which thread?

    // Event for synchronize
    std_event_id_t m_event_id;

    // All objects
    simple::hash_set<Object *> m_objects;

    // List of all reference value in this domain
    ValueList m_value_list;
    IntR m_gc_counter;

    // List of all contexts in threads
    simple::manual_list<DomainContext> m_context_list;

private:
    typedef GlobalIdManager<Entry> DomainIdManager;
    static DomainIdManager *m_id_manager;

    typedef simple::hash_map<DomainId, Domain *, global_id_hash_func> DomainIdMap;
    static DomainIdMap *m_all_domains;
    static Domain *m_domain_0;

    // Critical Section to operate m_all_domains
    static struct std_critical_section *m_domain_cs;
};

} // End of namespace: cmm
