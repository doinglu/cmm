// simple_allocator.h
// Initial version by doing

#pragma once

#include "std_template/simple_list.h"

namespace cmm
{

class MemList
{
private:
    typedef enum : Uint16
    {
        // The block contains 1 or N class
        CONTAIN_1_CLASS = 0x0001,
        CONTAIN_N_CLASS = 0x0002,
        CONTAIN_CLASS = CONTAIN_1_CLASS | CONTAIN_N_CLASS,
    } Attrib;

    enum
    {
        CLASS_1_STAMP = 0x19770531,
        CLASS_N_STAMP = 0x19801126
    };

    // Destructor function entry
    typedef void(*DestructFunc)(void *ptr_class);

    struct RawBlock
    {
        // Get length of me
        size_t length() const { return n * size; }

        // Get raw data pointer
        Uint8 *data() const { return (Uint8*)(this + 1); }

        // Get single or class array pointer
        void *class_ptr(size_t index = 0) const
        {
            STD_ASSERT(("Buffer doesn't contain any class.", (attrib & CONTAIN_CLASS)));
            STD_ASSERT(("Index out of range of class_ptr.", index < n));
            return data() + index * size;
        }

        Attrib attrib;
        DestructFunc destructor;
        size_t len;
        Uint32 n;
        Uint32 size;
        Uint32 stamp;
    };

    typedef simple::list_node<RawBlock> BlockNode;

public:
    ~MemList()
    {
        while (m_list.size() > 0)
            destruct_and_free_node(__FILE__, __LINE__, m_list.begin().get_node());
    }

public:
    template <typename T, typename... Types>
    T* new1(const char *file, int line, Types&&... args)
    {
        // Allocate a size block & bind it
        auto* node = (BlockNode*)std_allocate_memory(sizeof(BlockNode) + sizeof(T), "mem_list", file, line);
        if (!node)
            return 0;
        m_list.append_node(node);

        // Build array block head
        auto* block = &node->value;
        block->n = 0;
        block->size = (Uint32)sizeof(T);
        block->stamp = CLASS_1_STAMP;

        // Save constructor/destructor
        void(*destructor)(T *) = [](T *me) { me->~T(); };
        block->destructor = (DestructFunc)destructor;

        // Construct all class
        Uint8 *ptr_class = block->data();
        new (ptr_class)T(simple::forward<Types>(args)...);
        block->n = 1;

        // Mark attribute
        block->attrib = Attrib::CONTAIN_1_CLASS;

        // Return the block
        return (T*)block->data();
    }

    template<typename T>
    void delete1(const char *file, int line, T *p)
    {
        auto* node = (BlockNode*)((Uint8*)p - sizeof(BlockNode));
        destruct_and_free_node(file, line, node);
    }

    template<typename T>
    T* newn(const char *file, int line, size_t n)
    {
        // Allocate a size  block & bind it
        auto* node = (BlockNode*)std_allocate_memory(sizeof(BlockNode) + sizeof(T) * n, "mem_list", file, line);
        if (!node)
            return 0;
        m_list.append_node(node);

        // Build array block head
        auto* block = &node->value;
        block->n = 0;
        block->size = (Uint32)sizeof(T);
        block->stamp = CLASS_N_STAMP;

        // Save constructor/destructor
        void(*destructor)(T *) = [](T *me) { me->~T(); };
        block->destructor = (DestructFunc)destructor;

        // Construct all class
        Uint8 *ptr_class = block->data();
        for (block->n = 0; block->n < n; block->n++, ptr_class += sizeof(T))
            new (ptr_class)T();

        // Mark attribute
        block->attrib = Attrib::CONTAIN_N_CLASS;

        // Return the block
        return (T*)block->data();
    }

    template<typename T>
    void deleten(const char *file, int line, T *p)
    {
        auto* node = (BlockNode*)((Uint8*)p - sizeof(BlockNode));
        destruct_and_free_node(file, line, node);
    }

private:
    // Destruct and free the node
    void destruct_and_free_node(const char* file, int line, BlockNode* node)
    {
        m_list.remove_node(node);
        auto* block = &node->value;
        STD_ASSERT(("Bad stamp of block class[].", block->stamp == CLASS_1_STAMP || block->stamp == CLASS_N_STAMP));
        Uint8 *ptr_class = block->data();
        for (size_t i = 0; i < block->n; i++, ptr_class += block->size)
            block->destructor(ptr_class);
        std_free_memory(node, "mem_list", file, line);
    }

private:
    simple::manual_list<RawBlock> m_list;
};

} // End of namespace: simple
