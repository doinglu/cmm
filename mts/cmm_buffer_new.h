// cmm_buffer_new.h

// ATTENTION: WHY BUFFER_NEW?
// There is GC working for memory management. But the GC only focus the
// ReferenceImpl types such as StringImpl, BufferImpl, FunctionPtrImp,
// ArrayImpl, MapImpl. If we need more C++ class types, we should put them
// into a BufferImpl by using BUFFER_NEW.
// For example:
// Class MyClass;
// MyClass *p = BUFFER_NEW(MyClass, Initializer...)
// OR:
// MyClass *p = BUFFER_NEWN(MyClass, size)
// We don't need do BUFFER_DELETE/BUFFER_DELETEN when the allocated class
// is freed. They will be collected when no Value refer to them.
// How ever, we still can using BUFFER_DELETE/BUFFER_DELETEN to destruct
// the class before
//
// TIPS:
// BUFFER_NEW return the pointer of BufferImpl instead of Class *. WHY?
// We stored the classes into BufferImpl, we also need to store the pointer
// of BufferImpl to prevent it being freed. The return type "BufferImpl"
// prompt us to store it. We need to pickup the class by call class_ptr()
// outselves.
//
// TIPS:
// the BUFFER_ALLOC() return an unbinded buffer. OR we free it manually,
// OR we bind it to current domain, OR it will be leak.

#pragma once

#include "std_template/simple.h"
#include "cmm.h"
#include "cmm_value.h"

namespace cmm
{

template <typename T>
void buffer_constructor(T *me, T *other)
{
    new (me) T(*other);
}

template <typename T>
void buffer_destructor(T *me)
{
    me->~T();
}

template <typename T, typename... Types>
BufferImpl *buffer_new(const char *file, int line, Types&&... args)
{
    // Allocate a size buffer & bind it
    auto *buffer = BUFFER_ALLOC(sizeof(T));
    buffer->bind_to_current_domain();

    // Construct the class T
    new (buffer->data())T(simple::forward<Types>(args)...);

    // Save constructor/destructor
    void (*constructor)(T *, const T *) = [](T *me, const T *other) { new (me) T(*other); };
    void (*destructor)(T *) = [](T *me) { me->~T(); };
    buffer->constructor = (BufferImpl::ConstructFunc)constructor;
    buffer->destructor = (BufferImpl::DestructFunc)destructor;

    // Mark attrib
    buffer->attrib = BufferImpl::CONTAIN_1_CLASS;

    // Return the buffer
    return buffer;
}

// Delete buffer contains 1 class
inline void buffer_delete(const char *file, int line, BufferImpl *buffer)
{
    // Verify attrib & free the buffer - destructor will be invoked in free()
    STD_ASSERT(("Bad buffer class pointer to delete.",
               buffer->attrib & BufferImpl::CONTAIN_1_CLASS));
    BufferImpl::free(file, line, buffer);
}

template <typename T>
BufferImpl *buffer_new_arr(const char *file, int line, size_t n)
{
    // Allocate a size buffer & bind it
    auto *buffer = BUFFER_ALLOC(BufferImpl::RESERVE_FOR_CLASS_ARR + sizeof(T) * n);
    buffer->bind_to_current_domain();

    // Build array info head
    auto *info = (BufferImpl::ArrInfo *)buffer->data();
    info->n = (Uint32)n;
    info->size = (Uint32)sizeof(T);
    info->stamp = BufferImpl::CLASS_ARR_STAMP;

    // Construct all class
    Uint8 *ptr_class = (Uint8 *) (info + 1);
    for (size_t i = 0; i < n; i++, ptr_class += sizeof(T))
        new (ptr_class)T();

    // Save constructor/destructor
    void(*constructor)(T *, const T *) = [](T *me, const T *other) { new (me) T(*other); };
    void(*destructor)(T *) = [](T *me) { me->~T(); };
    buffer->constructor = (BufferImpl::ConstructFunc)constructor;
    buffer->destructor = (BufferImpl::DestructFunc)destructor;

    // Mark attribute
    buffer->attrib = BufferImpl::CONTAIN_N_CLASS;

    // Return the buffer
    return buffer;
}

// Delete buffer contains N classes
void buffer_delete_arr(const char *file, int line, BufferImpl *buffer)
{
    // Verify attrib & free the buffer - destructor will be invoked in free()
    STD_ASSERT(("Bad buffer class array pointer to delete.",
               buffer->attrib & BufferImpl::CONTAIN_N_CLASS));
    BufferImpl::free(file, line, buffer);
}

// Macro as operators
#define BUFFER_NEW(T, ...)          buffer_new<T>(__FILE__, __LINE__, ##__VA_ARGS__)
#define BUFFER_DELETE(p)            buffer_delete(__FILE__, __LINE__, p)
#define BUFFER_NEWN(T, n)           buffer_new_arr<T>(__FILE__, __LINE__, n)
#define BUFFER_DELETEN(p)           buffer_delete_arr(__FILE__, __LINE__, p)

}