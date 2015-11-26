// cmm_function.h
// Component & function relatives

#pragma once

#include "std_template/simple_hash_map.h"
#include "cmm.h"
#include "cmm_object.h"
#include "cmm_string_pool.h"
#include "cmm_typedef.h"
#include "cmm_value.h"
#include "cmm_value_list.h"

namespace cmm
{

class Domain;
class Efun;
class Function;
class Object;
class Program;
class Thread;
struct Instruction;

// Abstract class, all program class are derived from this class
// All classes derived from this class MUST NOT have virtual functions,
// constructor, destructor.
// The derived classes MUST BE initialized, destoried by Program 
class AbstractComponent
{
public:
    Program *m_program; // The relative program
    Value m_members[1]; // Members start from here
};

// SyntaxVariable of function
class SyntaxVariable
{
friend Function;

public:
	typedef enum
	{
        NULLABLE = 0x0001,
        DEFAULT = 0x0002
	} Attrib;

public:
    SyntaxVariable(Function *function, const String& name, ValueType type, Attrib attrib);

public:
    // Return the attribute of parameter
    Attrib get_attrib()
    {
        return m_attrib;
    }

    // Return the default value
    const Value& get_default()
    {
        return m_default;
    }

    // Return the name of parameter
    const StringImpl *get_name()
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
    Function   *m_function; // Owner function
	ValueType   m_type;     // Type of this argument
	Attrib      m_attrib;   // Attrib of this parmeters
    Value       m_default;  // Default value (valid only for has_default())
};

class Variables : public simple::vector<SyntaxVariable *> { };
typedef SyntaxVariable Parameter;
typedef SyntaxVariable LocalVariable;
typedef Variables Parameters;
typedef Variables LocalVariables;

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
        INTERPRETED = 0x4000,       // Being interpreted by VM
        PRIVATE = 0x8000,           // A private function
	} Attrib;

    // Types of entry routine
    // For script functions
    typedef Value (AbstractComponent::*ScriptEntry)(Thread *, Value *, ArgNo);
    // For external functions
    typedef Value (*EfunEntry)(Thread *, Value *, ArgNo);
    struct Entry
    {
        Entry(int zero = 0) { script_entry = 0; }
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
    // Create local variable definition in function
    LocalVariable *define_local_variable(const String& name, ValueType type, LocalVariable::Attrib attrib = (LocalVariable::Attrib)0);

    // Create parameter definition in function
    Parameter *define_parameter(const String& name, ValueType type, Parameter::Attrib attrib = (Parameter::Attrib)0);

    // Define the return type
    void define_ret_type(ValueType type, SyntaxVariable::Attrib attrib = (SyntaxVariable::Attrib)0)
    {
        m_ret_type = type;
        if (attrib & SyntaxVariable::Attrib::NULLABLE)
            m_attrib = (Function::Attrib)(m_attrib | RET_NULLABLE);
    }

    // Finish adding parameters
    bool finish_adding_parameters();

    // Get byte codes address
    const Instruction *get_byte_codes_addr() const;

    // Reserve local space
    void reserve_local(LocalNo count)
    {
        m_max_local_no = count;
    }

    // Set byte codes
    void set_byte_codes(Instruction *codes, size_t len);

public:
    // Get attribute
    Attrib get_attrib() const
    {
        return m_attrib;
    }

    // Get the entry pointer for efun
    EfunEntry get_efun_entry() const
    {
        return m_entry.efun_entry;
    }

    // Get local variables
    const LocalVariables& get_local_variables() const
    {
        return m_local_variables;
    }

    // Get max argument count
    LocalNo get_max_arg_no() const
    {
        return m_max_arg_no;
    }

    // Get max local count
    LocalNo get_max_local_no() const
    {
        // ATTENTION:
        // m_max_local_no != m_local_variables.size()
        return m_max_local_no;
    }

    // Get min argument count
    ArgNo get_min_arg_no() const
    {
        return m_min_arg_no;
    }

    // Get the function name
    StringImpl *get_name() const
    {
        return m_name;
    }

    // Get parameters
    const Parameters& get_parameters() const
    {
        return m_parameters;
    }

    // Get program
    const Program *get_program() const
    {
        return m_program;
    }

    // Get the entry pointer for script
    ScriptEntry get_script_entry() const
    {
        return m_entry.script_entry;
    }

    // Get return type
    ValueType get_ret_type() const
    {
        return m_ret_type;
    }

    // Is this function being interpreted by VM?
    bool is_being_interpreted() const
    {
        return (m_attrib & INTERPRETED) ? true : false;
    }

    // Is this function private?
    bool is_private() const
    {
        return (m_attrib & PRIVATE) ? true : false;
    }

    // Is this function public?
    bool is_public() const
    {
        return !is_private();
    }

private:
    StringImpl *m_name;
    Program    *m_program;
    Attrib      m_attrib;
    ArgNo       m_min_arg_no;
	ArgNo       m_max_arg_no;
    LocalNo     m_max_local_no;
    ValueType   m_ret_type;

	// Parameters & local variables
    Parameters m_parameters;
    LocalVariables m_local_variables;

    // Byte codes for interpreted function
    typedef simple::unsafe_vector<Instruction> ByteCodes;
    ByteCodes m_byte_codes;

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
    StringImpl  *m_name;
    Program     *m_program;
    ValueType    m_type;
    MemberIndex  m_index;
};

// Program of object
class Program
{
public:
    typedef enum
    {
        COMPILED_TO_NATIVE = 0x0001,    // This component was compiled to native class
        INTERPRETED = 0x8000,           // This is a interpreted program
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
    Program(const String& name, Attrib attrib = (Attrib)0);
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
    static void update_all_programs();

public:
    // Create an interpreter component
    static Object *new_interpreter_component();

public:
    // Is this program compiled to native class?
    bool is_compiled_to_native()
    {
        return m_attrib & COMPILED_TO_NATIVE ? true : false;
    }

    // Is this program being interpreted by the interpreter?
    bool is_using_interpreter()
    {
        return !is_compiled_to_native();
    }

public:
    // Create a constant & return the index in pool
    ConstantIndex define_constant(const Value& value);

    // Decribe the object properties
    void define_object(size_t object_size);

    // Create function definition in this program
    Function *define_function(const String& name,
                              Function::Entry entry,
                              ArgNo min_arg_no = 0, ArgNo max_arg_no = 0,
                              Function::Attrib attrib = (Function::Attrib)0);

    // Create member definition in this program
    Member *define_member(const String& name, ValueType type);

public:
    // Add a function to public callee map
    void add_callee(ComponentNo component_no, Function *function);

    // Add component in this program
    void add_component(const String& program_name);

    // Set function handler: new_instance
    void set_new_instance_func(NewInstanceFunc func)
    {
        m_new_instance_func = func;
    }

    // Update program after all programs are loaded
    void update_program();

private:
    // Mark a reference value & all values to CONSTANT in this container
    // (if value is array or mapping)
    void mark_constant(Value *value);
        
    // Update all callees during updating program
    void update_callees();

public:
    // Get component by component no
    Program *get_component(ComponentNo component_no) const
    {
        return m_components[component_no].program;
    }

    // Get component offset by component no
    ComponentOffset get_component_offset(ComponentNo component_no) const
    {
        return m_components[component_no].offset;
    }

    // Get constant by index
    Value *get_constant(ConstantIndex index) const
    {
        return m_constants.get_array_address(index);
    }

    // Get constant count
    ConstantIndex get_constants_count() const
    {
        return (ConstantIndex)m_constants.size();
    }

    // Get function by function no
    Function *get_function(FunctionNo function_no) const
    {
        return m_functions[function_no];
    }

    // Get mapped component no
    ComponentNo get_mapped_component_no(ComponentNo this_component_no, ComponentNo call_component_no) const
    {
        auto map_offset = m_components_map_offset[this_component_no];
        auto mapped_component_no = m_components_no_map[map_offset + call_component_no];
        return mapped_component_no;
    }

    // Get member by index
    const Member *get_member(MemberIndex index) const
    {
        return m_members[index];
    }

    // Get member count
    MemberIndex get_members_count() const
    {
        return (MemberIndex)m_members.size();
    }

    // Get name
    StringImpl *get_name() const
    {
        return m_name;
    }

    // Get this single component's size
    size_t get_this_component_size() const
    {
        return m_this_component_size;
    }

    // Get object's size
    size_t get_entire_object_size() const
    {
        return m_entire_object_size;
    }

    // Get callee by name (access by public)
    // Update name to shared string if found
    bool get_public_callee_by_name(const String *name, CalleeInfo *ptr_info) const
    {
        if (!Program::convert_to_shared(name))
            // This string is not in pool, not such callee
            return false;
        return m_public_callees.try_get(name->ptr(), ptr_info);
    }

    // Get callee by name (access by this component)
    // Update name to shared string if found
    bool get_self_callee_by_name(const String *name, CalleeInfo *ptr_info) const
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
    Value invoke(Thread *thread, ObjectId oid, const Value& function_name, Value *args, ArgNo n) const;

    // Invoke routine can be accessed by self component
    // See ATTENTION of invoke
    Value invoke_self(Thread *thread, const Value& function_name, Value *args, ArgNo n) const;

public:
    // Get function by entry
    static Function *get_function_by_entry(void *function_or_entry)
    {
        Function *function;
        if (!m_entry_functions->try_get(function_or_entry, &function))
            function = (Function *)function_or_entry;
        return function;
    }

    // Get program by name
    static Program *get_program_by_name(const Value& program_name);

private:
    // All constant strings of programs
    static StringPool *m_string_pool;

    // Function entry -> function map
    typedef simple::hash_map<void *, Function *> FunctionEntryMap;
    static FunctionEntryMap *m_entry_functions;

    // Program name -> program map
    typedef simple::hash_map<StringImpl *, Program *> ProgramNameMap;
    static ProgramNameMap *m_name_programs;

    // Obsoleted programs
    typedef simple::hash_set<Program *> ObsoletedProgramSet;
    static ObsoletedProgramSet *m_obsoleted_programs;

    // Critical Section for access
    static struct std_critical_section *m_program_cs;

private:
    Attrib m_attrib;
    StringImpl *m_name;
    size_t m_entire_object_size;
    size_t m_this_component_size;

    // New() instance function
    NewInstanceFunc m_new_instance_func;

    // All components of this program
    simple::unsafe_vector<ComponentInfo> m_components;

    // Remap all components when they do call_far
    typedef simple::unsafe_vector<MapOffset> ComponentsMapOffset;
    typedef simple::unsafe_vector<ComponentNo> ComponentsNoMap;
    ComponentsMapOffset m_components_map_offset;
    ComponentsNoMap m_components_no_map;

    // Rember all allocated constants
    ValueList m_list;

    // All constants
    simple::unsafe_vector<Value> m_constants;

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
