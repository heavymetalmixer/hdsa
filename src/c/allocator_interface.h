#ifndef HDSA_ALLOCATOR_INTERFACE_H
#define HDSA_ALLOCATOR_INTERFACE_H

#include <stddef.h>
#include <stdbool.h>

#include "errors.h"

/**
 * @brief Enum to select which allocation function to use inside wrapper functions
 * for each allocator. This reduces the amount of possible mistakes the user can
 * make when initializing the Allocator_Interface(1 function pointer instead of
 * multiple).
 */
typedef enum hdsa_Allocator_Behavior
{
    HDSA_ALLOCATION_TYPE_ALLOCATION = 0,
    HDSA_ALLOCATION_TYPE_DEALLOCATION,
    HDSA_ALLOCATION_TYPE_RESET,
    HDSA_ALLOCATION_TYPE_GIVE_BACK_MEMORY,
    HDSA_ALLOCATION_TYPE_RESIZE
} hdsa_Allocator_Behavior;

/**
 * @brief Runtime polymorphic type that can be used with any allocator,
 * that creates wrapper functions with these function signatures.
 *
 * It's basically a vtable but without the OOP focus so there's no
 * "base" class nor "derived" ones. It's a little more complicated to
 * used than virtual inheritance but your allocator's data memebers
 * don't need their data to be modified, you only add the wrapper function
 * and you're good to go.
 *
 * Steps to use it:
 *
 * 1) Instantiate this struct on the stack or the heap.
 *
 * 2) Make sure your allocator has the generic wrapper function. This
 * wraper function has all the parameters any kind of allocation function
 * would need, plus one parameter to chose which kind of function will
 * be used (an enum value). The implementation will use a switch or a
 * series of if statements to choose the correct allocation function to
 * call.
 *
 * 3) Initialize the "allocator interface" object with a pointer to your
 * allocator, and the function pointer with the addresses of the wrapper
 * function of that allocator.
 *
 * Now you can pass this interface type by pointer as a parameter to
 * functions your write, and deference its members to call the functions
 * of whatever allocator you initialized the interface with.
 */
typedef struct hdsa_Allocator_Interface
{
    void* allocator;
    bool (*generic_alloc_function)
    (
        void* const allocator,
        hdsa_Allocator_Behavior alloc_type,
        void* old_allocation,
        ptrdiff_t old_size,
        ptrdiff_t new_size,
        ptrdiff_t new_alignment,
        void** const allocation_out,
        hdsa_Error* const error_out
    );
} hdsa_Allocator_Interface;

#endif // HDSA_ALLOCATOR_INTERFACE_H
