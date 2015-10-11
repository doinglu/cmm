// cmm_object.cpp

#include "std_port/std_port.h"
#include "std_port/std_port_os.h"
#include "std_port/std_port_spin_lock.h"
#include "cmm_domain.h"
#include "cmm_object.h"

namespace cmm
{

// ID->objects's entries
Object::ObjectIdManager *Object::m_id_manager = 0;
    
int Object::init()
{
    // Create the id manager
    m_id_manager = new Object::ObjectIdManager(MAX_ID_PAGES);
    return 0;
}

void Object::shutdown()
{
    // Destory the id manager
    delete Object::m_id_manager;
}

// Allocate an ID & assign to this object
bool Object::assign_oid()
{
    STD_ASSERT(("The object is already binded an id.", !m_oid.i64));
    STD_ASSERT(("The object shouldn't have been joined any domain.", !m_domain));

    auto *entry = Object::m_id_manager->allocate_id();

    // Assign oid to me
    m_oid = entry->gid;
    entry->object = this;
    entry->domain = 0;
    entry->program = this->m_program;
    return true;
}

// Free an ID
void Object::free_oid()
{
    Object::m_id_manager->free_id(m_oid);
}

// Let object join domain
void Object::set_domain(Domain *domain)
{
    if (m_domain)
        throw "Object was already in domain.";
    m_domain = domain;
    domain->join(this);

    // Update entry.domain
    auto *entry = Object::m_id_manager->get_entry_by_id(m_oid);
    entry->domain = m_domain;
}

} // End of namespace: cmm
