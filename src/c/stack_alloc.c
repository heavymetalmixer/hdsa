#include "stack_alloc.h"


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
void hdsa_allocator_stack_init_external(hdsa_Allocator_Stack* const self, void* buffer, ptrdiff_t size)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "The buffer size to initialize the Allocator_Stack is less than 1.\n");

    hdsa_allocator_virtual_init_external(&self->v_alloc, buffer, size);
    self->start_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;
}

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
bool hdsa_allocator_stack_init_static(hdsa_Allocator_Stack* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The reserved_size to initialize the Allocator_Stack is less than 1.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The buffer size to initialize the Allocator_Stack is less than 1.\n");

    self->start_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;

    if (!hdsa_allocator_virtual_init_static(&self->v_alloc, reserved_size, committed_size, error_out))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        hdsa_Allocator_Stack_Last* last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
        last_alloc->offset = 0;
        last_alloc->size = 0;
        self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last);

        self->start_offset = self->v_alloc.last_block->used_block;
        self->current_offset = self->v_alloc.last_block->used_block;
        self->used_total += self->v_alloc.last_block->used_block;
        self->allocation_amount += 2;
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

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
bool hdsa_allocator_stack_init_dynamic(hdsa_Allocator_Stack* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The reserved_size to initialize the Allocator_Stack is less than 1.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The buffer size to initialize the Allocator_Stack is less than 1.\n");

    self->start_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;

    if (!hdsa_allocator_virtual_init_dynamic(&self->v_alloc, reserved_size, committed_size, error_out))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        hdsa_Allocator_Stack_Last* last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
        last_alloc->offset = 0;
        last_alloc->size = 0;
        self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last);

        self->start_offset = self->v_alloc.last_block->used_block;
        self->current_offset = self->v_alloc.last_block->used_block;
        self->used_total += self->v_alloc.last_block->used_block;
        self->allocation_amount += 2;
    }

    return true;
}

/**
 * @param[in] self Allocator_Stack pointer, must not be null.
 * @return Amount of bytes yet to be allocated in the buffer. For Static and
 * Dynamic mode it's the remaining space in the committed memory of the last
 * block. If the buffer/last_block is null 0 is returned.
 */
ptrdiff_t hdsa_allocator_stack_remaining_storage(const hdsa_Allocator_Stack* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");

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
bool hdsa_allocator_stack_allocate(hdsa_Allocator_Stack* const self, ptrdiff_t size, ptrdiff_t alignment, void** const allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((size > 0), "Allocation sizes of 0 or negative aren't allowed.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "Allocation alignments of 0 or negative aren't allowed.\n");

    puts("###############  STACK ALLOCATION!   ################\n");

    ptrdiff_t buffer_address = 0, current_address = 0, next_address = 0, padding = 0;
    hdsa_Allocator_Stack_Header* header = (hdsa_Allocator_Stack_Header*)0;
    alignment = (hdsa_common_is_power_of_two(alignment)) ? alignment : hdsa_common_round_pow_two(alignment);

    switch (self->v_alloc.mode)
    {
        case HDSA_ALLOCATOR_VIRTUAL_EXTERNAL:
        {
            if (!self->v_alloc.buffer)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NULL_BUFFER;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->v_alloc.buffer, __LINE__ - 3, "The Allocator_Stack's buffer is null, the pointer inside allocation_out will be set to null.\n");
                *allocation_out = (void*)0;

                return false;
            }

            printf("allocation size: %lld\n", size);
            printf("allocation alignment: %lld\n", alignment);
            printf("allocation_amount before: %lld\n", self->allocation_amount);
            printf("remaining_space before: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("start_offset before: %lld\n", self->start_offset);
            printf("current_offset before: %lld\n", self->current_offset);

            buffer_address = (ptrdiff_t)self->v_alloc.buffer;
            current_address = buffer_address + self->current_offset;
            padding = hdsa_common_padding_with_header(current_address, (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header), (ptrdiff_t)alignment);

            printf("padding from the previous allocation: %lld\n", padding);

            if ((self->current_offset + padding + size) > self->v_alloc.committed_total)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, (current_offset + size) > self->buffer_length, __LINE__ - 3, "The Allocator_Stack can't fit the new allocation in the buffer.");
                *allocation_out = (void*)0;

                return false;
            }

            next_address = current_address + padding;
            header = (hdsa_Allocator_Stack_Header*)(next_address - (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header));
            header->padding = padding;
            header->start_offset = self->start_offset; // Store the previous start_offset in the header

            printf("current_adress: %lld\n", current_address);
            printf("next_adress: %lld\n", next_address);
            printf("next_offset: %lld\n", (next_address - buffer_address));
            printf("header_offset: %lld\n", (ptrdiff_t)header - buffer_address);
            printf("header->start_offset: %lld\n", header->start_offset);
            printf("header->padding: %lld\n", header->padding);

            self->start_offset = next_address - buffer_address;
            self->current_offset = self->start_offset + size;
            self->allocation_amount += 1;
            self->used_total += (padding + size);

            printf("start_offset after: %lld\n", self->start_offset);
            printf("current_offset after: %lld\n", self->current_offset);
            printf("remaining_space after: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("allocation_amount after: %lld\n\n\n", self->allocation_amount);

            *allocation_out = (void*)next_address;
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
                    if (!hdsa_allocator_stack_init_static(self, maximum_size * 4, maximum_size, error_out))
                    {
                        return false;
                    }
                }
                else
                {
                    if (!hdsa_allocator_stack_init_dynamic(self, maximum_size * 4, maximum_size, error_out))
                    {
                        return false;
                    }
                }
            }

            hdsa_Allocator_Stack_Last* last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));

            printf("allocation size: %lld\n", size);
            printf("allocation alignment: %lld\n", alignment);
            printf("allocation_amount before: %lld\n", self->allocation_amount);
            printf("remaining_space before: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("start_offset before: %lld\n", self->start_offset);
            printf("current_offset before: %lld\n", self->current_offset);
            printf("Current block's last alloc offset before: %lld\n", last_alloc->offset);
            printf("Current block's last alloc size before: %lld\n\n", last_alloc->size);

            buffer_address = (ptrdiff_t)self->v_alloc.last_block;
            current_address = buffer_address + self->current_offset;
            padding = hdsa_common_padding_with_header(current_address, (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header), (ptrdiff_t)alignment);

            printf("padding from the previous allocation: %lld\n", padding);

            if ((self->current_offset + padding + size) > self->v_alloc.last_block->committed_block)
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
                    printf("allocation size: %lld\n", size);
                    printf("allocation_amount before: %lld\n", self->allocation_amount);
                    printf("remaining_space before: %lld\n", self->v_alloc.committed_total - self->current_offset);
                    printf("start_offset before: %lld\n", self->start_offset);
                    printf("current_offset before: %lld\n", self->current_offset);

                    last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
                    last_alloc->offset = 0;
                    last_alloc->size = 0;
                    self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last);

                    self->used_total += self->v_alloc.last_block->used_block;
                    self->start_offset = self->v_alloc.last_block->used_block;
                    self->current_offset = self->v_alloc.last_block->used_block;
                    self->allocation_amount += 2;

                    buffer_address = (ptrdiff_t)self->v_alloc.last_block; // Given that the block is the actual buffer, that's the address to give to start_address
                    current_address = buffer_address + self->current_offset;
                    padding = hdsa_common_padding_with_header(current_address, (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header), (ptrdiff_t)alignment);

                    printf("padding from the previous allocation: %lld\n", padding);
                }
            }

            next_address = current_address + padding;
            header = (hdsa_Allocator_Stack_Header*)(next_address - (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header));
            header->padding = padding;
            header->start_offset = self->start_offset; // Store the previous start_offset in the header

            printf("current_adress: %lld\n", current_address);
            printf("next_adress: %lld\n", next_address);
            printf("next_offset: %lld\n", (next_address - buffer_address));
            printf("header_offset: %lld\n", (ptrdiff_t)header - buffer_address);
            printf("header->start_offset: %lld\n", header->start_offset);
            printf("header->padding: %lld\n", header->padding);

            self->start_offset = next_address - buffer_address;
            self->current_offset = self->start_offset + size;
            self->allocation_amount += 1;
            self->used_total += (padding + size);
            self->v_alloc.last_block->used_block += (padding + size);

            last_alloc->offset = self->start_offset;
            last_alloc->size = self->current_offset - self->start_offset;

            printf("start_offset after: %lld\n", self->start_offset);
            printf("current_offset after: %lld\n", self->current_offset);
            printf("remaining_space after: %lld\n", self->v_alloc.committed_total - self->current_offset);
            printf("allocation_amount after: %lld\n", self->allocation_amount);
            printf("Current block's last alloc offset before: %lld\n", last_alloc->offset);
            printf("Current block's last alloc size before: %lld\n\n\n", last_alloc->size);

            *allocation_out = (void*)next_address;
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;
    }

    error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
    HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Wrong mode integer, __LINE__ - 3, "The Allocator_Stack mode isn't External, Static nor Dynamic.");
    *allocation_out = (void*)0;

    return false;
}

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
void hdsa_allocator_stack_deallocate(hdsa_Allocator_Stack* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");

    puts("----------------   STACK DEALLOCATION!   ------------------\n");

    ptrdiff_t buffer_address = 0, start_address = 0, temp_prev_offset = 0;
    hdsa_Allocator_Stack_Header* header = (hdsa_Allocator_Stack_Header*)0;
    hdsa_Allocator_Stack_Last* last_alloc = (hdsa_Allocator_Stack_Last*)0;

    switch (self->v_alloc.mode)
    {
        case HDSA_ALLOCATOR_VIRTUAL_EXTERNAL:
        {
            if (!self->v_alloc.buffer || (self->current_offset == 0))
            {
                return;
            }

            buffer_address = (ptrdiff_t)self->v_alloc.buffer;
            start_address = (ptrdiff_t)self->start_offset + buffer_address;

            printf("allocation_amount before: %lld\n", self->allocation_amount);
            printf("start_offset and current_offset before allocation: %lld, %lld\n", self->start_offset, self->current_offset);
            printf("start_address: %lld\n\n", start_address);

            header = (hdsa_Allocator_Stack_Header*)(start_address - (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header));
            temp_prev_offset = self->start_offset - header->padding;

            printf("header offset %lld\n", ((ptrdiff_t)header - buffer_address));
            printf("header: %lld\n", (ptrdiff_t)header);
            printf("header->padding: %lld\n", header->padding);
            printf("header->start_offset: %lld\n", header->start_offset);

            self->used_total -= (self->current_offset - self->start_offset) + header->padding;
            self->current_offset = temp_prev_offset;
            self->start_offset = header->start_offset;
            self->allocation_amount -= 1;

            printf("start_offset and current_offset after allocation: %lld, %lld\n", self->start_offset, self->current_offset);
            printf("allocation_amount after: %lld\n", self->allocation_amount);
            printf("remaining space: %lld\n\n\n", (self->v_alloc.committed_total - self->current_offset));
        } break;

        case HDSA_ALLOCATOR_VIRTUAL_STATIC:
        case HDSA_ALLOCATOR_VIRTUAL_DYNAMIC:
        {
            if (!self->v_alloc.last_block)
            {
                return;
            }

            /**
             * If the only data inside the last Memory Block are the 2 headers, deallocate
             * that block.
             */
            if (self->current_offset == ((ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block) + (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last)))
            {
                self->used_total -= self->v_alloc.last_block->used_block;
                hdsa_allocator_virtual_deallocate(&self->v_alloc);
                self->allocation_amount -= 2;

                if (!self->v_alloc.last_block)
                {
                    printf("The Allocator_Stack just deallocated its only block.\n");

                    return;
                }

                last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
                self->start_offset = last_alloc->offset;
                self->current_offset = self->start_offset + last_alloc->size;
            }

            last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
            buffer_address = (ptrdiff_t)self->v_alloc.last_block;
            start_address = (ptrdiff_t)self->start_offset + buffer_address;

            printf("allocation_amount before: %lld\n", self->allocation_amount);
            printf("start_offset and current_offset before: %lld, %lld\n", self->start_offset, self->current_offset);
            printf("start_address: %lld\n", start_address);
            printf("Current block's last alloc offset before: %lld\n", last_alloc->offset);
            printf("Current block's last alloc size before: %lld\n\n", last_alloc->size);

            header = (hdsa_Allocator_Stack_Header*)(start_address - (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Header));
            temp_prev_offset = self->start_offset - header->padding;

            printf("header offset %lld\n", ((ptrdiff_t)header - buffer_address));
            printf("header: %lld\n", (ptrdiff_t)header);
            printf("header->padding: %lld\n", header->padding);
            printf("header->start_offset: %lld\n", header->start_offset);

            self->used_total -= (self->current_offset - self->start_offset) + header->padding;
            self->v_alloc.last_block->used_block -= (self->current_offset - self->start_offset) + header->padding;
            self->current_offset = temp_prev_offset;
            self->start_offset = header->start_offset;

            self->allocation_amount -= 1;
            last_alloc->offset = (self->start_offset != ((ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block) + (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last))) ? self->start_offset : 0;
            last_alloc->size = (self->current_offset != ((ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block) + (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last))) ? (self->current_offset - self->start_offset) : 0;

            printf("start_offset and current_offset after: %lld, %lld\n", self->start_offset, self->current_offset);
            printf("allocation_amount after: %lld\n", self->allocation_amount);
            printf("remaining space: %lld\n", (self->v_alloc.committed_total - self->current_offset));
            printf("Current block's last alloc offset after: %lld\n", last_alloc->offset);
            printf("Current block's last alloc size after: %lld\n\n\n", last_alloc->size);
        } break;

        default:
        {
            return;
        } break;
    }
}

/**
 * @brief It deallocates all the memory blocks except the first one and sets
 * the offsets, v_alloc.last_block->used_block and used_total to the size of
 * hdsa_Memory_Block + hdsa_Allocator_Stack_Last, so allocations are done from
 * the beginning of the remaining block. allocation_amount is set to 1 as it
 * represents the block's header allocation.
 *
 * If there are no memory blocks for Static and Dynamic modes, or if the
 * allocator is in External mode all data members are set to 0/null.
 * @param[in] self Allocator_Stack pointer, must not be null.
 */
void hdsa_allocator_stack_reset(hdsa_Allocator_Stack* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");

    hdsa_allocator_virtual_reset(&self->v_alloc);

    if (!self->v_alloc.last_block) // External mode always has a null last block
    {
        self->start_offset = 0;
        self->current_offset = 0;
        self->used_total = 0;
        self->allocation_amount = 0;

        puts("The Allocator_Stack was empty or in External mode when trying to reset it. All the allocator data was set to 0.\n");

        return;
    }

    hdsa_Allocator_Stack_Last* last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
    last_alloc->offset = 0;
    last_alloc->size = 0;
    self->v_alloc.last_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block) + (ptrdiff_t)sizeof(hdsa_Allocator_Stack_Last);

    self->start_offset = self->v_alloc.last_block->used_block;
    self->current_offset = self->v_alloc.last_block->used_block;
    self->used_total = self->v_alloc.last_block->used_block;
    self->allocation_amount = 2;

    puts("The Allocator_Stack has been reset!\n");
}

/**
 * @brief It deallocates all the memory blocks and sets all the data in the
 * allocator and "v_alloc" to 0/null.
 *
 * For External mode the pointer to the buffer is set to null.
 * @param[in] self The Allocator_Stack pointer, must not be null.
 */
void hdsa_allocator_stack_give_back_memory(hdsa_Allocator_Stack* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");

    hdsa_allocator_virtual_give_back_memory(&self->v_alloc);

    self->start_offset = 0;
    self->current_offset = 0;
    self->allocation_amount = 0;
    self->used_total = 0;

    puts("All the memory in the Allocator_Stack was given back to the OS or user.\n");
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
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc.committed_total > 0), "The committed_total is 0 or negative.\n");
    HDSA_BASIC_ASSERT((old_size > -1), "The size for the existing allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_size > -1), "The size for the resized/new allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_alignment > 0), "The new alignment can't be negative nor 0.\n");

    puts("*****************   STACK RESIZING!   ******************\n");

    void* new_memory = (void*)0;
    ptrdiff_t real_new_alignment = 0, start_address = 0, buffer_address = 0, end_address = 0;

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
             * The new size being 0 means only deallocation, and that results
             * in the latest allocation being deallocated regardless of the
             * address inside old_memory.
             */
            if (new_size == 0)
            {
                hdsa_allocator_stack_deallocate(self);
                *allocation_out = old_memory;
                error_out->code = HDSA_ERROR_SUCCESS;

                return true;
            }

            if (!self->v_alloc.buffer)
            {
                self->start_offset = 0;
                self->current_offset = 0;
                self->allocation_amount = 0;
                self->used_total = 0;

                self->v_alloc.reserved_total = 0;
                self->v_alloc.committed_total = 0;
                self->v_alloc.last_block = (hdsa_Allocator_Memory_Block*)0;

                error_out->code = HDSA_ERROR_ALLOCATION_NULL_BUFFER;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->v_alloc.buffer, __LINE__ - 12, "While in External mode the Allocator_Stack has a null buffer when trying to resize an allocation.");

                *allocation_out = (void*)0;
                return false;
            }

            real_new_alignment = (hdsa_common_is_power_of_two(new_alignment)) ? new_alignment : hdsa_common_round_pow_two(new_alignment);

            buffer_address = (ptrdiff_t)self->v_alloc.buffer;
            end_address = buffer_address + self->v_alloc.committed_total;
            start_address = buffer_address + self->start_offset;

            /**
             * If old-memory isn't the last allocation or even if it is, if the alignment
             * on the latest allocation isn't the same as "new_alignment" then do a new
             * allocation.
             */
            if
            (
                ((ptrdiff_t)old_memory != start_address) ||
                (!hdsa_common_is_aligned((void*)start_address, real_new_alignment))
            )
            {
                if (!hdsa_allocator_stack_allocate(self, new_size, real_new_alignment, (void**)&new_memory, error_out))
                {
                    *allocation_out = new_memory;
                    return false;
                }

                /**
                 * If old_memory isn't null and old_size is at least 1 copy the data from
                 * old_memory to the new allocation in the stack.
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
            if ((start_address + new_size) > end_address)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Not enough space, __LINE__ - 1, "The buffer of Allocator_Stack doesn't have enough space for new allocation size.");

                return false;
            }

            self->used_total -= (self->current_offset - self->start_offset); // Subtracting the existing last allocation's size from used_total
            self->current_offset = self->start_offset + new_size;
            self->used_total += (self->current_offset - self->start_offset); // Adding the new allocation's size to used_total

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
             * The new size being 0 means only deallocation, and that results
             * in the latest allocation being deallocated.
             */
            if (new_size == 0)
            {
                hdsa_allocator_stack_deallocate(self);
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
                    if (!hdsa_allocator_stack_init_static(self, maximum_size * 4, maximum_size, error_out))
                    {
                        *allocation_out = (void*)0;
                        return false;
                    }
                }
                else
                {
                    if (!hdsa_allocator_stack_init_dynamic(self, maximum_size * 4, maximum_size, error_out))
                    {
                        *allocation_out = (void*)0;
                        return false;
                    }
                }
            }

            buffer_address = (ptrdiff_t)self->v_alloc.last_block;
            end_address = buffer_address + self->v_alloc.committed_total;
            start_address = buffer_address + self->start_offset;

            /**
             * If old-memory isn't the last allocation or even if it is, if the alignment
             * on the latest allocation isn't the same as "new_alignment" then do a new
             * allocation.
             */
            if
            (
                ((ptrdiff_t)old_memory != start_address) ||
                (!hdsa_common_is_aligned((void*)start_address, real_new_alignment)) ||
                ((start_address + new_size) > end_address)
            )
            {
                if (!hdsa_allocator_stack_allocate(self, new_size, real_new_alignment, (void**)&new_memory, error_out))
                {
                    *allocation_out = new_memory;
                    return false;
                }

                /**
                 * If old_memory isn't null and old_size is at least 1 copy the data from
                 * old_memory to the new allocation in the stack.
                 */
                if (old_memory && (old_size > 0))
                {
                    // Choose the smallest of the 2 sizes and copy the data from old_memory to new_memory
                    ptrdiff_t copy_size = (old_size < new_size) ? old_size : new_size;
                    memcpy(new_memory, old_memory, (size_t)copy_size);
                }

                // Here should go the deallocation of old_memory if this wasn't an Stack

                *allocation_out = new_memory;
                error_out->code = HDSA_ERROR_SUCCESS;

                return true;
            }

            hdsa_Allocator_Stack_Last* last_alloc = (hdsa_Allocator_Stack_Last*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
            self->used_total -= (self->current_offset - self->start_offset); // Subtracting the existing last allocation's size from used_total
            self->v_alloc.last_block->used_block -= (self->current_offset - self->start_offset);

            self->current_offset = self->start_offset + new_size;
            last_alloc->size = new_size;

            self->used_total += (self->current_offset - self->start_offset); // Adding the new allocation's size to used_total
            self->v_alloc.last_block->used_block += (self->current_offset - self->start_offset);

            printf("Resizing the latest allocation. current_offset is now: %lld\n", self->current_offset);
            *allocation_out = old_memory;
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;
    }

    error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
    HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Wrong mode integer, __LINE__ - 3, "The Allocator_Stack mode isn't External, Static nor Dynamic.");
    *allocation_out = (void*)0;

    return false;
}

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
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((allocator), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((old_size > -1), "The size for the existing allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_size > -1), "The size for the resized/new allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_alignment > 0), "The new alignment can't be negative nor 0.\n");

    switch(alloc_func)
    {
        case HDSA_ALLOCATION_TYPE_ALLOCATION:
        {
            return hdsa_allocator_stack_allocate((hdsa_Allocator_Stack* const)allocator, new_size, new_alignment, allocation_out, error_out);
        } break;

        case HDSA_ALLOCATION_TYPE_DEALLOCATION:
        {
            hdsa_allocator_stack_deallocate((hdsa_Allocator_Stack* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;
            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_RESET:
        {
            hdsa_allocator_stack_reset((hdsa_Allocator_Stack* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_GIVE_BACK_MEMORY:
        {
            hdsa_allocator_stack_give_back_memory((hdsa_Allocator_Stack* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_RESIZE:
        {
            return hdsa_allocator_stack_resize((hdsa_Allocator_Stack* const)allocator, old_allocation, old_size, new_size, new_alignment, allocation_out, error_out);
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
 * @brief It creates a new Allocator_Interface struct from an Allocator_Stack
 * pointer.
 * @param[in] self Pointer to the Allocator_Stack, must not be null.
 * @returns A copy to a new Allocator_Interface struct with self as the
 * "allocator", and hdsa_allocator_stack_generic_function as the
 * "generic_alloc_function".
 */
hdsa_Allocator_Interface hdsa_allocator_stack_create_interface(const hdsa_Allocator_Stack* const self)
{
    return (hdsa_Allocator_Interface){
        (void*)self,
        &hdsa_allocator_stack_generic_function
    };
}
