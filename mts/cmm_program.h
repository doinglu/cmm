// cmm_function.h
// Component & function relatives

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm_basic_types.h"
#include "cmm_object.h"
#include "cmm_string_pool.h"
#include "cmm_typedef.h"
#include "cmm_value.h"

namespace cmm
{

class Domain;
class Function;
class Object;
class Program;
class Thread;

// Abstract class, all program class are derived from this class
class AbstractComponent
{
};

// Parameter of function
class Parameter
{
friend Function;

public:
	typedef enum
	{
		NULLABLE = 1,
	} Attrib;

public:
    // Only private constructor, it should be created by Function only
    Parameter(Function *function, const simple::string& name);

private:
	String      *m_name;     // Point to string pools in Program
	Value::Type  m_type;     // Type of this argument
	Attrib       m_attrib;   // Attrib of this parmeters
};

// Function
class Function
{
friend Program;

public:
	typedef enum
	{
		RANDOM_ARG = 1,
	} Attrib;

    // Type of entry routine
    // Value Entry(Thread *, Value *, size_t);
    // Why use void *?
    // Since Entry would be routine in a class (can't be decided now), so
    // we use void * instead.
    typedef Value(AbstractComponent::*Entry)(Thread *, Object *, ComponentNo, Value *, ArgNo);

public:
    // Only private constructor, it should be created by Program only
    Function(Program *program, const simple::string& name);

public:
    // Create new parameter
    Parameter *define_parameter(const simple::string& name);

public:
    // Get the entry pointer
    Entry get_func_entry() { return m_func_entry; }

private:
    Program *m_program;
    String  *m_name;
	ArgNo    m_min_arg_no;
	ArgNo    m_max_arg_no;
	Attrib   m_attrib;

	// Parameters
	simple::vector<Parameter *> m_parameters;

    // Entry
    Entry    m_func_entry;
};

// Member of program
class Member
{
friend Program;

public:
    Member(Program *program, const simple::string& name);

private:
    Program     *m_program;
    String      *m_name;
    Value::Type  m_type;
    MemberOffset m_offset;
};

// Program of object
class Program
{
public:
    typedef enum
    {
        COMPILED_TO_NATIVE = 1,     // This component was compiled to native class
    } Attrib;

    typedef Object *(*NewInstanceFunc)();

    // Callee info of a function in program
    struct CalleeInfo
    {
        Function *function;
        ComponentNo component_no;
    };

    // Component info
    struct ComponentInfo
    {
        String *program_name;
        Program *program;
        ComponentOffset offset;
    };

public:
    static int init();
    static void shutdown();

public:
    Program(const simple::string& name);
    ~Program();

public:
    // Convert a normal String pointer to shared String pointer
    // The *pp_string would be updated if found in pool
    static String *convert_to_shared(String **pp_string);
        
    // Find or add a string into pool
    static String *find_or_add_string(const simple::string& str);

    // Find string in pool (return 0 if not found)
    static String *find_string(const simple::string &str);

    // Find a program by name (shared string)
    static Program *find_program_by_name(String *program_name);

    // Update callees of all programs
    static void update_all_callees();

public:
    // Decribe the object properties
    void define_object(size_t object_size)
    {
        m_object_size = object_size;
    }

    // Create function in this program
    Function *define_function(const simple::string& name,
                              Function::Entry func_entry,
                              ArgNo min_arg_no, ArgNo max_arg_no,
                              Function::Attrib attrib = (Function::Attrib)0);

    // Create member in this program
    Member *define_member(const simple::string& name, Value::Type type, MemberOffset offset);

public:
    // Add a function to callee map
    void add_callee(ComponentNo component_no, Function *function);
        
    // Add component in this program
    void add_component(const simple::string& program_name, ComponentOffset offset);

    // Set function handler: new_instance
    void set_new_instance_func(NewInstanceFunc func)
    {
        m_new_instance_func = func;
    }

    // Update all callees after all components added
    void update_callees();

public:
    // Get callee by name
    // Update *pp_string to shared string if found
    bool get_callee_by_name(String **pp_name, CalleeInfo *ptr_info)
    {
        if (!Program::convert_to_shared(pp_name))
            // This string is not in pool, not such callee
            return false;
        return m_callees.try_get(*pp_name, ptr_info);
    }

    // Get component by component no
    Program *get_component(ComponentNo component_no)
    {
        return m_components[component_no].program;
    }

    // Get component offset by component no
    ComponentOffset get_component_offset(ComponentNo component_no)
    {
        return m_components[component_no].offset;
    }

    // Get function by function no
    Function *get_function(FunctionNo function_no)
    {
        return m_functions[function_no];
    }

    // Get mapped component no
    ComponentNo get_mapped_component_no(ComponentNo this_component_no, ComponentNo call_component_no)
    {
        auto map_offset = m_components_map_offset[this_component_no];
        auto mapped_component_no = m_components_no_map[map_offset + call_component_no];
        return mapped_component_no;
    }

    // Get object's size
    size_t get_object_size()
    {
        return m_object_size;
    }

    // Create a new instance
    Object *new_instance(Domain *domain);

    // Invoke routine
    Value invoke(Thread *thread, ObjectId oid, const Value& function_name, Value *args, ArgNo n);

private:
    // All constant strings of programs
    static StringPool *m_string_pool;

    // Program name -> program map
    typedef simple::hash_map<String *, Program *> ProgramNameMap;
    static ProgramNameMap *m_all_programs;
    static struct std_critical_section *m_program_cs;

private:
    Attrib m_attrib;
    String *m_name;
    size_t m_object_size;

    // New() instance function
    NewInstanceFunc m_new_instance_func;

    // All components of this program
    simple::unsafe_vector<ComponentInfo> m_components;

    // Remap all components when they do call_far
    typedef simple::unsafe_vector<MapOffset> ComponentsMapOffset;
    typedef simple::unsafe_vector<ComponentNo> ComponentsNoMap;
    ComponentsMapOffset m_components_map_offset;
    ComponentsNoMap m_components_no_map;

    // All functions defined this program (those defined in component not included) 
    simple::unsafe_vector<Function *> m_functions;

    // All members defined this program (those defined in component not included) 
    simple::unsafe_vector<Member *> m_members;

    // Callees map by function name
    simple::hash_map<String *, CalleeInfo> m_callees;
};

} // End of namespace: cmm
