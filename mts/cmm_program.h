// cmm_function.h
// Component & function relatives

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm.h"
#include "cmm_object.h"
#include "cmm_string_pool.h"
#include "cmm_typedef.h"
#include "cmm_value.h"

namespace cmm
{

class Domain;
class Efun;
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
        NULLABLE = 0x0001,
        DEFAULT = 0x0002
	} Attrib;

public:
    Parameter(Function *function, const String& name);

public:
    // Return the attribute of parameter
    Attrib get_attrib()
    {
        return m_attrib;
    }

    // Return the name of parameter
    const String get_name()
    {
        return m_name;
    }

    // Return the type of parameter
    ValueType get_type()
    {
        return m_type;
    }

    // Is this parameter has default value?
    bool has_default()
    {
        return (m_attrib & DEFAULT) != 0;
    }

    // Is this parameter be nullable?
    bool is_nullable()
    {
        return (m_attrib & NULLABLE) != 0;
    }

private:
	StringImpl *m_name;     // Point to string pools in Program
	ValueType   m_type;     // Type of this argument
	Attrib      m_attrib;   // Attrib of this parmeters
};

// Function
class Function
{
friend Program;
friend Efun;

public:
	typedef enum
	{
		RANDOM_ARG = 0x0001,        // Accept random arguments
        RET_NULLABLE = 0x0002,      // Return nullable type
        EXTERNAL = 0x2000,          // An external function
        PRIVATE = 0x8000,           // A private function
	} Attrib;

    // Types of entry routine
    // For script functions
    typedef Value (AbstractComponent::*ScriptEntry)(Thread *, Value *, ArgNo);
    // For external functions
    typedef Value (*EfunEntry)(Thread *, Value *, ArgNo);
    struct Entry
    {
        Entry() { script_entry = 0; }
        Entry(ScriptEntry entry) { script_entry = entry; }
        Entry(EfunEntry entry) { efun_entry = entry; }

        union
        {
            ScriptEntry script_entry;
            EfunEntry efun_entry;
        };
    };

public:
    Function(Program *program, const String& name);
    ~Function();

public:
    // Create parameter definition in function
    Parameter *define_parameter(const String& name, Parameter::Attrib attrib);

    // Finish adding parameters
    bool finish_adding_parameters();

public:
    // Get the entry pointer for efun
    EfunEntry get_efun_entry()
    {
        return m_entry.efun_entry;
    }

    // Get the entry pointer for script
    ScriptEntry get_script_entry()
    {
        return m_entry.script_entry;
    }

    // Get the function name
    StringImpl *get_name()
    {
        return m_name;
    }
        
    // Is this function public?
    bool is_public()
    {
        return !is_private();
    }

    // Is this function private?
    bool is_private()
    {
        return (m_attrib & PRIVATE) ? true : false;
    }

private:
    Program    *m_program;
    StringImpl *m_name;
	ArgNo       m_min_arg_no;
	ArgNo       m_max_arg_no;
	Attrib      m_attrib;
    ValueType   m_ret_type;

	// Parameters
	simple::vector<Parameter *> m_parameters;

    // Entry
    Entry       m_entry;
};

// Member of program
class Member
{
friend Program;

public:
    Member(Program *program, const String& name);

private:
    Program     *m_program;
    StringImpl  *m_name;
    ValueType    m_type;
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
        StringImpl *program_name;
        Program *program;
        ComponentOffset offset;
    };

public:
    static int init();
    static void shutdown();

public:
    Program(const String& name);
    ~Program();

public:
    // Convert a normal StringImpl pointer to shared StringImpl pointer
    // The string.m_string would be updated if found in pool
    static bool convert_to_shared(const String* string);
        
    // Find or add a string into pool
    static StringImpl *find_or_add_string(const String& string);

    // Find string in pool (return 0 if not found)
    static StringImpl *find_string(const String& string);

    // Find a program by name (shared string)
    // The string.m_string would be updated if found in pool
    static Program *find_program_by_name(const String& program_name);

    // Update callees of all programs
    static void update_all_callees();

public:
    // Decribe the object properties
    void define_object(size_t object_size)
    {
        m_object_size = object_size;
    }

    // Create function definition in this program
    Function *define_function(const String& name,
                              Function::Entry entry,
                              ArgNo min_arg_no = 0, ArgNo max_arg_no = 0,
                              Function::Attrib attrib = (Function::Attrib)0);

    // Create member definition in this program
    Member *define_member(const String& name, ValueType type, MemberOffset offset);

public:
    // Add a function to public callee map
    void add_callee(ComponentNo component_no, Function *function);

    // Add component in this program
    void add_component(const String& program_name, ComponentOffset offset);

    // Set function handler: new_instance
    void set_new_instance_func(NewInstanceFunc func)
    {
        m_new_instance_func = func;
    }

    // Update all callees after all components added
    void update_callees();

public:
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

    // Get name
    StringImpl *get_name()
    {
        return m_name;
    }

    // Get object's size
    size_t get_object_size()
    {
        return m_object_size;
    }

    // Get callee by name (access by public)
    // Update name to shared string if found
    bool get_public_callee_by_name(const String *name, CalleeInfo *ptr_info)
    {
        if (!Program::convert_to_shared(name))
            // This string is not in pool, not such callee
            return false;
        return m_public_callees.try_get(name->ptr(), ptr_info);
    }

    // Get callee by name (access by this component)
    // Update name to shared string if found
    bool get_self_callee_by_name(const String *name, CalleeInfo *ptr_info)
    {
        if (!Program::convert_to_shared(name))
            // This string is not in pool, not such callee
            return false;
        return m_self_callees.try_get(name->ptr(), ptr_info);
    }

    // Create a new instance
    Object *new_instance(Domain *domain);

    // Invoke routine
    // function_name may be modified to shared string, IGNORE the const
    // ATTENTION: Must use Value for input parameter to avoid constructor String(),
    // or the modification of function_name (to lookup shared) would be useless.
    Value invoke(Thread *thread, ObjectId oid, const Value& function_name, Value *args, ArgNo n);

    // Invoke routine can be accessed by self component
    // See ATTENTION of invoke
    Value invoke_self(Thread *thread, const Value& function_name, Value *args, ArgNo n);

private:
    // All constant strings of programs
    static StringPool *m_string_pool;

    // Program name -> program map
    typedef simple::hash_map<StringImpl *, Program *> ProgramNameMap;
    static ProgramNameMap *m_all_programs;
    static struct std_critical_section *m_program_cs;

private:
    Attrib m_attrib;
    StringImpl *m_name;
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
    typedef simple::hash_map<StringImpl *, CalleeInfo> CalleInfoMap;
    CalleInfoMap m_public_callees;  // Can be accessed by public
    CalleInfoMap m_self_callees;    // Only can be accessed by self
};

} // End of namespace: cmm
