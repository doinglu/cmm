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
bool Domain::init()
{
    std_new_critical_section(&m_domain_cs);

    // Create the id manager
    m_id_manager = XNEW(DomainIdManager, MAX_ID_PAGES);

    // Create the domain 0
    m_all_domains = XNEW(DomainIdMap);
    m_domain_0 = XNEW(Domain, "Zero");
    return true;
}

// Shutdown this moudule
void Domain::shutdown()
{
    // Clear all domains
    auto domains = m_all_domains->values();
    for (auto& it: domains)
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
    auto* entry = m_id_manager->allocate_id();
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
    m_value_list.set_name(m_name);

    // Register me
    std_enter_critical_section(m_domain_cs);
    m_all_domains->put(m_id, this);
    std_leave_critical_section(m_domain_cs);
}

Domain::~Domain()
{
    // Remove all objects
    auto objects = m_objects.to_array();
    for (auto& it: objects)
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
    auto* thread = Thread::get_current_thread();
    if (thread)
        thread->update_end_sp_of_current_domain_context();
    gc_internal(thread);
}

void Domain::gc_internal(Thread* thread)
{
    if (!m_value_list.get_count())
        // Value list is empty
        return;

    auto b = std_get_current_us_counter();////----

    MarkValueState state(&m_value_list);
////----    printf("Values before GC = %zu\n", m_value_list.get_count());////----

    auto b1 = std_get_current_us_counter();////----
    // Scan all thread contexts of this domain
    for (auto& context: m_context_list)
    {
        // Mark all values in stack
        size_t bp = context.m_start_sp;
        size_t ep = context.m_end_sp;
        Value* value_ptr = context.m_thread->get_stack_offset(bp - 1);
        for (size_t i = bp; i > ep; i--, value_ptr--)
            if (value_ptr->is_reference_value())
                state.mark_value(value_ptr->m_reference);
    }

    // Thread.m_ret should be NIL when GC
    STD_ASSERT(!thread || thread->get_ret().m_type == NIL);

    // Scan all member objects in this domain
    for (auto& object: m_objects)
        object->get_program()->mark_value(state, object);
    auto e1 = std_get_current_us_counter();////----
    ////----printf("GC mark: %zuus.\n", (size_t)(e1 - b1));////----

#if USE_LIST_IN_VALUE_LIST
    // Get pointer of pointer to first node 
    ReferenceImpl** pp = &state.value_list->get_container().begin().get_node()->prev->next;
    ReferenceImpl* p;
    while ((p = *pp)->next)
    {
        // Not end stub node, check this node
        if (!p->owner)
        {
            // Keep this
            p->owner = state.value_list;
            pp = &p->next;
        } else
        {
            // Drop this, don't update pp since *pp will be changed
            STD_ASSERT(("Value is not belonged to the list when free.", p->owner == state.value_list));
            p->unbind();
            XDELETE(p);
        }
    }
#elif USE_VECTOR_IN_VALUE_LIST
    // Free all non-refered values & regenerate value list
    ReferenceImpl** head_address = state.value_list->get_head_address();
    // Sort state.list to
    // [Valid] [Valid] .... [Valid] [Free] [Free] ... [Free]
    // 0                            offset              size()
    size_t offset = 0;
    size_t size = state.value_list->get_count();
    for (auto i = 0; i < size; i++)
    {
        auto* p = head_address[i];
        if (p->owner == 0)
        {
            // Swap valid [i] with [offset]
            p->owner = state.value_list;
            p->offset = offset;
            head_address[offset]->offset = i;
            simple::swap(head_address[offset], head_address[i]);
            offset++;
        }
    }

    for (auto i = offset; i < size; i++)
    {
        if (head_address[i])
        {
            // Erase the owner to prevent calling unbind when destructing
            head_address[i]->owner = 0;
            XDELETE(head_address[i]);
        }
    }
    STD_ASSERT(("Value list is not correct after GC.", state.container->size() >= offset));
    state.container->shrink(offset);
#else
    auto& list = m_value_list.get_container();
    for (auto it = list.begin(); it != list.end();)
    {
        auto* p = *it;
        if (p->owner != 0)
        {
            // Unused, free it
            p->owner = 0;
            XDELETE(p);
            list.erase(it);
            continue;
        } else
            // Set back to owner
            p->owner = &m_value_list;
        ++it;
    }
#endif

    // Reset gc counter
    m_gc_counter = (IntR)m_value_list.get_count();
    if (m_gc_counter < 2048)
        m_gc_counter = 2048;
    else
    if (m_gc_counter > 4*1024*1024)
        m_gc_counter = 4*1024*1024;

    auto e = std_get_current_us_counter();
    ////----printf("GC cost: %zuus (alive: %zu).\n", (size_t)(e - b), m_value_list.get_count());////----
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
Map& Domain::get_domain_detail(Value* map)
{
    auto* thread = Thread::get_current_thread();
    auto r = ReserveStack(1, thread);
    Value& val = r[0];
    Value& key = r[1];
    *map = XNEW(MapImpl, 7);
    map->set(key = "type", val = m_type);
    map->set(key = "id", val = m_id);
    map->set(key = "name", val = m_name);
    map->set(key = "running", val = m_running);
    map->set(key = "wait_counter", val = m_wait_counter);
    map->set(key = "thread_holder_id", val = (size_t)m_thread_holder_id);
    return (Map&)*map;
}

} // End of namespace: cmm
