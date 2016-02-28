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

// Reserve Values as local variables
#define __RESERVE_LOCAL(n) \
Value* __local = STD_ALLOCA(sizeof(Value)*(n)); \
memset(__local, 0, sizeof(Value)*(n)); \
_thread->get_this_call_context()->m_locals = __local; \
_thread->get_this_call_context()->m_local_no = (LocalNo)n;

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
    size_t  m_start_sp;             // Start stack from pointer
    size_t  m_end_sp;               // End stack frame pointer
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
    void*       m_function_or_entry;// Function or function entry (for local call only)
    Object*     m_this_object;      // This object of context
    Value*      m_frame;            // Frame pointer of value stack
    ComponentNo m_component_no;     // Component no in this object
    ArgNo       m_arg_no;           // Arguments count
    LocalNo     m_local_no;         // Local variables count
};

// Value stack
// To store all local values in runtime
// The stack is grows upward, for convenience of appending optional arguments
class ValueStack
{
public:
    char*       m_mem;
    Value*      m_base;
    size_t      m_sp;
    size_t      m_size;

public:
    ValueStack(size_t size);
    ~ValueStack();

public:
    // Assure there is at least n values space
    void assure(size_t n)
    {
        // Do nothing
        if (m_sp < n)
            throw_error("Value stack overflow.\n");
    }

    // Reserve n values (init to NIL)
    void reserve(size_t n)
    {
        STD_ASSERT(("Value stack overflow, use assure() before reserve.", m_sp >= n));
        m_sp -= n;
        memset(&m_base[m_sp], 0, sizeof(Value) * n);
    }

    // Reserve n values without init
    void reserve_without_init(size_t n)
    {
        STD_ASSERT(("Value stack overflow, use assure() before reserve.", m_sp >= n));
        m_sp -= n;
    }
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
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

public:
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
        if (m_this_domain_context < m_all_domain_contexts)
            return;
        m_this_domain_context->value.m_end_sp = m_value_stack.m_sp;
    }

public:
    // Return argument without safety check
    Value& get_arg(ArgNo n)
    {
        STD_ASSERT(("Argument out of range.", n < m_this_call_context->m_arg_no));
        return m_this_call_context->m_frame[n];
    }

    // Get local variable without safety check, n in [-local_count..-1]
    Value& get_local(LocalNo n)
    {
        STD_ASSERT(("Local value out of range.", m_this_call_context->m_frame + n >= get_stack_top()));
        return m_this_call_context->m_frame[n];
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
    void push_call_context(Object *ob, void *function_or_entry, ArgNo argn, ComponentNo component_no)
    {
        if (m_this_call_context >= m_end_call_context)
            throw "Too depth call context.\n";
        m_this_call_context++;
        m_this_call_context->m_function_or_entry = function_or_entry;
        m_this_call_context->m_arg_no = argn;
        m_this_call_context->m_local_no = 0;
        m_this_call_context->m_this_object = ob;
        m_this_call_context->m_component_no = component_no;
        m_this_call_context->m_frame = m_value_stack.m_base + m_value_stack.m_sp;
        // Don't init locals, it should be updated after entered function

        // Default return TVOID
        m_ret = TVOID;
    }

    // Push new domain context
    void push_domain_context();

    // Restore previous function call context
    void pop_call_context(Value* ret)
    {
        // Reset m_ret after pop context
        *ret = m_ret;
        m_ret = TVOID;
        m_value_stack.m_sp = m_this_call_context->m_frame + m_this_call_context->m_arg_no - m_value_stack.m_base;
        m_this_call_context--;
    }

    // Restore previous domain context
    void pop_domain_context();

    // Restore call context when error occurred
    void restore_call_stack_for_error(CallContext *to_call_context);

    // Trace callstack & print it
    void trace_call_stack();

public:
    // Switch execution ownership to a new domain
    void switch_domain(Domain *to_domain);

    // Try to switch execution ownership to a new domain by oid
    bool try_switch_object_by_id(Thread *thread, ObjectId to_oid, ArgNo n);

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
    Value& get_domain_context_list(Value* ptr);

    // Value stacks operation
public:
    // Assure there is at least n values space in value stack
    void assure_stack(size_t n)
    {
        m_value_stack.assure(n);
    }

    // Get base pointer for current stack frame
    Value* get_base_pointer()
    {
        return m_this_call_context->m_frame;
    }

    // Restore value stack
    void pop_stack(size_t n)
    {
        STD_ASSERT(("Overpop current stack frame.\n",
            m_this_call_context < m_all_call_contexts ||
            m_value_stack.m_base + m_value_stack.m_sp + n <= m_this_call_context->m_frame));
        m_value_stack.m_sp += n;
    }

    // Push value to stack
    void push(const Value& value)
    {
        assure_stack(1);
        m_value_stack.m_base[--m_value_stack.m_sp] = value;
    }

    // Push n values to stack
    void push_n(const Value* ptr_value, size_t n)
    {
        assure_stack(n);
        m_value_stack.m_sp -= n;
        for (auto i = 0; i < n; i++)
            m_value_stack.m_base[m_value_stack.m_sp + i] = ptr_value[i];
    }

    // Caller assure the stack is reserved
    void push_unsafe(const Value& value)
    {
        STD_ASSERT(("Stack is not reserved with enough space.", m_value_stack.m_sp > 0));
        m_value_stack.m_base[--m_value_stack.m_sp] = value;
    }

    // Reserve n values of value stack
    Value* reserve_stack(size_t n)
    {
        m_value_stack.reserve(n);
        return get_stack_top();
    }

    // Reserve n values of value stack without init
    Value* reserve_stack_without_init(size_t n)
    {
        m_value_stack.reserve_without_init(n);
        return get_stack_top();
    }

    // Get value address
    Value* get_stack_offset(size_t index)
    {
        return &m_value_stack.m_base[index];
    }

    // Get top address
    Value* get_stack_top()
    {
        return &m_value_stack.m_base[m_value_stack.m_sp];
    }

    // Set current function's return value
    void set_ret(const Value& value)
    {
        m_ret = value;
    }

    // Get current function's return value
    const Value& get_ret()
    {
        return m_ret;
    }

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
    char         m_name[32];

    // Local memory list for this thread
    ValueList    m_value_list;

    // Value stack
    ValueStack   m_value_stack;

    // Return value of current function
    Value        m_ret;

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

private:
    static std_tls_t m_thread_tls_id;

    // Function routine to get current stack pointer
    typedef void *(*GetStackPointerFunc)();
    static GetStackPointerFunc m_get_stack_pointer_func;

private:
    // Configurations
    static size_t m_max_call_context_level;
    static size_t m_max_domain_context_level;
    static size_t m_max_value_stack_size;
};

// Class to reserve local stack in a function
class ReserveStack
{
public:
    ReserveStack(size_t n, Thread* thread = 0)
    {
        if (!thread)
            thread = Thread::get_current_thread();

        m_stack_ptr = thread->reserve_stack(n);
        m_reserved = n;
        m_thread = thread;
#ifdef _DEBUG
        m_sp = m_stack_ptr - thread->get_stack_offset(0);
#endif
    }

    ~ReserveStack()
    {
        m_thread->pop_stack(m_reserved);
    }

public:
    Value& operator [](size_t index)
    {
        STD_ASSERT(("Local index is out of range.", index < m_reserved));
        return m_stack_ptr[index];
    }

private:
    Thread* m_thread;
    Value*  m_stack_ptr;
    size_t  m_reserved;
#ifdef _DEBUG
    size_t  m_sp;
#endif
};

} // End of namespace: cmm
