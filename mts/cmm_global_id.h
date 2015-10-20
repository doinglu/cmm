// cmm_global_id.h

#pragma once

#include "std_port/std_port.h"
#include "cmm.h"

// This file implements template for ID management.
// Global ID is a 64 bits, cross multi-process Id.
// It can be accessed concurrency.

namespace cmm
{

template <class Entry>
class GlobalIdManager
{
public:
    typedef simple::list_node<Entry> Node;
    typedef simple::manual_list<Entry> EntryList;

    // To lookup an item by GlobalId following these steps
    // 1. Get page by Id.index_page
    // 2. Get object by Id.index_offset in page
    // Why not use a simple array or vector?
    // Since we don't want to lock mutex when trying to get an object. So we use
    // two-level indexes to find the object by ID. 
    struct NodePage
    {
        Node nodes[GLOBAL_ID_PER_PAGE];
    };

public:
    GlobalIdManager(size_t max_pages)
    {
        m_max_pages = max_pages;

        // Init entries lock
        std_init_spin_lock(&m_entries_lock);

        // Init pages
        m_pages = XNEWN(NodePage *, m_max_pages);
        for (size_t i = 0; i < m_max_pages; i++)
            m_pages[i] = 0;
        m_page_count = 0;

        // Init the free entries list
        m_free_entries = XNEW(EntryList);
    }

    ~GlobalIdManager()
    {
        // Free the free entries list
        XDELETE(m_free_entries);

        // Free pages
        for (size_t i = 0; i < m_max_pages; i++)
            if (m_pages[i])
                XDELETE(m_pages[i]);
        XDELETEN(m_pages);

        // Free entries lock
        std_destroy_spin_lock(&m_entries_lock);
    }

    // Allocate a gid (create new page when necessary)
    Entry *allocate_id()
    {
        // Not free node? Create a new page
        std_get_spin_lock(&m_entries_lock);

        // Lookup a free node
        if (m_free_entries->size() == 0)
        {
            if (m_page_count >= m_max_pages)
            {
                STD_TRACE("Out of object id entries page, please adjust Object::MAX_PAGES.\n");
                return 0;
            }
            auto *m_page = XNEW(NodePage);
            if (!m_page)
            {
                STD_TRACE("Can not allocate new object id entries page.\n");
                return 0;
            }
            memset(m_page, 0, sizeof(NodePage));
            auto page_no = m_page_count++;
            m_pages[page_no] = m_page;

            // Put nodes of the new page to list
            for (size_t i = 0; i < GLOBAL_ID_PER_PAGE; i++)
            {
                auto *entry = &m_page->nodes[i].value;
                entry->gid.index_page = page_no;
                entry->gid.index_offset = i;
                m_free_entries->append_node(&m_page->nodes[i]);
            }
        }

        // Take off the first node
        auto it = m_free_entries->begin();
        auto *entry = &(*it);
        m_free_entries->remove_node(it);

        // Increase version of this node
        ++entry->gid.version;
        if (!entry->gid.version)
            entry->gid.version = 1;
        // New oid is generated

        // Done of creation
        std_release_spin_lock(&m_entries_lock);

        return entry;
    }

    // Return the gid to pool
    void free_id(GlobalId gid)
    {
        STD_ASSERT(("The id is not binded to anyone yet.", gid.i64));

        // Return the node to list
        std_get_spin_lock(&m_entries_lock);

        auto *node = get_node_by_id(gid);
        STD_ASSERT(("The entry is not binded to the id.", node));
        if (node->value.object)
        {
            // Clear other fields except gid
            auto gid = node->value.gid;
            memset(&node->value, 0, sizeof(node->value));
            node->value.gid = gid;
            m_free_entries->append_node(node);
        }

        std_release_spin_lock(&m_entries_lock);
    }

    // Get entry (unchecked, the gid must/or used to be valid)
    Entry *get_entry_by_id(GlobalId gid)
    {
        auto *node = get_node_by_id(gid);
        return node ? &node->value : 0;
    }

    // Get entry (check valid, any input is fine)
    Entry *get_entry_by_id_safe(GlobalId gid)
    {
        auto *node = get_node_by_id_safe(gid);
        return node ? &node->value : 0;
    }

private:
    // Return the node by id (unchecked, the gid must/or used to be valid)
    Node *get_node_by_id(GlobalId gid)
    {
        STD_ASSERT(("gid.index_page is over limit.", gid.index_page < m_max_pages));

        auto *page = m_pages[gid.index_page];
        STD_ASSERT(("Page[old.index_page] is not existed.", page));

        STD_ASSERT(gid.index_offset <= GLOBAL_ID_PER_PAGE);
        auto *node = &page->nodes[gid.index_offset];

        if (node->value.gid.i64 != gid.i64)
            // Not mached, return 0
            return 0;

        return node;
    }

    // Return the node by id (check valid, any input is fine)
    Node *get_node_by_id_safe(GlobalId gid)
    {
        NodePage *page;

        if (gid.index_page >= m_max_pages)
            // Bad index
            return 0;

        page = m_pages[gid.index_page];
        if (!page)
            // No such page
            return 0;

        STD_ASSERT(gid.index_offset <= GLOBAL_ID_PER_PAGE);
        auto *node = &page->nodes[gid.index_offset];

        if (node->value.gid.i64 != gid.i64)
            // Not mached, return 0
            return 0;

        return node;
    }

private:
    std_spin_lock_t m_entries_lock;
    size_t m_max_pages;
    size_t m_page_count;
    NodePage **m_pages;
    EntryList *m_free_entries;
};

}
