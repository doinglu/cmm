// cmm_domain.cpp

#include <stdio.h>

#include "std_port/std_port.h"
#include "std_port/std_port_os.h"
#include "std_template/simple_hash_set.h"
#include "cmm_domain.h"
#include "cmm_object.h"
#include "cmm_program.h"
#include "cmm_value.h"

int conflict; ////----

namespace cmm
{

Domain::DomainIdManager *Domain::m_id_manager = 0;
Domain::DomainIdMap *Domain::m_all_domains = 0;
Domain *Domain::m_domain_0 = 0;
struct std_critical_section *Domain::m_domain_cs = 0;

// Initialize this module
int Domain::init()
{
    std_new_critical_section(&m_domain_cs);

    // Create the id manager
    m_id_manager = XNEW(DomainIdManager, MAX_ID_PAGES);

    // Create the domain 0
    m_all_domains = XNEW(DomainIdMap);
    m_domain_0 = XNEW(Domain, "Zero");
    return 0;
}

// Shutdown this moudule
void Domain::shutdown()
{
    // Clear all domains
    auto domains = m_all_domains->values();
    for (auto &it: domains)
        XDELETE(it);
    STD_ASSERT(("All domains should be freed.", m_all_domains->size() == 0));
    XDELETE(m_all_domains);
    m_domain_0 = 0;

    // Destory the id manager
    XDELETE(m_id_manager);

    std_delete_critical_section(m_domain_cs);
}

Domain::Domain(const char *name)
{
    m_type = NORMAL;
    m_id.i64 = 0;
    m_running = 0;
    m_wait_counter = 0;
    m_thread_holder_id = 0;

    // GC counter (first gc after 8 allocation)
    m_gc_counter = 8;

    // Create event for synchronous
    std_create_event(&m_event_id);

    // Assign an id
    auto *entry = m_id_manager->allocate_id();
    m_id = entry->gid;

    // Store the domain name
    char buf[sizeof(m_name)];
    if (name == NULL)
    {
        // Auto generate name by domain id
        m_id.print(buf, sizeof(buf), "Domain");
        name = buf;
    }
    strncpy(m_name, name, sizeof(m_name));
    m_name[sizeof(m_name) - 1] = 0;

    // Register me
    std_enter_critical_section(m_domain_cs);
    m_all_domains->put(m_id, this);
    std_leave_critical_section(m_domain_cs);
}

Domain::~Domain()
{
    // Remove all objects
    auto objects = m_objects.to_array();
    for (auto &it: objects)
        XDELETE(it);
    STD_ASSERT(("There are still alive objects in domain.", m_objects.size() == 0));

#ifdef _DEBUG
    gc();
    STD_ASSERT(m_value_list.get_count() == 0);
#endif
    m_value_list.free();

    std_delete_event(m_event_id);

    std_enter_critical_section(m_domain_cs);
    m_all_domains->erase(m_id);
    std_leave_critical_section(m_domain_cs);

    // The reference values list should be empty
    ////----        STD_ASSERT(("Objects in domain should be empty.", !m_objects.size()));
    ////----        STD_ASSERT(("Values in domain should be empty.", !m_value_list.get_count()));
}

// Thread enter domain
void Domain::enter()
{
    UintR running;
    running = std_cpu_lock_xchg(&m_running, 1);
    if (running)
    {
        conflict++;
        std_cpu_lock_add(&m_wait_counter, 1);
        while ((running = std_cpu_lock_xchg(&m_running, 1)))
            std_wait_event(m_event_id);
    }
}

// Thread leave domain
void Domain::leave()
{
    m_running = 0;

    std_cpu_mfence();
    if (m_wait_counter)
    {
        std_cpu_lock_add(&m_wait_counter, 1);
        std_raise_event(m_event_id);
    }
}

// Garbage collect
void Domain::gc()
{
    if (!m_value_list.get_count())
        // Value list is empty
        return;

    auto b = std_get_current_us_counter();////----

    auto *thread = Thread::get_current_thread();
    if (thread)
        thread->update_end_sp_of_current_domain_context();

    MarkValueState state(&m_value_list);

    // Mask of valid pointer
    // For all valid pointer, the last N bits should be zero
    const IntPtr mask = sizeof(void *) - 1;

    // Put all values into the map
    auto *p = m_value_list.get_list();
    state.low = p;
    state.high = p;
    while (p)
    {
        // Reset bound of valid pointers
        if (p < state.low)
            state.low = p;
        else
        if (p > state.high)
            state.high = p;

        STD_ASSERT((((IntPtr) p) & mask) == 0);

        // Add to set
        state.set.put(p);

        // For BufferImpl with classes, put the class pointer into set
        if (p->get_type() == BUFFER)
        {
            auto *buffer_impl = (BufferImpl *)p;
            if (buffer_impl->attrib & BufferImpl::CONTAIN_CLASS)
            {
                // This buffer contains class (or array of class)
                // Map first class pointer -> this buffer impl
                state.class_ptrs.put(buffer_impl->class_ptr(), buffer_impl);
            }
        }
        p = p->next;
    }
    m_value_list.reset();

    // Scan all thread contexts of this domain
    for (auto &context: m_context_list)
    {
        // Mark all values
        auto *p = (ReferenceImpl **)context.m_start_sp;
        while (--p > (ReferenceImpl **)context.m_end_sp)
            if (state.is_possible_pointer(*p))
                state.mark_value(*p);
    }

    // Scan all member objects in this domain
    for (auto &object: m_objects)
    {
        auto *p_end = (ReferenceImpl **)object;
        size_t size = object->get_program()->get_entire_object_size();
        auto *p = (ReferenceImpl **)(((char *)p_end) + size);
        while (--p >= p_end)
            if (state.is_possible_pointer(*p))
                state.mark_value(*p);
    }

    // Free all non-refered values & regenerate value list
    for (auto &it: state.set)
    {
        // Value is not refered, free it
        XDELETE(it);
    }

    // Reset gc counter
    m_gc_counter = m_value_list.get_count();
    if (m_gc_counter < 256)
        m_gc_counter = 256;
    else
    if (m_gc_counter > 1024)
        m_gc_counter = 1024;

    auto e = std_get_current_us_counter();
    ////----printf("GC cost: %zuus.\n", (size_t) (e - b));////----
}

// Let object join in domain
void Domain::join_object(Object *ob)
{
    STD_ASSERT(ob->get_domain() == this);
    m_objects.put(ob);
}

// Object was destructed, left domain
void Domain::object_was_destructed(Object *ob)
{
    STD_ASSERT(m_objects.contains(ob));
    m_objects.erase(ob);
}

// Generate a map for detail information
Map Domain::get_domain_detail()
{
    Map map(7);
    map["type"] = m_type;
    map["id"] = m_id;
    map["name"] = m_name;
    map["running"] = m_running;
    map["wait_counter"] = m_wait_counter;
    map["thread_holder_id"] = (size_t)m_thread_holder_id;
    return map;
}

} // End of namespace: cmm
