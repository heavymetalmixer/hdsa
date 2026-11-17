#ifndef HDSA_ALLOCATOR_STACK_H
#define HDSA_ALLOCATOR_STACK_H

#include <stdio.h>

#include "common.h"
#include "allocator_interface.h"
#include "virtual_alloc.h"

/**
 * @brief A stack allocator is rather simple and fast, just in second place
 * after arena allocators in those aspects.
 *
 * Allocations are made kinda contiguously while adding padding for alignment
 * concerns and a header inside that padding, making the padding mandatory and
 * with with a minimal size (the size of the header), while using 2 offsets,
 * one that marks the beggining of the usable memory by the user (right after)
 * the header and another at the end of the usable memory.
 *
 * One big difference with arena allocators, is that stack allocators work in
 * a "LIFO" way (last in, first out), so individual deallocations can be done
 * starting by the latest allocation, and the following ones in the opposite
 * order of how they were allocated.
 *
 * Stack allocators are said to be "the evolution of arena allocators" because
 * of the same "linear behavior", but thanks to the header this allocator
 * always uses more memory and memory isn't as contiguous, making it's
 * performance not as good in all situations and its usages not as many. It
 * tends to be used when individual allocations are necessary, when the LIFO
 * behavior is required (like how objects in the Call Stack are destroyed) and
 * the datum inside the allocator have long lifetimes (mostly the first
 * allocations).
 *
 * It has 3 modes that change the arena's behavior: External, Static and
 * Dynamic.
 *
 * 1) External sets a user-provided buffer that doesn't grow.
 *
 * 2) Static uses the internal Allocator Virtual to do a virtual memory
 * allocation and create a "Memory Block" with it. The maximum size this block
 * will have is called "reserved memory", and the current usable memory in the
 * block (that can grow as long as it doesn't go beyond reserved) is called
 * "committed memory". Allocations that need to go above the current committed
 * memory result in the Stack asking the OS for more committed memory, growing
 * the committed memory, no need to do a reallocation/resizing of the existing
 * data in the block. If an allocation that needs more committed memory goes
 * beyond the reserved memory an error is given back to the user.
 *
 * 3) Dynamic is like Static but can create new memory blocks in a linked-list
 * manner, so memory from different blocks isn't contiguous. Allocations are
 * always done in the last block in the list and thanks to "used_total" the
 * sum of all used memory on every block is saved. Also, when doing
 * deallocations, if one deallocation leaves the block "empty" (besides
 * certain info the allocator saves at the beginning of the block), the next
 * deallocation wil give back that block to the OS and the latest allocation in
 * the previous block is deallocated (as if both allocations were part of the
 * same block).
 *
 * Just like Allocator_Arena, Allocator_Stack can also reset to allocate from
 * the beginning without getting rid of all the available memory.
 *
 * "v_alloc" is the internal Allocator_Virtual that performs the virtual memory
 * allocations.
 *
 * "start_offset" is the offset that marks the beginning of the usable memory
 * for the user in the latest allocation and its saved right after the
 * Stack_Header. In Dynamic mode it's set after the first 2 allocations for the
 * new Memory Block (the block's header and the Stack_Last header).
 *
 * "current_offset" marks the end of the usable memory for the latest
 * allocation and it's always above start_offset (except when there's no
 * allocations in the current block, in which case both share the same value).
 *
 * "allocation_amount" saves how many allocations the user has requested (plus
 * 2 in Static or Dynamic mode for the block's header and the Stack_Last
 * header), not the virtual committed nor reserved memory.
 *
 * "used_total" saves the sum of all the used memory in every memory block, the
 * sum of every block's "used_block" to be more accurate, not of the committed
 * memory.
 *
 * "v_alloc.committed_total" is used as the v_alloc.buffer's (External mode) or
 * sum of all blocks' "length" or "capacity" (Static and Dynamic). Also, in
 * External mode "v_alloc.reserved_total" contains the same number.
 */
typedef struct hdsa_Allocator_Stack
{
    hdsa_Allocator_Virtual v_alloc;
    ptrdiff_t start_offset;
    ptrdiff_t current_offset;
    ptrdiff_t allocation_amount;
    ptrdiff_t used_total;
} hdsa_Allocator_Stack;

/**
 * @brief It holds the start_offset and padding of the previous allocation.
 * Necessary to be able to deallocate in a LIFO way.
 */
typedef struct hdsa_Allocator_Stack_Header
{
    ptrdiff_t start_offset;
    ptrdiff_t padding;
} hdsa_Allocator_Stack_Header;

/**
 * @brief It holds info about the latest allocation made by Allocator_Stack
 * inside a Memory Block for Static and Dynamic modes.
 *
 * It's stored right after the Memory Block's header and counts as the block's
 * second allocation, the user-made allocations start from the third one.
 */
typedef struct hdsa_Allocator_Stack_Last
{
    ptrdiff_t offset;
    ptrdiff_t size;
} hdsa_Allocator_Stack_Last;

/**
 * @brief It initializes the Allocator_Stack with an existing buffer of a
 * certain size. It doesn't use the Memory Block's header nor the
 * Allocator_Stack_Last's header.
 * @param[in] self Allocator_Stack pointer, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Stack. It can be null
 * but make sure to change it later on. If it is null or size is 0, all the
 * allocator's data is set to 0/null.
 * @param[in] size Size of the buffer in bytes, must be at least 0.
 */
void hdsa_allocator_stack_init_external(hdsa_Allocator_Stack* const self, void* buffer, ptrdiff_t size);

/**
 * @brief It initializes the Allocator_Stack with sizes for reserved and
 * committed memory for a first virtual memory allocation inside the function.
 *
 * The allocator will only hold one Memory Block at a time so to change the
 * reserved amount, the current block must be deallocated first and then to
 * perform a new allocation with the new reserved amount.
 *
 * The Memory Block will store it's own header and the Allocator_Stack_Last's
 * header as the first 2 allocations.
 * @param[in] self Allocator_Stack pointer, must not be null.
 * @param[in] reserved_size Maximum size of the block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_granularity_size.
 *
 * If it's 0 no allocation will be done.
 * @param[in] committed_size Size of the usable block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_page_size.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
bool hdsa_allocator_stack_init_static(hdsa_Allocator_Stack* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out);

/**
 * @brief It initializes the Allocator_Stack with sizes for reserved and
 * committed memory for a first virtual memory allocation inside the function.
 *
 * Unlike the Static mode, Dynamic can hold multiple blocks, each with their
 * own reserved and commited amounts, but allocations are only done in the last
 * block.
 *
 * Every Memory Block will store it's own header and the Allocator_Stack_Last's
 * header as the first 2 allocations.
 * @param[in] self Allocator_Stack pointer, must not be null.
 * @param[in] reserved_size Maximum size of the first memory block in bytes,
 * must be at least 0. It will be rounded to a multiple of
 * hdsa_virtual_granularity_size (64 KB).
 *
 * If it's 0 no allocation will be done.
 * @param[in] committed_size Size of the usable block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_page_size.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
bool hdsa_allocator_stack_init_dynamic(hdsa_Allocator_Stack* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out);

/**
 * @param[in] self Allocator_Stack pointer, must not be null.
 * @return Amount of bytes yet to be allocated in the buffer. For Static and
 * Dynamic mode it's the remaining space in the committed memory of the last
 * block. If the buffer/last_block is null 0 is returned.
 */
ptrdiff_t hdsa_allocator_stack_remaining_storage(const hdsa_Allocator_Stack* const self);

/**
 * @brief It allocates a certain portion of memory according to a size provided
 * by the user. The buffer to back this memory depends on the mode "v_alloc" is
 * set to.
 *
 * Every Memory Block will store it's own header and the Allocator_Stack_Last's
 * header as the first 2 allocations.
 * @param[in] self Allocator_Stack pointer, must not be null.
 * @param[in] size Amount of bytes to allocate, must be at least 1. If the
 * mode is Dynamic or Static and there's no memory block allocated,
 * (size + alignment - 1) * 4 will be used for the "reserved_size" of a virtual
 * allocation and (size + alignment - 1) for the "committed_size".
 * @param[in] alignment Alignment for the allocation. Must be at least 1.
 * If the value is bigger than 1 it will be set to the closest power of 2, if
 * it's not a power of 2 already.
 * @param[out] allocation_out Output parameter for the allocation result. If
 * the allocation fails at any point of the function the pointer allocation_out
 * is pointing to will be set to null.
 * @param[out] error_out Output parameter for error reporting, must not be
 * null.
 * @returns True if resizing succeeds in any of its variants, false otherwise.
 */
bool hdsa_allocator_stack_allocate(hdsa_Allocator_Stack* const self, ptrdiff_t size, ptrdiff_t alignment, void** const allocation_out, hdsa_Error* const error_out);

/**
 * @brief As this allocator works in a LIFO way, this function deallocates
 * only the latest allocation without any parameters besides the allocator
 * itself.
 *
 * If the Allocator_Stack is in Static or Dynamic mode and there's no
 * allocations inside the "last_block" besides the block's header and the
 * Allocator_Stack_Last header (2 allocations in that block, which is the same
 * as no user-made allocations), this allocator will deallocate the
 * last_block and the latest allocation of the previous block as well.
 *
 * If the buffer in External mode or last_block in Static o Dynamic mode is
 * null this function does nothing.
 * @param[in] self Allocator_Stack pointer, must not be null.
 */
void hdsa_allocator_stack_deallocate(hdsa_Allocator_Stack* const self);

/**
 * @brief It deallocates all the memory blocks except the first one and sets
 * the offsets, v_alloc.last_block->used_block and used_total to the size of
 * hdsa_Memory_Block + hdsa_Allocator_Stack_Last, so allocations are done from
 * the beginning of the remaining block. allocation_amount is set to 1 as it
 * represents the block's header allocation.
 *
 * If there are no memory blocks for Static and Dynamic modes, or if the
 * allocator is in External mode all data members are set to 0/null.
 * @param[in] self Allocator_Arena pointer, must not be null.
 */
void hdsa_allocator_stack_reset(hdsa_Allocator_Stack* const self);

/**
 * @brief It deallocates all the memory blocks and sets all the data in the
 * allocator and "v_alloc" to 0/null.
 *
 * For External mode the pointer to the buffer is set to null.
 * @param[in] self The Allocator_Stack pointer, must not be null.
 */
void hdsa_allocator_stack_give_back_memory(hdsa_Allocator_Stack* const self);

/**
 * @brief Usually it does a new allocation, copies the data from the existing
 * one and does a deallocation, but it's more complicated than that:
 *
 * 1) If "old_memory" is null or "old_size" is 0 the new allocation is done but
 * there's no data copying.
 *
 * 2) On the other hand, if it ends up being the latest allocation then only
 * resize the latest allocation, no data copying is done.
 * @param[in] self The Allocator_Stack pointer, must not be null.
 * @param[in] old_memory The existing allocation pointer. It can be null or
 * even not part of the same buffer. If it's null only the new allocation is
 * done.
 * @param[in] old_size Size of the existing allocation, must not be negative.
 * If both sizes are the same nothing is done.
 * @param[in] new_size New size for the allocation, must not be negative.
 * If it's 0 the latest allocation is deallocated and the pointer
 * allocation_out is pointing to is set to old_memory.
 * @param[in] new_alignment Alignment for the new allocation or resizing, must
 * be at least 1, and it will be rounded to a power of 2 (1 is one of those).
 * @param[out] allocation_out Output pointer to pointer for the allocation,
 * must not be null.
 *
 * If the allocation fails the pointer allocation_out is pointing to is set to
 * null. If both sizes and aligments are the same (old_memory has its own
 * alignment), or new_size is 0 then allocation_out will point to old_memory.
 *
 * If old_memory is the latest allocation and both alignments are the same, but
 * both sizes are different no data copy is done, the allocation is changed in
 * size to new_size and the pointer allocation_out is poiting to will point to
 * old_memory.
 * @param[out] error_out Pointer to the error struct, must not be null;
 * @returns True if resizing succeeds in any of its variants, false otherwise.
 */
bool hdsa_allocator_stack_resize
(
    hdsa_Allocator_Stack* const self,
    void* const old_memory,
    ptrdiff_t old_size,
    ptrdiff_t new_size,
    ptrdiff_t new_alignment,
    void** const allocation_out,
    hdsa_Error* const error_out
);

/**
 * @brief Generic wrapper function for the Allocator_Interface. It has all the
 * parameters any allocation function could need, but depending on the
 * Allocation_Type value some of the parameters will be ignored.
 */
bool hdsa_allocator_stack_generic_function
(
    void* const allocator,
    hdsa_Allocator_Behavior alloc_func,
    void* old_allocation,
    ptrdiff_t old_size,
    ptrdiff_t new_size,
    ptrdiff_t new_alignment,
    void** const allocation_out,
    hdsa_Error* const error_out
);

/**
 * @brief It creates a new Allocator_Interface struct from an Allocator_Stack
 * pointer.
 * @param[in] self Pointer to the Allocator_Stack, must not be null.
 * @returns A copy to a new Allocator_Interface struct with self as the
 * "allocator", and hdsa_allocator_stack_interface_wrapper as the
 * "generic_alloc_function".
 */
hdsa_Allocator_Interface hdsa_allocator_stack_create_interface(const hdsa_Allocator_Stack* const self);

#endif // HDSA_ALLOCATOR_STACK_H
