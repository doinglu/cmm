// cmm_program.cpp

#include "std_port/std_port.h"
#include "cmm_domain.h"
#include "cmm_object.h"
#include "cmm_program.h"
#include "cmm_thread.h"
#include "cmm_value.h"

namespace cmm
{

Parameter::Parameter(Function *function, const Value& name_value)
{
}

Function::Function(Program *program, const Value& name_value)
{
    m_program = program;
    m_name = Program::find_or_add_string(name_value.get_string());
    m_min_arg_no = 0;
    m_max_arg_no = 0;
    m_attrib = (Attrib)0;
}

Parameter *Function::define_parameter(const Value& name_value)
{
    return 0;
}

Member::Member(Program *program, const Value& name_value)
{
    m_program = program;
    m_name = Program::find_or_add_string(name_value.get_string());
    m_type = (ValueType)0;
    m_offset = 0;
}

// StringImpl pool of all programs
StringPool *Program::m_string_pool = 0;

// Program name -> program map
Program::ProgramNameMap *Program::m_all_programs = 0;
struct std_critical_section *Program::m_program_cs = 0;

int Program::init()
{
    m_string_pool = XNEW(StringPool);
    m_all_programs = XNEW(ProgramNameMap);

    std_new_critical_section(&m_program_cs);
    return 0;
}

void Program::shutdown()
{
    // Clear all programs
    auto programs = m_all_programs->values();
    for (auto it = programs.begin(); it != programs.end(); ++it)
        XDELETE(*it);
    STD_ASSERT(("All programs should be freed", m_all_programs->size() == 0));
    XDELETE(m_all_programs);

    std_delete_critical_section(m_program_cs);

    // Clear all strings
    XDELETE(m_string_pool);
}

// Create a program
Program::Program(const Value& name_value)
{
    m_name = Program::find_or_add_string(name_value.get_string());
    STD_ASSERT(("Program has been defined.", !find_program_by_name(m_name)));

    std_enter_critical_section(m_program_cs);
    m_all_programs->put(m_name, this);
    std_leave_critical_section(m_program_cs);
}

// Destruct program (only being called when shuttint down)
Program::~Program()
{
    // Destruct all functions
    for (auto it = m_functions.begin(); it != m_functions.end(); ++it)
        XDELETE(*it);

    std_enter_critical_section(m_program_cs);
    m_all_programs->erase(m_name);
    std_leave_critical_section(m_program_cs);
}

// COnver
StringImpl *Program::convert_to_shared(StringImpl **pp_string)
{
    StringImpl *string = *pp_string;
    if (!(string->attrib & ReferenceImpl::SHARED))
    {
        // Not shared? Lookup in pool
        string = find_string(string);
        if (string)
            // Modify previous string
            *pp_string = string;
    }

    return string;
}

// Find or add a string into pool
StringImpl *Program::find_or_add_string(const StringImpl *string)
{
    // Not found, create new string
    return m_string_pool->find_or_insert(string);
}

// Find string in pool (return 0 if not found)
StringImpl *Program::find_string(const StringImpl *string)
{
    return m_string_pool->find(string);
}

// Find a program by name (shared string)
Program *Program::find_program_by_name(StringImpl *program_name)
{
    Program *program;

    if (!convert_to_shared(&program_name))
        // No such name, program not found
        return 0;

    if (!m_all_programs->try_get(program_name, &program))
        // Program not found
        return 0;

    return program;
}

// Update callees of all programs
void Program::update_all_callees()
{
    for (auto it = m_all_programs->begin(); it != m_all_programs->end(); ++it)
        it->second->update_callees();
}

// Create function in this program
Function *Program::define_function(const Value& name_value,
    Function::Entry func_entry,
    ArgNo min_arg_no, ArgNo max_arg_no,
    Function::Attrib attrib)
{
    auto *function = XNEW(Function, this, name_value);
    function->m_func_entry = func_entry;
    function->m_min_arg_no = min_arg_no;
    function->m_max_arg_no = max_arg_no;
    function->m_attrib = attrib;
    m_functions.push_back(function);
    return function;
}

// Create member in this program
Member *Program::define_member(const Value& name_value, ValueType type, MemberOffset offset)
{
    auto *member = XNEW(Member, this, name_value);
    member->m_type = type;
    member->m_offset = offset;
    m_members.push_back(member);
    return member;
}

// Add a function to callee map
// Public function -> public callee
// Private function of this component + public function -> self callee
void Program::add_callee(ComponentNo component_no, Function *function)
{
    CalleeInfo info;
    info.component_no = component_no;
    info.function = function;
    if (function->is_public())
    {
        // Put in public callees
        m_public_callees.put(function->m_name, info);

        // Put in private callees if not existed
        if (!m_self_callees.contains_key(function->m_name))
            m_self_callees.put(function->m_name, info);
    }

    if (component_no == 0 && function->is_private())
    {
        // Override previous entry if existed
        m_self_callees[function->m_name] = info;
    }
}

// Detail for components in a program
// For a class with components, it looks like the following code:
/*
class __clone_entity_ob : public Object
{
private:
    __clone_entity  entity;       // component 0
    __feature_name  name;         // component 1
    __feature_dbase dbase;        // component 2
    __extend_feature_name name2;  // component 3
}

# "..._impl" means this is an implmentation class.
# "public Object" means it was derived from Object (All implementation should be).
# The members are components in this class.
# The name suffix "2" means there is already a component with same name (pure file name).
# The next component with same name is "3", "4" and so on.

When init the program for this class:

static Object *__clone_entity_ob::new_instance()
{
    return new __clone_entity_ob();
}

static Program *__clone_entity_ob::create_program()
{
    Program *program = new Program("/object/entity");

    program->set_new_instance_func(&new_instance);

    program->add_component("/feature/name", offset(m_name));
    program->add_component("/feature/dbase", offset(m_dbase));
    program->add_component("/extend/feature/name", offset(m_name2));

    program->define_function(...);
    ...

    return program;
}

# new_instance() - to new this class.
# create_program() - to new & init the program for this class.

ABOUT add_component() & update_callees()

Let's see the class samples:

// Components refered by /clone/entity
// 1 - /feature/name
// 2 - /feature/dbase
// 3 - /extend/feature/name
class __clone_entity
{
public:
    // Routine 0 of this component
    Value print_hello(Value *__args, ArgNo __n)
    {
        ...
        printf("Hello! %s\n", call_far(1, 7).m_string->c_str()); // Component 1, routine 7 - /feature/name::get_name()
        ...
    }
}

// Components refered by /feature/dbase
// N/A
class __feature_dbase
{
private:
    Value m_dbase;

public:
    // Routine 0 of this component
    Value set(Value *__args, ArgNo __n)
    {
        ...
        m_dbase[__args[0]] = __args[1];
        ...
    }

    // Routine 1 of this component
    Value query(Value *__args, ArgNo __n)
    {
        ...
        return m_dbase[__args[0]];
    }
}

// Components refered by /feature/name
// 1 - /feature/dbase
class __feature_name
{
    // Skip 6 routines

    // Routine 6 of this component
    Value set_name(Value *__args, ArgNo __n)
    {
        ...
        return call_far(1, 0, Value("name"), __args[0]); // Component 1, routine 0 - /feature/dbase::set()
        ...
    }

    // Routine 7 of this component
    Value get_name(Value *__args, ArgNo __n)
    {
        ...
        return call_far(1, 1, Value("name")); // Component 1, routine 1 - /feature/dbase::query()
    }
}

// Components refered by /extend/feature/name
// 1 - /feature/dbase
class __extend_feature_name
{
    ...
}

After all programs were created. The moudle will to call the update_callees() of all programs
to create their remap component entries.

Since the component no is different for differnt component. For example: 

For /feature/name, the component "1" is /feature/dbase. When it try to call the /feature/dbase::set
it call component "1" & routine 0. BUT, when /feature/name is a component in /clone/entity, the
/feature/dbase is component "2". So, we need create multiply entries for different cases. It should
map component "1" for /feature/name in /clone/entity to "2".

For /feature/name
Remap component no for /feature/name (component 0) : [
    0 -> 0 (/feature/name, offset 0)
    1 -> 1 (/feature/dbase, offset XX1)
]
Remap component no for /feature/dbase (component 1) : [
    0 -> 1 (/feature/dbase, offset XX1)
]

For /feature/dbase:
Remap component no for /feature/dbase (component 0) : [
    0 -> 0 (/feature/dbase, offset 0)
]

For /clone/entity
Remap component no for /clone/entity (component 0) : [
    0 -> 0 (/clone/entity, offset 0)
    1 -> 1 (/feature/name, offset XX2)
    2 -> 2 (/feature/dbase, offset XX3)
    3 -> 3 (/extend/feature/name, offset XX4) 
]
Remap component no for /feature/name (component 1) : [  // <--- The above sample
    0 -> 1 (/feature/name, offset XX2)
    1 -> 2 (/feature/dbase, offset XX3)                 // <--- MapImpl "1" to "2"
]
Remap component no for /feature/dbase (component 2) : [
    0 -> 2 (/feature/dbase, offset XX3)
]
Remap component no for /extend/feature/name (component 3) : [
    0 -> 3 (/extend/feature/name, offset XX4)
]
We put all mapped no in COMPONENT NO MAP ARRAY, it looks like following for the sample
/clone/entity:
Index:   0  1  2  3  4  5  6  7
MapImpl to: [0, 1, 2, 3; 1, 2; 2; 3;]
The ; means another MAP OFFSET ARRAY for components in /clone/entity, the array is:
Index:   0  1  2  3
Offset: [0, 4, 6, 7]

When /feature/name::get_name() in /clone/entity is called. The "this component no" is "1"
which means /feature/name in /clone/entity. It will call another routine /feature/dbase::query()
which component is "1" - /feature/dbase in /feature/name.
So we lookup element "1" in OFFSET ARRAY and we get offset 4. Then we lookup element "1"
plus offset 4 (1+4=5) in COMPONENT NO MAP ARRAY, we get "2" at last.
Finally, we will entry component "2" (/feature/dbase in /clone/entity) to call the
correct routine.

ATTENTION:
THE COMPONENT NO MAP ARRAY [0..N] is alway for the components in itself so it should be exactly
[0, 1, 2...N]. We still put them in the vector for convenience. 
The caller should not use call_far() to invoke routine in "this class" but use call_near()
instead.
*/

// Add component in this program
void Program::add_component(const Value& program_name_value, ComponentOffset offset)
{
    // Lookup & add all new programs
    ComponentInfo component;
    component.program_name = Program::find_or_add_string(program_name_value.get_string());
    component.program = 0; // Will be updated later in update_callees()
    component.offset = offset;
    m_components.push_back(component);
}

// Update all callees after all components added
void Program::update_callees()
{
    simple::hash_map<StringImpl *, ComponentNo> component_no_map;

    // Update "program" in Component info & build map: program->offset
    // Also calculate size of ComponentsNoMap
    size_t components_no_map_size = 0;
    for (auto it = m_components.begin(); it != m_components.end(); ++it)
    {
        auto *program = Program::find_program_by_name(it->program_name);
        STD_ASSERT(("Program is not found.", program));
        it->program = program;
        component_no_map.put(it->program_name, (ComponentNo)it.get_index());

        // Count size of components
        components_no_map_size += program->m_components.size();
    }

    // Build components no map
    m_components_map_offset.reserve(m_components.size());
    m_components_no_map.reserve(components_no_map_size);
    for (auto it = m_components.begin(); it != m_components.end(); ++it)
    {
        auto *program = Program::find_program_by_name(it->program_name);
        STD_ASSERT(("Program is not found.", program));

        // Start map this component
        m_components_map_offset.push_back((MapOffset)m_components_no_map.size());

        // Create ComponentsMap for this component
        auto end = program->m_components.end();
        for (auto entry = program->m_components.begin(); entry != end; ++entry)
        {
            auto component_no = component_no_map[entry->program_name];
            m_components_no_map.push_back(component_no);
        }
    }
    STD_ASSERT(m_components_no_map.size() == components_no_map_size);

    // After build, the m_components_no_map should be:
    // [
    //     no.0, no.1, no.2 ...     // For components in this program itself
    //     no.a, no.b ...           // For components in component 1 of this program
    //     no.x, no.y ...           // For components in component 2 of this program
    // ]

    // Lookup all functions & add to callees map
    for (auto it = m_components.begin(); it != m_components.end(); ++it)
    {
        auto *program = Program::find_program_by_name(it->program_name);
        ComponentNo component_no = (ComponentNo)it.get_index();

        auto end = program->m_functions.end();
        for (auto fun_it = program->m_functions.begin(); fun_it != end; ++fun_it)
        {
            auto *function = *fun_it;
            add_callee(component_no, function);
        }
    }
}

// Create a new instance
Object *Program::new_instance(Domain *domain)
{
    Thread *thread = Thread::get_current_thread();
    Domain *prev_domain = thread->get_current_domain();

    thread->switch_domain(domain);

    auto *ob = m_new_instance_func();
    if (ob)
    {
        ob->m_program = this;
        ob->assign_oid();
        ob->set_domain(domain);
    }

    thread->switch_domain(prev_domain);

    return ob;
}

// Invoke a function in program
Value Program::invoke(Thread *thread, ObjectId oid, const Value& function_name_value, Value *args, ArgNo n)
{
    CalleeInfo callee;

    if (function_name_value.m_type != ValueType::STRING)
        // Bad type of function name
        return Value();

    if (!get_public_callee_by_name((StringImpl **)&function_name_value.m_string, &callee))
        // No such function
        return Value();

    if (!thread->try_switch_object_by_id(thread, oid, args, n))
        // The object is not existed or just destructed
        return Value();

    // Call
    auto *object = Object::get_object_by_id(oid);
    auto component_no = callee.component_no;
    ComponentOffset offset = m_components[component_no].offset;
    auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
    Function::Entry func = callee.function->m_func_entry;

    CallContextNode __context(thread, object, component_no, args, n, (Value*)0, 0);
    thread->enter_function_call(&__context);
    auto ret = (component_impl->*func)(thread, args, n);
    return thread->leave_function_call(&__context, ret);
}

// Invoke self function
// Call public function in this program OR private function of this component
Value Program::invoke_self(Thread *thread, const Value& function_name_value, Value *args, ArgNo n)
{
    CalleeInfo callee;

    if (function_name_value.m_type != ValueType::STRING)
        // Bad type of function name
        return Value();

    // Get program of the current module
    auto *object = thread->get_this_object();
    auto component_no = thread->get_this_component_no();
    auto *to_program = m_components[component_no].program;

    if (!to_program->get_self_callee_by_name((StringImpl **)&function_name_value.m_string, &callee))
        // No such function
        return Value();

    // Call
    ComponentOffset offset = m_components[component_no].offset;
    auto *component_impl = (AbstractComponent *)(((Uint8 *)object) + offset);
    Function::Entry func = callee.function->m_func_entry;
    auto *context = &thread->get_this_context()->value;
    auto prev_component_no = context->m_this_component;
    context->m_this_component = component_no;
    Value ret = (component_impl->*func)(thread, args, n);
    context->m_this_component = prev_component_no;
    return ret;
}

} // End of namespace: cmm
