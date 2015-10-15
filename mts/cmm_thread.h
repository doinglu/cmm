// cmm_thread.h
// For thread in cmm
#pragma once

#include "std_port/std_port.h"
#include "std_template/simple_list.h"

#include "cmm_basic_types.h"
#include "cmm_object.h"
#include "cmm_value.h"
#include "cmm_value_list.h"

namespace cmm
{

class Domain;
class Object;
class Thread;

// VM function call context
// This is not normal value in VM stack, it will store the function call
// context
// This class should be stored in OS stack by alloca(), it should never
// be delete but by call pop_context()
class CallContext
{
friend Thread;
friend simple::list_node<CallContext>;

private:
    // Constructor for Thread start CallContext only
    CallContext(Thread *thread);

public:
    CallContext(Thread *thread, Object *this_object, ComponentNo this_component,
                Value* arg, ArgNo arg_count,
                Value* local, ArgNo local_count);

    // Don't define destructor since it should never be called

public:
    // Pop context & return to previous one
    void pop_context();

    // Update end_sp of this context
    void update_end_sp(void *end_sp)
    {
        m_end_sp = end_sp;
    }

private:
    // Linked to previous context in current thread
    Domain* m_domain;
    simple::list_node<CallContext> *m_prev_context;

public:
    Value*  m_arg;                 // Arguments
    ArgNo   m_arg_count;           // Count of arguments
    Value*  m_local;               // Local variables
    ArgNo   m_local_count;         // Count of local variables
	Value   m_ret;				   // Receive return value
    Object *m_this_object;         // This object
    ComponentNo m_this_component;  // This component no
    Object *m_domain_object;       // Object which in domain
    void   *m_start_sp;            // Start stack from pointer
    void   *m_end_sp;              // End stack frame pointer
    Thread *m_thread;              // In thread
};

// Define the node of CallContext
typedef simple::list_node<CallContext> CallContextNode;

// Thread context data
// Stack:
//   The stack grows upwards. (Not same as normal stack that grows downwards) 
//   It's for convenience that to dynamic re-allocate the stack.
//   m_stack[m_sp] is to store the next pushed value.
class Thread
{
friend CallContext;

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
    Thread(simple::string name = "");
    ~Thread();

    // Thread is started
    void start();

    // Thread will be stopped
    void stop();

public:
    // Return argument, n in [0..arg_count-1]
    Value& get_arg(ArgNo n)
    {
        if (n >= m_context->value.m_arg_count)
            throw "No such argument.";

        return get_arg_unsafe(n);
    }

    // Return argument without safety check
    Value& get_arg_unsafe(ArgNo n)
    {
        return m_context->value.m_arg[n];
    }

    // Get local variable without safety check, n in [0..local_count-1]
    Value& get_local_unsafe(ArgNo n)
    {
        return m_context->value.m_local[n];
    }

	// Get address to strored the return value
	Value& get_ret()
	{
		return m_context->value.m_ret;
	}

    // Return current domain of this object
    inline Domain *get_current_domain()
    {
        return m_current_domain;
    }

    // Return this component
    ComponentNo get_this_component_no()
    {
        return m_context->value.m_this_component;
    }

    // Return this context
    CallContextNode *get_this_context()
    {
        return m_context;
    }

    // Return this object
    Object *get_this_object()
    {
        return m_context->value.m_this_object;
    }

public:
    // Enter function call
    void enter_function_call(CallContextNode *context);

    // Leave function call
    Value leave_function_call(CallContextNode *context, Value& ret);

    // Switch execution ownership to a new domain
    void switch_domain(Domain *to_domain);

    // Switch execution ownership to a new domain
    void switch_object(Object *to_object);

    // Try to switch execution ownership to a new domain by oid
    bool try_switch_object_by_id(Thread *thread, ObjectId to_oid, Value *args, ArgNo n);

    // Will change domain?
    bool will_change_domain(Domain *to_domain);

public:
    // Bind value to local memory list
    void bind_value(ReferenceImpl *value)
    {
        m_value_list.append_value(value);
    }

private:
    // Transfer all values in local value list to current domain
    void transfer_values_to_current_domain();

private:
    // Thread name
    simple::string m_name;

    // Local memory list for this thread
    ValueList m_value_list;

    // Current function context frame
    CallContextNode *m_context;

    // Current domain
    // ATTENTION:
    // Current domain doesn't same as m_context->value.domain, since
    // m_context->value.domain is the domain that owned the function in context, but
    // m_current_domain is "CURRENT" one.
    // When a function trying to invoke another domain function, the m_current_domain
    // will be updated before enter the new domain.
    Domain *m_current_domain;

private:
    // Start domain for convenience
    Domain *m_start_domain;

    // Start CallContextNode for convenience
    CallContextNode *m_start_context;

private:
    static std_tls_t m_thread_tls_id;
};

} // End of namespace: cmm
