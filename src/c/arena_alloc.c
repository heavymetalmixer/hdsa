#include "arena_alloc.h"

/**
 * @brief It initializes the Allocator Arena with an existing buffer of a
 * certain size.
 * @param[in] self Allocator_Arena pointer, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Arena. It can be null
 * but make sure to change it later on. If it is null or size is 0, all the
 * arena's data is set to 0/null.
 * @param[in] size Size of the buffer in bytes, must be at least 0.
 */
void hdsa_allocator_arena_init_external(hdsa_Allocator_Arena* const self, void* buff, ptrdiff_t size)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "The buffer size to initialize the Allocator_Arena is less than 1.\n");

    hdsa_allocator_virtual_init_external(&self->v_alloc, buff, size);
    self->previous_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;
}

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
bool hdsa_allocator_arena_init_static(hdsa_Allocator_Arena* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The reserved_size to initialize the Allocator_Arena is less than 1.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The buffer size to initialize the Allocator_Arena is less than 1.\n");

    self->previous_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;

    if (!hdsa_allocator_virtual_init_static(&self->v_alloc, reserved_size, committed_size, error_out))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        self->previous_offset = self->v_alloc.last_block->used_block;
        self->current_offset = self->v_alloc.last_block->used_block;
        self->used_total += self->v_alloc.last_block->used_block;
        self->allocation_amount += 1;
    }

    return true;
}

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
bool hdsa_allocator_arena_init_dynamic(hdsa_Allocator_Arena* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The reserved_size to initialize the Allocator_Arena is less than 1.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The buffer size to initialize the Allocator_Arena is less than 1.\n");

    self->previous_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;

    if (!hdsa_allocator_virtual_init_dynamic(&self->v_alloc, reserved_size, committed_size, error_out))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        self->previous_offset = self->v_alloc.last_block->used_block;
        self->current_offset = self->v_alloc.last_block->used_block;
        self->used_total += self->v_alloc.last_block->used_block;
        self->allocation_amount += 1;
    }

    return true;
}

/**
 * @param[in] self Allocator_Arena pointer, must not be null.
 * @return Amount of bytes yet to be allocated in the buffer. For Static and
 * Dynamic mode it's the remaining space in the committed memory of the last
 * block. If the buffer/last_block is null 0 is returned.
 */
ptrdiff_t hdsa_allocator_arena_remaining_storage(const hdsa_Allocator_Arena* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");

    if ((self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) || (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        return (self->v_alloc.last_block) ? (self->v_alloc.last_block->committed_block - self->current_offset) : 0;
    }

    return (self->v_alloc.buffer) ? self->v_alloc.committed_total - self->current_offset : 0;
}

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
bool hdsa_allocator_arena_allocate(hdsa_Allocator_Arena* const self, ptrdiff_t size, ptrdiff_t alignment, void** const allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((size > 0), "Allocation sizes of 0 or negative aren't allowed.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "Allocation alignments of 0 or negative aren't allowed.\n");

    puts("###############   ALLOCATION!   ################\n");

    ptrdiff_t buffer_address = 0, current_address = 0, aligned_address = 0, aligned_offset = 0, padding = 0;
    alignment = (hdsa_common_is_power_of_two(alignment)) ? alignment : hdsa_common_round_pow_two(alignment);

    switch (self->v_alloc.mode)
    {
        case HDSA_ALLOCATOR_VIRTUAL_EXTERNAL:
        {
            HDSA_BASIC_ASSERT((self->v_alloc.buffer), "The Allocator_Arena buffer null.\n");

            if (!self->v_alloc.buffer)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NULL_BUFFER;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->v_alloc.buffer, __LINE__ - 3, "The Allocator_Arena's buffer is null, the pointer inside allocation_out will be set to null.\n");
                *allocation_out = (void*)0;

                return false;
            }

            printf("allocation size: %lld\n", size);
            printf("allocation_amount before: %lld\n", self->allocation_amount);
            printf("remaining_space before: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("previous_offset before: %lld\n", self->previous_offset);
            printf("current_offset before: %lld\n", self->current_offset);

            buffer_address = (ptrdiff_t)self->v_alloc.buffer;
            current_address = buffer_address + self->current_offset;
            aligned_address = hdsa_common_align_forward(current_address, alignment);

            printf("current_address: %lld\n", current_address);
            printf("aligned_address: %lld\n", aligned_address);

            /**
             * Subtract away the buffer address number so aligned_offset becomes an actual
             * relative offset like previous_offset and current_offset, instead of the
             * huge address number aligned_address has.
             */
            aligned_offset = aligned_address - buffer_address;
            printf("aligned offset: %lld\n", aligned_offset);
            padding = aligned_offset -self->current_offset;

            if ((aligned_offset + size) > self->v_alloc.committed_total)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, (aligned_offset + size) > self->buffer_length, __LINE__ - 3, "The Allocator_Arena can't fit the new allocation in the buffer.");
                *allocation_out = (void*)0;

                return false;
            }

            self->previous_offset = aligned_offset;
            self->current_offset = aligned_offset + size;
            self->allocation_amount += 1;
            self->used_total += (padding + size);

            printf("previous_offset after: %lld\n", self->previous_offset);
            printf("current_offset after: %lld\n", self->current_offset);
            printf("remaining_space after: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("allocation_amount after: %lld\n\n\n", self->allocation_amount);

            *allocation_out = (void*)aligned_address;
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATOR_VIRTUAL_STATIC:
        case HDSA_ALLOCATOR_VIRTUAL_DYNAMIC:
        {
            /**
             * The maximum padding an address can get to be aligned is (alignment - 1)
             */
            ptrdiff_t maximum_size = (size + alignment - 1);

            if (!self->v_alloc.last_block)
            {
                if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
                {
                    if (!hdsa_allocator_arena_init_static(self, maximum_size * 4, maximum_size, error_out))
                    {
                        *allocation_out = (void*)0;
                        return false;
                    }
                }
                else
                {
                    if (!hdsa_allocator_arena_init_dynamic(self, maximum_size * 4, maximum_size, error_out))
                    {
                        *allocation_out = (void*)0;
                        return false;
                    }
                }
            }

            printf("allocation size: %lld\n", size);
            printf("allocation_amount before: %lld\n", self->allocation_amount);
            printf("remaining_space before: %lld\n", self->v_alloc.last_block->committed_block - self->current_offset);
            printf("previous_offset before: %lld\n", self->previous_offset);
            printf("current_offset before: %lld\n", self->current_offset);

            buffer_address = (ptrdiff_t)self->v_alloc.last_block;
            current_address = buffer_address + self->current_offset;
            aligned_address = hdsa_common_align_forward(current_address, alignment);

            printf("current_address: %lld\n", current_address);
            printf("aligned_address: %lld\n", aligned_address);

            /**
             * Subtract away the buffer address number so aligned_offset becomes an actual
             * relative offset like previous_offset and current_offset, instead of the
             * huge address number aligned_address has.
             */
            aligned_offset = aligned_address - buffer_address;
            printf("aligned offset: %lld\n", aligned_offset);

            padding = aligned_offset -self->current_offset;

            if ((aligned_offset + size) > self->v_alloc.last_block->committed_block)
            {
                if (!hdsa_allocator_virtual_commit(&self->v_alloc, maximum_size, allocation_out, error_out))
                {
                    ptrdiff_t new_reserved_size = ((maximum_size * 4) > self->v_alloc.last_block->reserved_block) ? (maximum_size * 4) : self->v_alloc.last_block->reserved_block;

                    if
                    (
                        (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) ||
                        !hdsa_allocator_virtual_allocate(&self->v_alloc, new_reserved_size, maximum_size, allocation_out, error_out)
                    )
                    {
                        return false;
                    }

                    puts("Allocating inside a new memory block.\n");

                    self->used_total += self->v_alloc.last_block->used_block;
                    self->previous_offset = self->v_alloc.last_block->used_block;
                    self->current_offset = self->v_alloc.last_block->used_block;
                    self->allocation_amount += 1;

                    buffer_address = (ptrdiff_t)self->v_alloc.last_block; // Given that the block is the actual buffer, that's the address to give to start_address
                    current_address = buffer_address + self->current_offset;
                    aligned_address = hdsa_common_align_forward(current_address, alignment);

                    /**
                     * Subtract away the buffer address number so aligned_offset becomes an actual
                     * relative offset like previous_offset and current_offset, instead of the
                     * huge address number aligned_address has.
                     */
                    aligned_offset = aligned_address - buffer_address;
                    printf("aligned offset: %lld\n", aligned_offset);

                    padding = aligned_offset -self->current_offset;
                }
            }

            self->previous_offset = aligned_offset;
            self->current_offset = aligned_offset + size;
            self->allocation_amount += 1;
            self->used_total += (padding + size);
            self->v_alloc.last_block->used_block += (padding + size);

            *allocation_out = (void*)aligned_address;
            error_out->code = HDSA_ERROR_SUCCESS;

            printf("previous_offset after: %lld\n", self->previous_offset);
            printf("current_offset after: %lld\n", self->current_offset);
            printf("remaining_space after: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("allocation_amount after: %lld\n\n\n", self->allocation_amount);

            return true;
        } break;
    }

    error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
    HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Wrong mode integer, __LINE__ - 3, "The Allocator_Arena mode isn't External, Static nor Dynamic.");
    *allocation_out = (void*)0;

    return false;
}

/**
 * @brief It does nothing. Arena allocators don't do individual deallocations,
 * they only deallocate the whole buffer at once.
 */
void hdsa_allocator_arena_deallocate(hdsa_Allocator_Arena* const self, void* ptr)
{}

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
void hdsa_allocator_arena_reset(hdsa_Allocator_Arena* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");

    hdsa_allocator_virtual_reset(&self->v_alloc);

    if (!self->v_alloc.last_block) // External mode always has a null last block
    {
        self->previous_offset = 0;
        self->current_offset = 0;
        self->used_total = 0;
        self->allocation_amount = 0;

        puts("The Allocator_Arena was empty or in External mode when trying to reset it. All the allocator data was set to 0.\n");

        return;
    }

    self->v_alloc.last_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->previous_offset = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->current_offset = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->used_total = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->allocation_amount = 1;

    puts("The Allocator_Arena has been reset!\n");
}

/**
 * @brief It deallocates all the memory blocks and sets all the data in the
 * allocator and "v_alloc" to 0/null.
 *
 * For External mode the pointer to the buffer is set to null.
 * @param[in] self The Allocator_Arena pointer, must not be null.
 */
void hdsa_allocator_arena_give_back_memory(hdsa_Allocator_Arena* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");

    hdsa_allocator_virtual_give_back_memory(&self->v_alloc);

    self->previous_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;

    puts("All the memory in the Allocator_Arena was given back to the OS or user.\n");
}

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
    void* const old_memory,
    ptrdiff_t old_size,
    ptrdiff_t new_size,
    ptrdiff_t new_alignment,
    void** const allocation_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc.committed_total > 0), "The committed_total is 0 or negative.\n");
    HDSA_BASIC_ASSERT((old_size > -1), "The size for the existing allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_size > -1), "The size for the resized/new allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_alignment > 0), "The new alignment can't be negative nor 0.\n");

    puts("===================   REALLOCATION!   ==================\n");

    void* new_memory = (void*)0;
    ptrdiff_t real_new_alignment = 0, previous_address = 0, buffer_address = 0, end_address = 0;

    switch(self->v_alloc.mode)
    {
        case HDSA_ALLOCATOR_VIRTUAL_EXTERNAL:
        {
            // If both sizes are the same nothing is done
            if (old_size == new_size)
            {
                error_out->code = HDSA_ERROR_SUCCESS;
                *allocation_out = old_memory;

                return true;
            }

            /**
             * The new size being 0 means only deallocation, but arena allocators don't do
             * individual deallocations
             */
            if (new_size == 0)
            {
                *allocation_out = old_memory;
                error_out->code = HDSA_ERROR_SUCCESS;

                return true;
            }

            if (!self->v_alloc.buffer)
            {
                self->previous_offset = 0;
                self->current_offset = 0;
                self->allocation_amount = 0;
                self->used_total = 0;

                self->v_alloc.reserved_total = 0;
                self->v_alloc.committed_total = 0;
                self->v_alloc.last_block = (hdsa_Allocator_Memory_Block*)0;

                error_out->code = HDSA_ERROR_ALLOCATION_NULL_BUFFER;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->v_alloc.buffer, __LINE__ - 12, "While in External mode the Allocator_Arena has a null buffer when trying to resize an allocation.");

                *allocation_out = (void*)0;
                return false;
            }

            real_new_alignment = (hdsa_common_is_power_of_two(new_alignment)) ? new_alignment : hdsa_common_round_pow_two(new_alignment);

            buffer_address = (ptrdiff_t)self->v_alloc.buffer;
            end_address = buffer_address + self->v_alloc.committed_total;
            previous_address = buffer_address + self->previous_offset;

            /**
             * If old-memory isn't the last allocation or even if it is, if the alignment
             * on the latest allocation isn't the same as "new_alignment" then do a new
             * allocation.
             */
            if
            (
                ((ptrdiff_t)old_memory != previous_address) ||
                (!hdsa_common_is_aligned((void*)previous_address, real_new_alignment))
            )
            {
                if (!hdsa_allocator_arena_allocate(self, new_size, real_new_alignment, (void**)&new_memory, error_out))
                {
                    *allocation_out = new_memory;
                    return false;
                }

                /**
                 * If old_memory isn't null and old_size is at least 1 copy the data from
                 * old_memory to the new allocation in the arena.
                 */
                if (old_memory && (old_size > 0))
                {
                    // Choose the smallest of the 2 sizes and copy the data from old_memory to new_memory
                    ptrdiff_t copy_size = (old_size < new_size) ? old_size : new_size;
                    memcpy(new_memory, old_memory, (size_t)copy_size);
                }

                *allocation_out = new_memory;
                error_out->code = HDSA_ERROR_SUCCESS;

                return true;
            }

            // Checking if new_size doesn't fit inside the buffer as the size of the latest allocation
            if ((previous_address + new_size) > end_address)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Not enough space, __LINE__ - 1, "The buffer of Allocator_Arena doesn't have enough space for new allocation size.");

                return false;
            }

            self->used_total -= (self->current_offset - self->previous_offset); // Subtracting the existing last allocation's size from used_total
            self->current_offset = self->previous_offset + new_size;
            self->used_total += (self->current_offset - self->previous_offset); // Adding the new allocation's size to used_total

            printf("Resizing the latest allocation. current_offset is now: %lld\n", self->current_offset);
            *allocation_out = old_memory;
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATOR_VIRTUAL_STATIC:
        case HDSA_ALLOCATOR_VIRTUAL_DYNAMIC:
        {
            // If both sizes are the same nothing is done
            if (old_size == new_size)
            {
                error_out->code = HDSA_ERROR_SUCCESS;
                *allocation_out = old_memory;

                return true;
            }

            /**
             * The new size being 0 means only deallocation, but arena allocators don't do
             * individual deallocations
             */
            if (new_size == 0)
            {
                *allocation_out = old_memory;
                error_out->code = HDSA_ERROR_SUCCESS;

                return true;
            }

            real_new_alignment = (hdsa_common_is_power_of_two(new_alignment)) ? new_alignment : hdsa_common_round_pow_two(new_alignment);

            if (!self->v_alloc.last_block)
            {
                ptrdiff_t maximum_size =  new_size + real_new_alignment - 1;

                if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
                {
                    if (!hdsa_allocator_arena_init_static(self, maximum_size * 4, maximum_size, error_out))
                    {
                        *allocation_out = (void*)0;
                        return false;
                    }
                }
                else
                {
                    if (!hdsa_allocator_arena_init_dynamic(self, maximum_size * 4, maximum_size, error_out))
                    {
                        *allocation_out = (void*)0;
                        return false;
                    }
                }
            }

            buffer_address = (ptrdiff_t)self->v_alloc.last_block;
            end_address = buffer_address + self->v_alloc.committed_total;
            previous_address = buffer_address + self->previous_offset;

            /**
             * If old-memory isn't the last allocation or even if it is, if the alignment
             * on the latest allocation isn't the same as "new_alignment" then do a new
             * allocation.
             */
            if
            (
                ((ptrdiff_t)old_memory != previous_address) ||
                (!hdsa_common_is_aligned((void*)previous_address, real_new_alignment)) ||
                ((previous_address + new_size) > end_address)
            )
            {
                if (!hdsa_allocator_arena_allocate(self, new_size, real_new_alignment, (void**)&new_memory, error_out))
                {
                    *allocation_out = new_memory;
                    return false;
                }

                /**
                 * If old_memory isn't null and old_size is at least 1 copy the data from
                 * old_memory to the new allocation in the arena.
                 */
                if (old_memory && (old_size > 0))
                {
                    // Choose the smallest of the 2 sizes and copy the data from old_memory to new_memory
                    ptrdiff_t copy_size = (old_size < new_size) ? old_size : new_size;
                    memcpy(new_memory, old_memory, (size_t)copy_size);
                }

                // Here should go the deallocation of old_memory if this wasn't an Arena

                *allocation_out = new_memory;
                error_out->code = HDSA_ERROR_SUCCESS;

                return true;
            }

            self->used_total -= (self->current_offset - self->previous_offset); // Subtracting the existing last allocation's size from used_total
            self->v_alloc.last_block->used_block -= (self->current_offset - self->previous_offset);

            self->current_offset = self->previous_offset + new_size;

            self->used_total += (self->current_offset - self->previous_offset); // Adding the new allocation's size to used_total
            self->v_alloc.last_block->used_block += (self->current_offset - self->previous_offset);

            printf("Resizing the latest allocation. current_offset is now: %lld\n", self->current_offset);
            *allocation_out = old_memory;
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;
    }

    error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
    HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Wrong mode integer, __LINE__ - 3, "The Allocator_Arena mode isn't External, Static nor Dynamic.");
    *allocation_out = (void*)0;

    return false;
}

/**
 * @brief Generic wrapper function for the Allocator_Interface. It has all the
 * parameters any allocation function could need, but depending on the
 * Allocation_Type value some of the parameters will be ignored.
 */
bool hdsa_allocator_arena_generic_function
(
    void* const allocator,
    hdsa_Allocator_Behavior alloc_func,
    void* old_allocation,
    ptrdiff_t old_size,
    ptrdiff_t new_size,
    ptrdiff_t new_alignment,
    void** const allocation_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((allocator), "The Allocator_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((old_size > -1), "The size for the existing allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_size > -1), "The size for the resized/new allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_alignment > 0), "The new alignment can't be negative nor 0.\n");

    switch(alloc_func)
    {
        case HDSA_ALLOCATION_TYPE_ALLOCATION:
        {
            return hdsa_allocator_arena_allocate((hdsa_Allocator_Arena* const)allocator, new_size, new_alignment, allocation_out, error_out);
        } break;

        case HDSA_ALLOCATION_TYPE_DEALLOCATION:
        {
            error_out->code = HDSA_ERROR_SUCCESS;
            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_RESET:
        {
            hdsa_allocator_arena_reset((hdsa_Allocator_Arena* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_GIVE_BACK_MEMORY:
        {
            hdsa_allocator_arena_give_back_memory((hdsa_Allocator_Arena* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_RESIZE:
        {
            return hdsa_allocator_arena_resize((hdsa_Allocator_Arena* const)allocator, old_allocation, old_size, new_size, new_alignment, allocation_out, error_out);
        } break;

        default:
        {
            error_out->code = HDSA_ERROR_NOT_IMPLEMENTED;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Invalidad allocation type, __LINE__ - 1, "The provided allocation type doesn't exist or is yet to be implemented.");

            return false;
        } break;
    }

    return false;
}

/**
 * @brief It creates a new Allocator_Interface struct from an Allocator_Arena
 * pointer.
 * @param[in] self Pointer to the Allocator_Arena, must not be null.
 * @returns A copy to a new Allocator_Interface struct with self as the
 * "allocator", and hdsa_allocator_arena_generic_function as the
 * "generic_alloc_function".
 */
hdsa_Allocator_Interface hdsa_allocator_arena_create_interface(const hdsa_Allocator_Arena* const self)
{
    return (hdsa_Allocator_Interface){
        (void*)self,
        &hdsa_allocator_arena_generic_function
    };
}

/**
 * @brief It connects the Allocator_Temp_Arena with the Allocator_Arena to be
 * used as a short lived last allocation that will be reverted later.
 * @param[in] self Temp_Arena pointer, must not be null.
 * @param[in] ar Arena_Alloc pointer, must not be null.
 */
void hdsa_allocator_temp_arena_init(hdsa_Allocator_Temp_Arena* const self, hdsa_Allocator_Arena* const ar)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Arena pointer null.\n");
    HDSA_BASIC_ASSERT((ar), "The Allocator_Arena pointer is null, it can't be used for initializing a Temp Arena.\n");
    HDSA_BASIC_ASSERT((ar->previous_offset > -1), "The Allocator_Arena previous_offset is negative.\n");
    HDSA_BASIC_ASSERT((ar->current_offset > -1), "The Allocator_Arena current_offset is negative.\n");

    self->arena = ar;
    self->pre_offset = self->arena->previous_offset;
    self->cur_offset = self->arena->current_offset;
    self->al_amount = self->arena->allocation_amount;
    self->u_total = self->arena->used_total;

    printf("The TempArena pre_offset and cur_offset are: %lld, %lld\n", self->pre_offset, self->cur_offset);
}

/**
 * @brief Cut ties with the current Allocator_Arena manually, so it can be used
 * by another one before the Allocator_Temp_Arena goes out of scope.
 * @param[in] self Temp_Arena pointer, must not be null.
 */
void hdsa_allocator_temp_arena_end(hdsa_Allocator_Temp_Arena* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Temp_Arena pointer is null.\n");
    HDSA_BASIC_ASSERT((self->arena), "The Allocator_Temp_Arena arena is null.\n");
    HDSA_BASIC_ASSERT((self->pre_offset > -1), "The Allocator_Arena previous_offset is negative.\n");
    HDSA_BASIC_ASSERT((self->cur_offset > -1), "The Allocator_Arena current_offset is negative.\n");

    self->arena->previous_offset = self->pre_offset;
    self->arena->current_offset = self->cur_offset;
    self->arena->allocation_amount = self->al_amount;
    self->arena->used_total = self->u_total;

    self->pre_offset = 0;
    self->cur_offset = 0;
    self->al_amount = 0;
    self->u_total = 0;
    self->arena = (hdsa_Allocator_Arena*)0;
}
