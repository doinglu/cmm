// cmm_thread.cpp

#include <stdio.h>
#include <stddef.h>
#include "std_port/std_port_compiler.h"
#include "std_port/std_port_mmap.h"
#include "std_port/std_port_os.h"
#include "cmm_domain.h"
#include "cmm_object.h"
#include "cmm_output.h"
#include "cmm_program.h"
#include "cmm_thread.h"

namespace cmm
{

Thread::GetStackPointerFunc Thread::m_get_stack_pointer_func = 0;
size_t Thread::m_max_call_context_level = 128;  // Default
size_t Thread::m_max_domain_context_level = 64; // Default
size_t Thread::m_max_value_stack_size = 1024;   // Default

std_tls_t Thread::m_thread_tls_id = STD_NO_TLS_ID;

ValueStack::ValueStack(size_t size)
{
    auto alloc_size = STD_MEM_PAGE_SIZE * 2 + size * sizeof(Value);
    m_mem = XNEWN(char, alloc_size);

    // Align to next page
    auto* p = (char*)((size_t)(m_mem + STD_MEM_PAGE_SIZE - 1) & ~(STD_MEM_PAGE_SIZE - 1));

    // Get start pointer of value stack
    m_base = (Value*)(p + STD_MEM_PAGE_SIZE);

    // Get size of stack
    m_size = (m_mem + alloc_size - (char*)m_base) / sizeof(Value);
    STD_ASSERT(("Value stack size is smaller than expected.", m_size >= size));

    // Grant privilege to the page (No access in case of stack overflow)
    std_mem_protect(p, STD_MEM_PAGE_SIZE, STD_PAGE_NO_ACCESS);

    m_sp = m_size;
}

ValueStack::~ValueStack()
{
    auto* p = (char*)m_base - STD_MEM_PAGE_SIZE;
    std_mem_protect(p, STD_MEM_PAGE_SIZE, STD_PAGE_READ | STD_PAGE_WRITE);

    XDELETEN(m_mem);
}

#if defined(__GNUC__)
// Disable optimization to compile GetStackPointerFunc
#pragma GCC push_options
#pragma GCC optimize ("O0")
#endif
// Initialize this module
bool Thread::init()
{
    std_allocate_tls(&m_thread_tls_id);

    // Set handler - This is only to prevent optimization
    m_get_stack_pointer_func = (GetStackPointerFunc)([]() { void *p; p = &p; return (void*)p; });

    // Start current thread
    Thread *thread = XNEW(Thread);
    thread->start();

    return true;
}
#if defined(__GNUC__)
#pragma GCC pop_options
#endif

// Shutdown this moudule
void Thread::shutdown()
{
    // Stop current thread
    Thread *thread = Thread::get_current_thread();
    thread->stop();
    XDELETE(thread);

    std_free_tls(m_thread_tls_id);
}

Thread::Thread(const char *name) :
    m_value_stack(m_max_value_stack_size)
{
    // Set default values
    m_current_domain = 0;

    // Set name to this thread
    if (name)
    {
        strncpy(m_name, name, sizeof(m_name));
        m_name[sizeof(m_name) - 1] = 0;
    } else
        snprintf(m_name, sizeof(m_name), "Thread(%zu)", (size_t) std_get_current_task_id());
    m_value_list.set_name(m_name);

    // Initialize call context
    m_all_call_contexts = new CallContext[m_max_call_context_level];
    m_end_call_context = m_all_call_contexts + m_max_call_context_level - 1;
    m_this_call_context = m_all_call_contexts - 1;

    // Initialize domain context
    m_all_domain_contexts = new DomainContextNode[m_max_domain_context_level];
    m_end_domain_context = m_all_domain_contexts + m_max_domain_context_level - 1;
    m_this_domain_context = m_all_domain_contexts - 1;
}

Thread::~Thread()
{
    // Thread is closed
    if (m_value_list.get_count())
    {
        printf("There %zu valus still alive in thread %s.\n",
               m_value_list.get_count(),
               m_name);
        m_value_list.free();
    }
}

// Thread is started
void Thread::start()
{
    auto existed = (Thread *) std_get_tls_data(m_thread_tls_id);
    if (existed)
        throw_error("There was a structure binded to this thread.\n");

    // Bind this
    std_set_tls_data(m_thread_tls_id, this);

    // Create a domain for this thread
    char buf[64];
    snprintf(buf, sizeof(buf), "ThreadDomain(%llx)", (Int64)std_get_current_task_id());
    buf[sizeof(buf) - 1] = 0;
    m_start_domain = XNEW(Domain, buf);
    switch_domain(m_start_domain);

    // Init domain context for this thread
    m_this_domain_context++;
    memset(m_this_domain_context, 0, sizeof(*m_this_domain_context));
    m_this_domain_context->value.m_thread = this;
    m_this_domain_context->value.m_call_context = m_this_call_context;
    m_this_domain_context->value.m_domain = m_current_domain;
    m_this_domain_context->value.m_start_sp = m_value_stack.m_size;
    // Link to domain
    m_current_domain->m_context_list.append_node(m_this_domain_context);
}

// Thread will be stopped
void Thread::stop()
{
    // Verify
    auto existed = (Thread *)std_get_tls_data(m_thread_tls_id);
    if (existed != this)
        throw_error("This structure isn't binded to this thread.\n");

    if (m_value_list.get_count())
    {
        printf("There are still %zu active reference values when thread is stopped.\n",
               m_value_list.get_count());
        m_value_list.free();
    }

    // Destroy start domain & context for convenience
    STD_ASSERT(m_current_domain == m_start_domain);
    STD_ASSERT(m_this_domain_context == m_all_domain_contexts);

    // Dont call pop_domain_context() when thread stopped, removed it
    STD_ASSERT(("There must be existed current_domain.", m_current_domain));
    m_current_domain->m_context_list.remove_node(m_this_domain_context);
    m_this_domain_context--;

    // Remove the thread local domain
    XDELETE(m_start_domain);

    // Unbind
    std_set_tls_data(m_thread_tls_id, 0);
}

// Get function for this context
Function *Thread::get_this_function()
{
    auto *context = get_this_call_context();
    return Program::get_function_by_entry(context->m_function_or_entry);
}

// Push new domain context
void Thread::push_domain_context()
{
    if (m_this_domain_context >= m_end_domain_context)
        throw "Too depth domain context.\n";

    // Update previous end_sp
    size_t arg_p = (m_this_call_context->m_frame - m_value_stack.m_base) + m_this_call_context->m_arg_no;
    m_this_domain_context->value.m_end_sp = arg_p;

    m_this_domain_context++;
    m_this_domain_context->value.m_thread = this;
    m_this_domain_context->value.m_call_context = m_this_call_context;
    m_this_domain_context->value.m_domain = m_current_domain;
    m_this_domain_context->value.m_start_sp = arg_p;
    m_this_domain_context->value.m_end_sp = m_value_stack.m_sp;

    // Link to domain
    m_current_domain->m_context_list.append_node(m_this_domain_context);
}

// Restore previous domain context
// Put the result to ret
void Thread::pop_domain_context()
{
    STD_ASSERT(("There must be existed current_domain.", m_current_domain));
    m_current_domain->m_context_list.remove_node(m_this_domain_context);

    // Get previous domain object & back to previous frame
    m_this_domain_context--;
    Domain *prev_domain = m_this_domain_context->value.m_domain;

    // Domain will be changed, try to copy ret to target domain
    // ATTENTION: Because there may be GC during siwtch_domain, so
    // I put the local value in temp_ret first. It will be concated
    // after domain switched.
    Value temp_ret = m_ret.copy_to_local(this);
    m_ret = TVOID;
    switch_domain(prev_domain);
    transfer_values_to_current_domain();
    m_ret = temp_ret;
}

// Restore when error occurred
// After restored, the "to_call_context" is the current call context
void Thread::restore_call_stack_for_error(CallContext *to_call_context)
{
    auto *call_context = get_this_call_context();

    STD_ASSERT(("Try to restore to bad call context.\n",
               to_call_context >= get_all_call_contexts() - 1));
    m_ret = TVOID;
    while (call_context >= to_call_context)
    {
        // Switch to previous domain
        while (m_this_domain_context > m_all_domain_contexts &&
               m_this_domain_context->value.m_call_context > call_context)
        {
            // Back to previous domain context according the call context
            pop_domain_context();
        }

        call_context--;
    }
    STD_ASSERT(("Try to restore to unknown call context.\n",
               to_call_context == call_context + 1));
    m_this_call_context = to_call_context;
}

// Trace callstack & print it
void Thread::trace_call_stack()
{
    auto *domain_context = get_this_domain_context();
    auto *call_context = get_this_call_context();
    auto *first_call_context = get_all_call_contexts();
    auto *current_domain = get_current_domain();

    while (call_context >= first_call_context)
    {
        // Switch to previous domain
        while (domain_context->prev &&
               domain_context->value.m_call_context > call_context)
        {
            // Back to previous domain context according the call context
            domain_context = domain_context->value.m_prev_context;
            switch_domain(domain_context->value.m_domain);
        }
        // Get the function via entry
        auto *function = Program::get_function_by_entry(call_context->m_function_or_entry);
        auto *object = call_context->m_this_object;
        auto *program = object->get_program();

        char oid_desc[64];
        object->get_oid().print(oid_desc, sizeof(oid_desc), "Object");
        printf("Function %s::%s @ %s(%s)\n",
               function->get_program()->get_name()->c_str(),
               function->get_name()->c_str(),
               program->get_name()->c_str(),
               oid_desc);

        // Print all arguments
        auto n = function->get_max_arg_no();
        if (function->get_attrib() & Function::RANDOM_ARG)
            // For random arg function, m_arg_no in call_context is the
            // count of arugments passed when calling
            n = call_context->m_arg_no;
        auto* args = call_context->m_frame;
        print_variables(function->get_parameters(), "Argument", args, n);

        // Print all local variables
        print_variables(function->get_local_variables(), "Local variables", args, 0);

        call_context--;
    }

    // Return the saved current domain of the thread
    switch_domain(current_domain);
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

// Switch object by oid
// Since the object may be destructed in other thread, I must lock the
// object & switch to it
bool Thread::try_switch_object_by_id(Thread *thread, ObjectId to_oid, ArgNo n)
{
    // ATTENTION:
    // We can these values no matter the entry is freed or allocated, since
    // the memory of entry won't be return to memory pool.
    // HOW EVER, these valus may be changed by other thread during the following
    // operation
    auto *entry = Object::get_entry_by_id(to_oid);
    auto *to_domain = entry->domain;

    if (m_current_domain != to_domain)
    {
        // Domain will be changed
        // Copy arguments to thread local value list & pass to target
        auto* args = thread->get_stack_top();
        for (ArgNo i = 0; i < n; i++)
            args[i] = args[i].copy_to_local(thread);

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
    auto *ob = entry->object;
    if (ob && ob->get_oid() == to_oid && ob->get_domain() == to_domain)
    {
        // OK, switch to right domain
        m_current_domain = to_domain;
        thread->transfer_values_to_current_domain();
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

    // Transfer the value no matter the domain to avoid them left on thread
    thread->transfer_values_to_current_domain();
    return false;
}

// Drop all values in local value list
void Thread::free_values()
{
    m_value_list.free();
}

// Return context of this thread
// ({
//     ([
//         "name" : <Domain_name>
//         "id" : <Domain_id>
//         ...
//         "stack_top" : <Top address of stack frame>
//         "stack_bottom" : <Bottom address of stack frame>
//     ])
//     ...
// })
Value& Thread::get_domain_context_list(Value* ptr)
{
    // Get count of context list
    size_t count = 0;
    auto *p = m_this_domain_context;
    while (p)
    {
        count++;
        p = p->prev;
    }

    // Update the end_sp
    update_end_sp_of_current_domain_context();

    auto r = ReserveStack(4);
    Map& detail = (Map&)r[0];
    Value& temp = r[1];
    Value& start_sp = r[2];
    Value& end_sp = r[3];
    Array& arr = (Array&)*ptr;

    // Allocate array for return
    *ptr = XNEW(ArrayImpl, count);
    p = m_this_domain_context;
    while (p)
    {
        p->value.m_domain->get_domain_detail(&detail);
        detail.set(temp = "stack_begin", start_sp = (Integer)p->value.m_start_sp);
        detail.set(temp = "stack_end", end_sp = (Integer)p->value.m_end_sp);
        arr.push_back(detail);
        p = p->prev;
    }

    return arr;    
}

// Transfer all values in local value list to current domain
// This routine should be invoked after having switched to a domain
void Thread::transfer_values_to_current_domain()
{
    if (m_value_list.get_count() && m_current_domain)
        m_current_domain->concat_value_list(&m_value_list);
}

} // End of namespace: cmm
