// cmm_thread.h
// For thread in cmm
#pragma once

#include "std_port/std_port.h"
#include "std_template/simple_list.h"

#include "cmm.h"
#include "cmm_object.h"
#include "cmm_value.h"
#include "cmm_value_list.h"

namespace cmm
{

class CallContext;
class Domain;
class Function;
class Object;
class Thread;

// VM domain context
// We save context when switching domain for tracing & GC (it needs to walk
// through all domain contexts)
class DomainContext
{
friend Thread;
friend simple::list_node<DomainContext>;

private:
    // For Thread::start() only
    DomainContext() { };

public:
    DomainContext(Thread *thread);

    // Don't define destructor since it should never be called

public:
    // Linked to previous domain context in current thread
    simple::list_node<DomainContext> *m_prev_context;
    Domain* m_domain;               // In which domain
    Thread *m_thread;               // In which thread
    void   *m_start_sp;             // Start stack from pointer
    void   *m_end_sp;               // End stack frame pointer
    CallContext *m_call_context;    // First call context of this domain context
};

// Tiny context of function call
// ATTENTION: Why we don't save component_no?
// We can derive the component_no from function entry
// ATTENTION: Why we don't save Function *? instead of m_entry
// We can derive the Function from entry. But for call_near, we can not
// get the Function * quickly.
class CallContext
{
public:
    Value      *m_args;
    Value      *m_locals;
    void       *m_function_or_entry;// Function or function entry (for local call only)
    Object     *m_this_object;      // This object of context
    ComponentNo m_component_no;     // Component no in this object
    ArgNo       m_arg_no;           // Real arguments (valid only for RANDOM_ARG)
};

// Define the node of DomainContext
typedef simple::list_node<DomainContext> DomainContextNode;

// Thread context data
// Stack:
//   The stack grows upwards. (Not same as normal stack that grows downwards) 
//   It's for convenience that to dynamic re-allocate the stack.
//   m_stack[m_sp] is to store the next pushed value.
class Thread
{
friend DomainContext;

public:
    typedef Uint32  Id;

public:
    // Initialize this module
    static int init();

    // Shutdown this moudule
    static void shutdown();

    // Get data binded to current thread
    static Thread *get_current_thread()
    {
        return (Thread *) std_get_tls_data(m_thread_tls_id);
    }

    // Get domain of current thread
    static Domain *get_current_thread_domain()
    {
        return get_current_thread()->get_current_domain();
    }

public:
    Thread(const char *name = 0);
    ~Thread();

public:
    // Thread is started
    void start();

    // Thread will be stopped
    void stop();

    // Update current context sp
    void update_end_sp_of_current_domain_context()
    {
        // Update end_sp of current context
        if (!m_this_domain_context)
            return;
        void *stack_pointer = m_get_stack_pointer_func();
        m_this_domain_context->value.m_end_sp = stack_pointer;
    }

public:
    // Return argument without safety check
    Value& get_arg_unsafe(ArgNo n)
    {
        return m_this_call_context->m_args[n];
    }

    // Get local variable without safety check, n in [0..local_count-1]
    Value& get_local_unsafe(ArgNo n)
    {
        return m_this_call_context->m_locals[n];
    }

    // Get all call contexts
    CallContext *get_all_call_contexts()
    {
        return m_all_call_contexts;
    }

    // Get all domain contexts
    DomainContextNode *get_all_domain_contexts()
    {
        return m_all_domain_contexts;
    }

    // Return current domain of this object
    inline Domain *get_current_domain()
    {
        return m_current_domain;
    }

    // Get end call context (the last one)
    CallContext *get_end_call_context()
    {
        return m_end_call_context;
    }

    // Get end domain context (the last one)
    DomainContextNode *get_end_domain_context()
    {
        return m_end_domain_context;
    }

    // Get this call context
    CallContext *get_this_call_context()
    {
        return m_this_call_context;
    }

    // Return this component
    ComponentNo get_this_component_no()
    {
        return m_this_call_context->m_component_no;
    }

    // Get this domain context
    DomainContextNode *get_this_domain_context()
    {
        return m_this_domain_context;
    }

    // Get this function
    Function *get_this_function();

    // Return this object
    Object *get_this_object()
    {
        return m_this_call_context->m_this_object;
    }

    // Push new context of current function call
    // For call_near, function_or_entry is the &AbstractComponent::*Func
    // For others, functino_or_entry is class Function *
    void push_call_context(Object *ob, void *function_or_entry, Value *args, ArgNo argn, ComponentNo component_no)
    {
        if (m_this_call_context >= m_end_call_context)
            throw "Too depth call context.\n";
        m_this_call_context++;
        m_this_call_context->m_function_or_entry = function_or_entry;
        m_this_call_context->m_args = args;
        m_this_call_context->m_arg_no = argn;
        m_this_call_context->m_this_object = ob;
        m_this_call_context->m_component_no = component_no;
        // Don't init locals, it should be updated after entered function
    }

    // Push new domain context
    void push_domain_context(void *sp);

    // Restore previous function call context
    void pop_call_context()
    {
        m_this_call_context--;
    }

    // Restore previous domain context
    Value pop_domain_context(const Value& ret);

    // Restore call context when error occurred
    void restore_call_stack_for_error(CallContext *to_call_context);

    // Trace callstack & print it
    void trace_call_stack();

public:
    // Switch execution ownership to a new domain
    void switch_domain(Domain *to_domain);

    // Try to switch execution ownership to a new domain by oid
    bool try_switch_object_by_id(Thread *thread, ObjectId to_oid, Value *args, ArgNo n);

public:
    // Bind value to local memory list
    void bind_value(ReferenceImpl *value)
    {
        m_value_list.append_value(value);
    }

    // Drop local values & free them
    void free_values();

    // Is the value list empty?
    size_t get_value_list_count()
    {
        return m_value_list.get_count();
    }

public:
    // Return domain context of this thread
    Value get_domain_context_list();

public:
    // Configurations
    static void set_max_call_context_level(size_t level)
    {
        m_max_call_context_level = level;
    }

private:
    // Transfer all values in local value list to current domain
    void transfer_values_to_current_domain();

private:
    // Thread name
    char m_name[32];

    // Local memory list for this thread
    ValueList m_value_list;

    // Function call context
    CallContext *m_all_call_contexts;
    CallContext *m_end_call_context;
    CallContext *m_this_call_context;

    // Function call cross domains
    DomainContextNode *m_all_domain_contexts;
    DomainContextNode *m_end_domain_context;
    DomainContextNode *m_this_domain_context;


    // Current domain
    // ATTENTION:
    // Current domain doesn't same as m_domain_context->value.domain, since
    // m_domain_context->value.domain is the domain that owned the function in
    // context, but m_current_domain is "CURRENT" one.
    // When a function trying to invoke another domain function, the m_current_domain
    // will be updated before enter the new domain.
    Domain *m_current_domain;

private:
    // Start domain for convenience
    Domain *m_start_domain;

    // Start DomainContextNode for convenience
    DomainContextNode *m_start_domain_context;

private:
    static std_tls_t m_thread_tls_id;

    // Function routine to get current stack pointer
    typedef void *(*GetStackPointerFunc)();
    static GetStackPointerFunc m_get_stack_pointer_func;

private:
    // Configurations
    static size_t m_max_call_context_level;
    static size_t m_max_domain_context_level;
};

} // End of namespace: cmm
