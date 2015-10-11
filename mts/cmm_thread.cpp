// cmm_thread.cpp

#include <stdio.h>
#include "std_port/std_port_compiler.h"
#include "std_port/std_port_os.h"
#include "cmm_domain.h"
#include "cmm_object.h"
#include "cmm_thread.h"

namespace cmm
{

// Constructor
CallContext::CallContext(Thread *thread,
    Object *this_object, ComponentNo this_component,
    Value *arg, ArgNo arg_count,
    Value *local, ArgNo local_count) :
    m_prev_context(thread->m_context),
    m_domain(thread->get_current_domain()),
    m_arg(arg),
    m_arg_count(arg_count),
    m_local(local),
    m_local_count(local_count),
    m_this_object(this_object),
    m_this_component(this_component),
    m_end_sp(&local_count),
    m_thread(thread)
{
    // Update end_sp of previous context
    if (m_prev_context)
        m_prev_context->value.m_end_sp = &local_count;

    // Save object in domain in this context
    if (this_object->get_domain())
        m_domain_object = this_object;
    else
        m_domain_object = m_prev_context ? m_prev_context->value.m_domain_object : 0;

    // Link to previous context
    CallContextNode *node;
    void *ptr_node = ((Uint8 *)this) - (size_t)&((CallContextNode *)0)->value;
    node = static_cast<CallContextNode *>(ptr_node);
    thread->m_context = node;
}

// CallContext has no destructor, use this routine instead
void CallContext::pop_context()
{
}

std_tls_t Thread::m_thread_tls_id = STD_NO_TLS_ID;

// Initialize this module
int Thread::init()
{
    std_allocate_tls(&m_thread_tls_id);
    return 0;
}

// Shutdown this moudule
void Thread::shutdown()
{
    std_free_tls(m_thread_tls_id);
}

Thread::Thread(simple::string name)
{
    // Set default values
    m_context = 0;
    m_current_domain = 0;

    // Set name to this thread
    if (name.length())
        m_name = name;
    else
        m_name.snprintf("(%zu)", 64, (size_t) std_get_current_task_id());
}

Thread::~Thread()
{
    // Thread is closed
    if (m_value_list.get_count())
    {
        printf("There %zu valus still alive in thread %s.\n",
               m_value_list.get_count(),
               m_name.c_str());
        auto *p = m_value_list.get_list();
        while (p)
        {
            auto *current = p;
            p = p->next;
            delete current;
        }
        m_value_list.reset();
    }
}

// Thread is started
void Thread::start()
{
    auto existed = (Thread *) std_get_tls_data(m_thread_tls_id);
    if (existed)
        throw "There was a structure binded to this thread.";

    // Bind this
    std_set_tls_data(m_thread_tls_id, this);
}

// Thread will be stopped
void Thread::stop()
{
    // Verify
    auto existed = (Thread *)std_get_tls_data(m_thread_tls_id);
    if (existed != this)
        throw "This structure isn't binded to this thread.";

    if (m_value_list.get_count())
    {
        printf("There are still %zu active reference values when thread is stopped.\n",
               m_value_list.get_count());
        drop_local_values();
    }

    // Unbind
    std_set_tls_data(m_thread_tls_id, 0);
}

// Enter function call
void Thread::enter_function_call(CallContextNode *context)
{
    // Put this context in domain
    if (m_current_domain)
        m_current_domain->m_context_list.append_node(context);
}

// Leave function call
// Pop stack & restore function context
Value Thread::leave_function_call(CallContextNode *context, const Value& ret)
{
    if (m_current_domain)
        m_current_domain->m_context_list.remove_node(context);

    auto c = &context->value;

    // Get previous domain object & back to previous frame
    Object *prev_domain_object = c->m_prev_context ? c->m_prev_context->value.m_domain_object : 0;
    c->m_thread->m_context = c->m_prev_context;

    if (!will_change_domain(prev_domain_object))
        // Domain won't be changed, return "ret" directly
        return ret;

    // Domain will be changed, try to copy ret to target domain
    Value dup_ret = duplicate_value_to_local(ret);
    c->m_thread->switch_object(prev_domain_object);
    transfer_values_to_current_domain();
    return dup_ret;
}

// Switch execution ownership to a new domain
void Thread::switch_domain(Domain *to_domain)
{
    if (m_current_domain == to_domain)
        // No target domain or domain is not switched
        return;

    if (m_current_domain)
        // Leave previous domain
        m_current_domain->leave();

    if (to_domain)
        // Enter new domain
        to_domain->enter();

    m_current_domain = to_domain;
}

// Switch execution ownership to a new domain
void Thread::switch_object(Object *to_object)
{
    Domain *to_domain = to_object ? to_object->get_domain() : 0;
    switch_domain(to_domain);
}

// Switch object by oid
// Since the object may be destructed in other thread, I must lock the
// object & switch to it
bool Thread::try_switch_object_by_id(ObjectId to_oid)
{
    // ATTENTION:
    // We can these values no matter the entry is freed or allocated, since
    // the memory of entry won't be return to memory pool.
    // HOW EVER, these valus may be changed by other thread during the following
    // operation
    auto *entry = Object::get_entry_by_id(to_oid);
    auto *ob = entry->object;
    auto *to_domain = entry->domain;

    if (m_current_domain != to_domain)
    {
        // OK, Switch the domain first
        if (m_current_domain)
            // Leave previous domain
            m_current_domain->leave();

        if (to_domain)
            // Enter new domain
            to_domain->enter();
    }

    // ATTENTION:
    // Now, after entered the "to_domain". I can verify the values since the those
    // won't be changed if they are belonged to "to_domain"
    if (ob && ob->get_oid() == to_oid && ob->get_domain() == to_domain)
    {
        // OK, switch to right domain
        m_current_domain = to_domain;
        return true;
    }

    // Switch to wrong domain
    STD_TRACE("! Switch interrupt, ob: %p(%llx), expect: %llx.\n",
              ob, ob->get_oid().i64, to_oid.i64);

    // Rollback, return to previous domain
    if (to_domain)
        // Leave new domain
        to_domain->leave();

    if (m_current_domain)
        // Enter previous domain
        m_current_domain->enter();
    return false;
}

// Should I change domain if enter target object?
bool Thread::will_change_domain(Object *to_object)
{
    Domain *to_domain = to_object ? to_object->get_domain() : 0;

    if (m_current_domain == to_domain)
        // No target domain or domain is not switched
        return false;

    // I will change domain
    return true;
}

// Drop local values
void Thread::drop_local_values()
{
    m_value_list.free();
}

// Duplicate value to local value list
// When call/ret to other domain, we need copy the value to local value list first
// Then use "transfer_values_to_current_domain" after having switched
Value Thread::duplicate_value_to_local(const Value& value)
{
    return value.copy_to_local(this);
}

// Transfer all values in local value list to current domain
// This routine should be invoked after having switched to a domain
void Thread::transfer_values_to_current_domain()
{
    if (m_value_list.get_count() && m_current_domain)
        m_current_domain->concat_value_list(&m_value_list);
}

} // End of namespace: cmm
