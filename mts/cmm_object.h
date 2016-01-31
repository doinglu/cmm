// cmm_object.h
// Object definition

#pragma once

#include "std_port/std_port_type.h"
#include "std_port/std_port_spin_lock.h"
#include "std_template/simple_vector.h"
#include "std_template/simple_list.h"
#include "cmm.h"
#include "cmm_global_id.h"
#include "cmm_value.h"

namespace cmm
{

class AbstractComponent;
class Domain;
class Program;

class Object
{
friend Program;

public:
    struct Entry
    {
        ObjectId gid;       // OID
        Object *object;     // The object
        Program *program;   // Program of the object
        Domain *domain;     // Domain of the object
    };

private:
    enum { MAX_ID_PAGES = 8192 };

public:
    // Initialize/shutdown this module
    static bool init();
    static void shutdown();

public:
    Object() { }
    virtual ~Object();

public:
    // Allocate an oid for this object
    bool assign_oid();

    // Free this object's oid
    void free_oid();

public:
    // Get object by id
    static Object *get_object_by_id(ObjectId oid)
    {
        auto *entry = get_entry_by_id(oid);
        return entry ? entry->object : 0;
    }

    // Get object's program by id
    // ATTENTION: WHY NOT GET OBJECT THEN GET PROGRAM?
    // This function is lock-free. The object may be destructued after gotten.
    // So we get the program directly and would verify it after we locked.
    static Program *get_program_by_id(ObjectId oid)
    {
        auto *entry = get_entry_by_id(oid);
        return entry ? entry->program : 0;
    }

public:
    // Get node & lock it
    static Entry *get_entry_by_id(ObjectId oid)
    {
        return m_id_manager->get_entry_by_id(oid);
    }

public:
    // Return oid
    ObjectId get_oid()
    {
        return m_oid;
    }

public:
    // Return domain of the object
    Domain *get_domain()
    {
        return m_domain;
    }

    // Set domain
    void set_domain(Domain *domain);

public:
    // Return specified component in this object
    AbstractComponent *get_component(ComponentNo component_no);

    // Return program of this object
    Program *get_program()
    {
        return m_program;
    }

private:
                            // [virtual table]
    ObjectId m_oid;         // Object's ID
    Domain  *m_domain;      // Belong to
    Program *m_program;     // Program of this object
    Value    m_members[1];  // Members start here

private:
    typedef GlobalIdManager<Entry> ObjectIdManager;
    static ObjectIdManager *m_id_manager;
};

} // End of namespace: cmm
