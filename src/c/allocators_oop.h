#ifndef ALLOCATORS_OOP_H
#define ALLOCATORS_OOP_H

#include <stddef.h>
#include <stdbool.h>

/**
 * Interface to implement for classes/structs that wanna use code with it.
 * Given it's a "pure abstract class" (no data besides the vtable) there are no
 * function implementations.
 * Add an instance of hdsa_AllocatorOOP to the struct that will use this
 * interface as the first data member, and initialize the vtable as a
 * static const hdsa_AllocatorVtable and assign its address to the
 * vptr.
 *
 * This code exists just as a reminder to myself as how more-or-less dynamic
 * polymorphism is to be done in C.
 */
typedef struct hdsa_Allocator_Vtable hdsa_Allocator_Vtable;

typedef struct hdsa_Allocator_OOP
{
    const hdsa_Allocator_Vtable* vptr;
} hdsa_Allocator_OOP;

typedef struct hdsa_Allocator_Vtable
{
    void* (*hdsa_allocator_allocate)(hdsa_Allocator_OOP* const self, ptrdiff_t allocation_size, ptrdiff_t allocation_alignment);
    void (*hdsa_allocator_deallocate)(hdsa_Allocator_OOP* const self, void* allocation_buffer, ptrdiff_t allocation_size, ptrdiff_t allocation_alignment);
    bool (*hdsa_allocator_is_equal)(const hdsa_Allocator_OOP* const self, const hdsa_Allocator_OOP* const al);
} hdsa_Allocator_Vtable;

#endif // ALLOCATORS_OOP_H
