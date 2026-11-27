#ifndef HDSA_ALLOCATOR_ARENA_H
#define HDSA_ALLOCATOR_ARENA_H

#include <stdio.h>

#include "common.h"
#include "allocator_interface.h"
#include "virtual_alloc.h"

/**
 * @brief An arena allocator, AKA linear allocator, is one of the the most
 * simple kinds of allocators and also the fastest one.
 *
 * Allocations are made contiguously while adding padding for alignment
 * concerns, by bumping up the offset that marks the end of the allocated space
 * (current_offset), hence why it's also called "bump allocator", "linear
 * allocator", or "arena allocator". Individual deallocations are a no-op
 * (they do nothing), but the allocator can be "reset" to make new allocations
 * happen from the beginning of the arena, or even to get rid of all the memory
 * the arena has at once.
 *
 * It has 3 modes that change the arena's behavior: External, Static and
 * Dynamic.
 *
 * 1) External is a typical arena with a fixed buffer that doesn't grow.
 *
 * 2) Static uses the internal Allocator Virtual to do a virtual memory
 * allocation and create a "Memory Block" with it. The maximum size this block
 * will have is called "reserved memory", and the current usable memory in the
 * block (that can grow as long as it doesn't go beyond reserved) is called
 * "committed memory". Allocations that need to go above the current committed
 * memory result in the Arena asking the OS for more committed memory, growing
 * the committed memory, no need to do a reallocation/resizing of the existing
 * data in the block. If an allocation that needs more committed memory goes
 * beyond the reserved memory an error is given back to the user.
 *
 * 3) Dynamic is like Static but can create new memory blocks in a linked-list
 * manner, so memory from different blocks isn't contiguous. Allocations are
 * always done in the last block in the list and thanks to "used_total" the
 * sum of all used memory on every block is saved.
 *
 * It's mostly used for very short-lived allocations and it cannot do
 * individual deallocations, only to "reset" the offset to go back to 0 so
 * all allocations are done from the beginning.
 *
 * "v_alloc" is the internal Allocator_Virtual that performs the virtual memory
 * allocations.
 *
 * "current_offset" is the last allocation offset of used memory, it's
 * increased according to the requested size and alignment of each new
 * allocation. In Dynamic mode it gets reset to the offset after the first
 * allocation when a new memory block is created.
 *
 * "previous_offset" is the offset after the previous allocation (after adding
 * padding for alignment concerns). It is also reset to the offset after the
 * first allocation when a new memory block is created in Dynamic mode. It's
 * also the offset where the last allocation starts.
 *
 * "allocation_amount" saves how many allocations the user has requested (plus
 * 1 in Static or Dynamic mode for the block's header), not the virtual
 * committed nor reserved memory.
 *
 * "used_total" saves the sum of all the used memory in every memory block, the
 * sum of every block's "used_block" to be more accurate, not of the committed
 * memory.
 *
 * "v_alloc.committed_total" is used as the v_alloc.buffer's (External mode) or
 * sum of all blocks' "length" or "capacity" (Static and Dynamic). Also, in
 * External mode "v_alloc.reserved_total" contains the same number.
 */
typedef struct hdsa_Allocator_Arena
{
    hdsa_Allocator_Virtual v_alloc;
    ptrdiff_t current_offset;
    ptrdiff_t previous_offset;
    ptrdiff_t allocation_amount;
    ptrdiff_t used_total;
} hdsa_Allocator_Arena;

/**
 * @brief It initializes the Allocator_Arena with an existing buffer of a
 * certain size.
 * @param[in] self Allocator_Arena pointer, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Arena. It can be null
 * but make sure to change it later on. If it is null or size is 0, all the
 * allocator's data is set to 0/null.
 * @param[in] size Size of the buffer in bytes, must be at least 0.
 */
void hdsa_allocator_arena_init_external(hdsa_Allocator_Arena* const self, void* buffer, ptrdiff_t size);

/**
 * @brief It initializes the arena with sizes for reserved and committed memory
 * for a first virtual memory allocation inside the function.
 *
 * The allocator will only hold one Memory Block at a time so to change the
 * reserved amount, the current block must be deallocated first and then to
 * perform a new allocation with the new reserved amount.
 * @param[in] self Allocator_Arena pointer, must not be null.
 * @param[in] reserved_size Maximum size of the block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_granularity_size.
 *
 * If it's 0 no allocation will be done.
 * @param[in] committed_size Size of the usable block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_page_size.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
bool hdsa_allocator_arena_init_static(hdsa_Allocator_Arena* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out);

/**
 * @brief It initializes the Allocator_Arena with sizes for reserved and
 * committed memory for a first virtual memory allocation inside the function.
 *
 * Unlike the Static mode, Dynamic can hold multiple blocks, each with their
 * own reserved and commited amounts, but allocations are only done in the last
 * block.
 * @param[in] self Allocator_Arena pointer, must not be null.
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
bool hdsa_allocator_arena_init_dynamic(hdsa_Allocator_Arena* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out);

/**
 * @param[in] self Allocator_Arena pointer, must not be null.
 * @returns Amount of bytes yet to be allocated in the buffer. For Static and
 * Dynamic mode it's the remaining space in the committed memory of the last
 * block. If the buffer/last_block is null 0 is returned.
 */
ptrdiff_t hdsa_allocator_arena_remaining_storage(const hdsa_Allocator_Arena* const self);

/**
 * @param[in] self Allocator_Arena pointer, must not be null.
 * @returns A pointer to the latest allocation, which can be null if
 * buffer/last_block is null.
 */
void* hdsa_allocator_arena_last_allocation(const hdsa_Allocator_Arena* const self);

/**
 * @brief It allocates a certain portion of memory according to a size provided
 * by the user. The buffer to back this memory depends on the mode "v_alloc" is
 * set to.
 * @param[in] self Allocator_Arena pointer, must not be null.
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
bool hdsa_allocator_arena_allocate(hdsa_Allocator_Arena* const self, ptrdiff_t size, ptrdiff_t alignment, void** const allocation_out, hdsa_Error* const error_out);

/**
 * @brief It does nothing. Arena allocators don't do individual deallocations,
 * they only deallocate the whole buffer at once.
 */
void hdsa_allocator_arena_deallocate(hdsa_Allocator_Arena* const self, void* ptr);

/**
 * @brief It deallocates all the memory blocks except the first one and sets
 * the offsets, v_alloc.last_block->used_block and used_total to the size of
 * hdsa_Memory_Block, so allocations are done from the beginning of the
 * remaining block. allocation_amount is set to 1 as it represents the block's
 * header allocation.
 *
 * If there are no memory blocks for Static and Dynamic modes, or if the
 * allocator is in External mode all data members are set to 0/null.
 * @param[in] self Allocator_Arena pointer, must not be null.
 */
void hdsa_allocator_arena_reset(hdsa_Allocator_Arena* const self);

/**
 * @brief It deallocates all the memory blocks and sets all the data in the
 * allocator and "v_alloc" to 0/null.
 *
 * For External mode the pointer to the buffer is set to null.
 * @param[in] self The Allocator_Arena pointer, must not be null.
 */
void hdsa_allocator_arena_give_back_memory(hdsa_Allocator_Arena* const self);

/**
 * @brief Usually it does a new allocation, copies the data from the existing
 * one and does a deallocation, but it's more complicated than that:
 *
 * 1) If "old_memory" is null or "old_size" is 0 the new allocation is done but
 * there's no data copying.
 *
 * 2) On the other hand, if it ends up being the latest allocation then only
 * resize the latest allocation, no data copying is done.
 * @param[in] self The Allocator_Arena pointer, must not be null.
 * @param[in] old_memory The existing allocation pointer. It can be null or
 * even not part of the same buffer. If it's null only the new allocation is
 * done.
 * @param[in] old_size Size of the existing allocation, must not be negative.
 * If both sizes are the same nothing is done.
 * @param[in] new_size New size for the allocation, must not be negative.
 * If it's 0 nothing is done and the pointer allocation_out is pointing to
 * is set to old_memory.
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
bool hdsa_allocator_arena_resize
(
    hdsa_Allocator_Arena* const self,
    void** const old_memory,
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
bool hdsa_allocator_arena_generic_function
(
    void* const allocator,
    hdsa_Allocator_Behavior alloc_func,
    void** old_allocation,
    ptrdiff_t old_size,
    ptrdiff_t new_size,
    ptrdiff_t new_alignment,
    void** const allocation_out,
    hdsa_Error* const error_out
);

/**
 * @brief It creates a new Allocator_Interface struct from an Allocator_Arena
 * pointer.
 * @param[in] self Pointer to the Allocator_Arena, must not be null.
 * @returns A copy to a new Allocator_Interface struct with self as the
 * "allocator", and hdsa_allocator_arena_interface_wrapper as the
 * "generic_alloc_function".
 */
hdsa_Allocator_Interface hdsa_allocator_arena_create_interface(const hdsa_Allocator_Arena* const self);

/**
 * @brief Think of this temporal arena as a checkpoint that saves the latest
 * Arena data (and the used_block from the latest block) so once the original
 * Allocator_Arena allocates once more and aims to deallocate, it can go back
 * to that previous allocation offset that Temp_Arena saved instead of having
 * to reset the whole buffer or doing nothing.
 */
typedef struct hdsa_Allocator_Temp_Arena
{
    hdsa_Allocator_Arena* arena;
    ptrdiff_t pre_offset;
    ptrdiff_t cur_offset;
    ptrdiff_t al_amount;
    ptrdiff_t u_total;
} hdsa_Allocator_Temp_Arena;

/**
 * @brief It connects the Allocator_Temp_Arena with the Allocator_Arena to be
 * used as a short lived last allocation that will be reverted later.
 * @param[in] self Temp_Arena pointer, must not be null.
 * @param[in] ar Arena_Alloc pointer, must not be null.
 */
void hdsa_allocator_temp_arena_init(hdsa_Allocator_Temp_Arena* const self, hdsa_Allocator_Arena* const ar);

/**
 * @brief Cut ties with the current Allocator_Arena manually, so it can be used
 * by another one before the Allocator_Temp_Arena goes out of scope.
 * @param[in] self Temp_Arena pointer, must not be null.
 */
void hdsa_allocator_temp_arena_end(hdsa_Allocator_Temp_Arena* const self);

#endif // HDSA_ALLOCATOR_ARENA_H
