#include "dynamic_array.h"

/**
 * @brief It initializes the Dynamic_Array with a given Allocator Virtual
 * pointer, and three numbers to determine the size of the buffer and how
 * big each "object" inside the array will be in bytes.
 *
 * @param[in] self Pointer to the dynamic array struct, must not be null.
 * @param[in] virtual_alloc An Allocator Virtual pointer to use internally, it
 * must be in Static Mode.
 * @param[in] object_amount Amount of "blocks" in array buffer will be divided
 * in. For the user this would be the "buffer's capacity" even though it's not
 * because the buffer is managed in bytes. It must be positive.
 * If object_amount is 0 the allocation is bypassed, the buffer is set to null
 * and buffer_capacity to 0.
 * @param[in] type_size Size of the object the user wants to store inside each
 * block of the array. It must be bigger than 0.
 * @param[in] type_alignment Alignment of the object to store inside the array.
 * It must be bigger than 0. If the value passed to it isn't a power of 2, the
 * function will round it up to the next power of 2.
 */
bool hdsa_dynamic_array_init
(
    hdsa_Dynamic_Array* const self,
    hdsa_Allocator_Virtual* const virtual_alloc,
    ptrdiff_t object_amount,
    ptrdiff_t type_size,
    ptrdiff_t type_alignment,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((virtual_alloc), "The Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((virtual_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The Allocator Virtual mode isn't Static.\n");
    HDSA_BASIC_ASSERT((object_amount > -1), "The amount of objects to initialize the hdsa_Dynamic_Array is negative.\n");
    HDSA_BASIC_ASSERT((type_size > 0), "The type_size is negative or 0.\n");
    HDSA_BASIC_ASSERT((type_alignment > 0), "The type_alignment is negative or 0.\n");

    self->v_alloc = virtual_alloc;
    self->buffer = (unsigned char*)0;
    self->type_alignment = (type_alignment > 0) ? hdsa_common_round_pow_two(type_alignment) : 0ll;
    self->type_size = (type_size > 0) ? hdsa_common_round_to_multiple(type_size, self->type_alignment) : 0;
    self->buffer_size = 0ll;
    self->buffer_capacity = 0ll;

    if ((object_amount > 0) && self->v_alloc && (self->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC))
    {
        /**
         * Gotta make sure enough space is allocated for all the requested objects,
         * even if the aligned address of self_buffer ends up being after the one
         * where the Memory Block ends.
         */
        ptrdiff_t committed_size = (object_amount * self->type_size) + (self->type_alignment - 1) + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);

        if (!self->v_alloc->last_block)
        {
            if (!hdsa_allocator_virtual_init_static(self->v_alloc, committed_size * 4, committed_size, error_out))
            {
                return false;
            }
        }

        if (self->v_alloc->last_block->reserved_block < hdsa_common_round_to_multiple(committed_size, hdsa_virtual_granularity_size()))
        {
            hdsa_allocator_virtual_give_back_memory(self->v_alloc);

            if (!hdsa_allocator_virtual_init_static(self->v_alloc, committed_size * 4, committed_size, error_out))
            {
                return false;
            }
        }

        if (self->v_alloc->last_block->committed_block < hdsa_common_round_to_multiple(committed_size, hdsa_virtual_page_size()))
        {
            void* temp_buffer;

            if (!hdsa_allocator_virtual_commit(self->v_alloc, committed_size - self->v_alloc->last_block->committed_block, (void**)&temp_buffer, error_out))
            {
                return false;
            }
        }

        self->buffer = (unsigned char*)hdsa_allocator_virtual_buffer(self->v_alloc);
        self->buffer = (unsigned char*)hdsa_common_align_forward((ptrdiff_t)self->buffer, self->type_alignment);

        ptrdiff_t header_difference = (ptrdiff_t)self->buffer - (ptrdiff_t)self->v_alloc->last_block;
        ptrdiff_t usable_memory = self->v_alloc->last_block->committed_block - header_difference;

        self->buffer_capacity = usable_memory / self->type_size;
        self->buffer_capacity *= self->type_size;
        self->v_alloc->last_block->used_block = self->buffer_capacity + header_difference;
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @returns Current amount of objects of size of type T the array
 * holds, not the current buffer_size.
 */
ptrdiff_t hdsa_dynamic_array_size(const hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");

    if (!self->buffer || (self->type_size < 1))
    {
        return 0;
    }

    return self->buffer_size / self->type_size;
}

/**
 * @returns Maximum amount of objects of size of type T the array
 * can hold, not the current buffer_capacity.
 */
ptrdiff_t hdsa_dynamic_array_capacity(const hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");

    if (!self->buffer || (self->type_size < 1))
    {
        return 0;
    }

    return self->buffer_capacity / self->type_size;
}

/**
 * @brief It returns the index to the last T size object in the array, not the
 * last real index in the buffer.
 *
 * If self->buffer_size is 0 then -1 is returned.
 */
ptrdiff_t hdsa_dynamic_array_last_index(const hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");

    return (self->buffer_size - self->type_size) / self->type_size;
}

/**
 * @brief It simply resets "buffer_size" to 0 so calling push() on this array
 * will be done from the beginning of the buffer.
 */
void hdsa_dynamic_array_reset_array(hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");

    self->buffer_size = 0ll;
}

/**
 * @brief It fills with zeroes the data from the index "position" (T sized)
 * n indexes above, with n being "object_amount".
 *
 * It doesn't use memset to avoid the compiler optimizing it away.
 *
 * If the array is empty (self->buffer_size being 0) the function does nothing.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] position The first position in the array to fill with 0s, must
 * be positive or 0.
 * @param[in] object_amount Amount of T-sized objects to fill with 0s in the
 * array. It must be at least 1, and if it goes above the size of the array
 * the whole self->buffer is filled with 0s.
 */
void hdsa_dynamic_array_zero_fill(hdsa_Dynamic_Array* const self, ptrdiff_t position, ptrdiff_t object_amount)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT(((position > -1) && (position < hdsa_dynamic_array_size(self))), "position is out of bounds.\n");
    HDSA_BASIC_ASSERT((object_amount > 0), "object_amount is negative or 0.\n");

    if
    (
        !self ||
        !self->buffer ||
        (self->buffer_size == 0) ||
        (position < 0) ||
        (position > hdsa_dynamic_array_last_index(self))||
        (object_amount < 1)
    )
    {
        return;
    }

    ptrdiff_t real_beginning = position * self->type_size;
    object_amount = (object_amount > (hdsa_dynamic_array_size(self) - position)) ? (hdsa_dynamic_array_size(self) - position) : object_amount; // Making sure that object_amount doesn't go above the array's size
    ptrdiff_t real_end = (position + object_amount) * self->type_size;

    for (ptrdiff_t i = real_beginning; i < real_end; ++i)
    {
        self->buffer[i] = 0;
    }
}

/**
 * @brief This one fills the buffer until (self->buffer_size - 1) with zeroes,
 * an expensive operation.
 *
 * Bear in mind: self->buffer_size isn't modified with this function as the
 * data after beginning won't be moved.
 *
 * It doesn't use memset to avoid the compiler optimizing it away.
 *
 * If the array is empty (self->buffer_size being 0) the function does nothing.
 */
void hdsa_dynamic_array_zero_fill_all(hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");

    if (self->buffer_size == 0)
    {
        return;
    }

    for (ptrdiff_t i = 0; i < self->buffer_size; ++i)
    {
        self->buffer[i] = 0;
    }
}

/**
 * @brief Copy data from the "data" pointer to self->buffer by the ammount in
 * "data_size" from "real_position" onwards.
 * @param[in] self The dynamic array pointer, must not be null. This is where
 * the data will be copied in.
 * @param[in] data Pointer with the data to copy, must no be null.
 * @param[in] data_size Amount of bytes to copy from "data". If it is 0 no copy
 * will be performed.
 * @param[in] real_position First index in "data" (measured in bytes) from
 * where the copy starts.
 */
static void hdsa_dynamic_array_copy_objects(hdsa_Dynamic_Array* const self, const unsigned char* const data, ptrdiff_t data_size, ptrdiff_t real_position)
{
    if (data_size == 0)
    {
        return;
    }

    for (ptrdiff_t i = 0; i < data_size; ++i)
    {
        self->buffer[i + real_position] = data[i];
    }
}

/**
 * @brief This function copies the data from real_position until the last byte,
 * and puts it in a higher position "n" bytes forward, with "n" being
 * "move_distance".
 *
 * It starts copying since the last byte until the first one in
 * "real_position".
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] move_distance How many bytes the data that starts from
 * real_position need to be copied above.
 * @param[in] real_position Byte (index) from where all the data will be
 * "moved", in other words, the last byte to copy in reverse.
 */
static void hdsa_dynamic_array_copy_to_move_up(hdsa_Dynamic_Array* const self, unsigned char* const destination, ptrdiff_t move_distance, ptrdiff_t real_position)
{
    if (move_distance == 0)
    {
        return;
    }

    for (ptrdiff_t i = (self->buffer_size + move_distance - 1); i > (real_position + move_distance - 1); --i)
    {
        destination[i] = self->buffer[i - move_distance];
    }
}

/**
 * @brief Growing strategy for the dynamic array divided in 3 cases according
 * to the internal Allocator Virtual for when adding data to the array makes
 * it go above the current self->buffer->capacity:
 *
 * 1) If the amount of bytes to add to the array doesn't go above the remaining
 * committed memory (minus the Memory Block's header size) then just increase
 * self->buffer_capacity.
 *
 * 2) If the amount of bytes to add to the array GOES above the remaining
 * committed memory (minus the Memory Block's header size), commit one more
 * page or the amount of bytes to add to the array rounded up to
 * hdsa_virtual_page_size() if its bigger than a page.
 *
 * 3) If the amount of bytes to add to the array goes above the remaining
 * RESERVED memory, create a new Memory Block 4 times as big, copy the data in
 * self->buffer after the header of the new Memory Block, deallocate the
 * previous Memory Block completely and give the new block to the array.
 * The usable new buffer starts after the header of the Block.
 *
 * In all the cases if the position isn't (hdsa_dynamic_array_last_index + 1),
 * the data from position and onwards will be copied into the next index.
 * @param[in] self The dynamic array pointer, must not be null. This one
 * holds to data to copied.
 * @param[in] excess Amount of bytes that go over self->buffer_capacity. It
 * must be at least 1.
 * @param[in] bytes_to_copy Amount of bytes to add to copy when called from
 * the insert functions, but it's ignored when called from other functions.
 * @param[in] insert_position Index of size T from where the data will be moved
 * forward (copied). It must be from 0 to (hdsa_dynamic_array_last_index + 1).
 * If you don't want the copy to happen then insert_position must be
 * (hdsa_dynamic_array_last_index + 1).
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds or isn't needed, false otherwise.
 */
static bool hdsa_dynamic_array_grow(hdsa_Dynamic_Array* const self, ptrdiff_t excess, ptrdiff_t bytes_to_copy, ptrdiff_t insert_position, hdsa_Error* const error_out)
{
    /**
     * In most cases header_difference will be the size of the header, but for
     * some alignments, like the ones SIMD types use, this size will be even
     * bigger because of the alignment on self->buffer.
     */
    ptrdiff_t header_difference = (ptrdiff_t)self->buffer - (ptrdiff_t)self->v_alloc->last_block;
    ptrdiff_t usable_memory = self->v_alloc->last_block->committed_block - header_difference;
    ptrdiff_t new_reserved_size = (self->v_alloc->last_block->reserved_block * 4) - header_difference;
    ptrdiff_t real_pos = insert_position * self->type_size;

    unsigned char* new_buffer;
    hdsa_Allocator_Virtual temp_alloc;

    if ((self->buffer_capacity + excess) <= usable_memory)
    {
        if (hdsa_dynamic_array_last_index((const hdsa_Dynamic_Array* const)self) >= insert_position)
        {
            hdsa_dynamic_array_copy_to_move_up(self, self->buffer, bytes_to_copy, real_pos);
        }
    }
    // else if (hdsa_allocator_virtual_commit(self->v_alloc, excess, (void**)&new_buffer, error_out))
    else if (hdsa_allocator_virtual_commit(self->v_alloc, (self->buffer_capacity + excess) - usable_memory, (void**)&new_buffer, error_out))
    {
        if (hdsa_dynamic_array_last_index((const hdsa_Dynamic_Array* const)self) >= insert_position)
        {
            hdsa_dynamic_array_copy_to_move_up(self, self->buffer, bytes_to_copy, real_pos);
        }

        excess = (hdsa_common_round_to_multiple((self->buffer_capacity + excess) - usable_memory, hdsa_virtual_page_size())) / self->type_size;
        excess *= self->type_size;
    }
    /**
     * Now try to create a new temporary Memory Block and copy all the data in
     * self->buffer inside of it. If it succeeds, copy the data in self->buffer to
     * it, deallocate its memory and make the temporary block the new last block
     * for self->v_alloc.
     */
    else if (hdsa_allocator_virtual_init_static(&temp_alloc, new_reserved_size, self->buffer_capacity + excess, error_out))
    {
        new_buffer = (unsigned char*)hdsa_allocator_virtual_buffer(&temp_alloc);
        new_buffer = (unsigned char*)hdsa_common_align_forward((ptrdiff_t)new_buffer, self->type_alignment);
        memcpy((void*)new_buffer, (void*)self->buffer, (size_t)real_pos);

        if (hdsa_dynamic_array_last_index((const hdsa_Dynamic_Array* const)self) >= insert_position)
        {
            hdsa_dynamic_array_copy_to_move_up(self, new_buffer, bytes_to_copy, real_pos);
        }

        hdsa_allocator_virtual_give_back_memory(self->v_alloc);
        *self->v_alloc = temp_alloc;
        self->buffer = new_buffer;

        puts("The array buffer was full so a new bigger buffer was allocated, and all the data was copied into it.\n");
    }
    else
    {
        return false;
    }

    self->buffer_capacity += excess;
    self->v_alloc->last_block->used_block += excess;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief Allocates more memory for the Dynamic Array as long as the allocator
 * can get more memory.
 *
 * WARNING: Do not use this function on a Dynamic Array unless init() was
 * already used on it beforehand.
 * @param[in] object_amount Number of T-sized objects you want more memory for.
 * It must be bigger than 1. If it is 0 then the function does nothing.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeded, false otherwise.
 */
bool hdsa_dynamic_array_allocate(hdsa_Dynamic_Array* const self, ptrdiff_t object_amount, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((object_amount > -1), "object_amount is less than 0");
    HDSA_BASIC_ASSERT((self->v_alloc), "The Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The Allocator Virtual mode isn't Static.\n");

    if (object_amount == 0)
    {
        return true;
    }

    ptrdiff_t real_new_size = object_amount * self->type_size;

    if (!self->buffer || !self->v_alloc->last_block)
    {
        if (!hdsa_dynamic_array_init(self, self->v_alloc, object_amount, self->type_size, self->type_alignment, error_out))
        {
            return false;
        }

        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    if (!hdsa_dynamic_array_grow(self, real_new_size, 0, (hdsa_dynamic_array_last_index(self) + 1), error_out))
    {
        return false;
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It deallocates all the memory in the allocator that the
 * Dynamic Array has and sets all the array's data to 0/null.
 * @param[in] self The dynamic array pointer, must not be null.
 */
void hdsa_dynamic_array_deallocate_all(hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc), "The Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The Allocator Virtual mode isn't Static.\n");

    hdsa_allocator_virtual_give_back_memory(self->v_alloc);

    self->buffer_capacity = 0ll;
    self->buffer_size = 0ll;
    self->buffer = (unsigned char*)0;
}

/**
 * @brief It reduces the buffer_capacity and even the amount of memory
 * allocated (if it results in less virtual memory pages) to fit the total
 * amount of T-size objects request by the user.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] object_amount The new smaller number of T-size objects the array
 * is allowed to have. If it is equal or bigger than self->buffer_capacity this
 * function does nothing. If it is smaller than self->buffer_size then
 * object_amount will be the new self->buffer_size.
 *
 * If after being rounded up to hdsa_virtual_page_size() it results in a
 * numbers of committed pages smaller than what the array currently holds, a
 * new buffer with this new number of pages is allocated, all the data inside
 * the current buffer is copied to the new one, the current one is deallocated
 * and the new one becomes the current buffer.
 *
 * This new buffer will hold the same amount of reserved memory as the
 * previous one.
 *
 * If it's 0 then all the memory inside the Dynamic Array is deallocated.
 * @returns False if an allocation fails, true otherwise.
 */
bool hdsa_dynamic_array_shrink(hdsa_Dynamic_Array* const self, ptrdiff_t object_amount, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((object_amount > -1), "The Dynamic Array can't be resized to a negative number.\n");

    ptrdiff_t smaller_size = object_amount * self->type_size;

    if (smaller_size >= self->buffer_capacity)
    {
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    if (object_amount == 0)
    {
        hdsa_dynamic_array_deallocate_all(self);
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    ptrdiff_t commited_size = hdsa_common_round_to_multiple(smaller_size, hdsa_virtual_page_size());
    ptrdiff_t size_to_copy = (smaller_size < self->buffer_size) ? smaller_size : self->buffer_size;
    ptrdiff_t header_difference = (ptrdiff_t)self->buffer - (ptrdiff_t)self->v_alloc->last_block;

    if (commited_size < self->v_alloc->committed_total)
    {
        hdsa_Allocator_Virtual temp_alloc;
        commited_size = ((commited_size - header_difference) > 0) ? commited_size - header_difference : 1;
        ptrdiff_t reserved_size = ((self->v_alloc->reserved_total - header_difference) > 0) ? self->v_alloc->reserved_total - header_difference : 1;

        if (!hdsa_allocator_virtual_init_static(&temp_alloc, reserved_size, commited_size, error_out))
        {
            return false;
        }

        unsigned char* new_buffer = (unsigned char*)hdsa_allocator_virtual_buffer(&temp_alloc);
        new_buffer = (unsigned char*)hdsa_common_align_forward((ptrdiff_t)new_buffer, self->type_alignment);
        memcpy((void*)new_buffer, (void*)self->buffer, (size_t)size_to_copy);
        hdsa_allocator_virtual_give_back_memory(self->v_alloc);

        *self->v_alloc = temp_alloc;
        self->buffer = new_buffer;
    }

    self->buffer_size = size_to_copy;
    self->buffer_capacity = smaller_size;
    self->v_alloc->last_block->used_block = self->buffer_capacity + ((ptrdiff_t)self->buffer - (ptrdiff_t)self->v_alloc->last_block);
    error_out->code = HDSA_ERROR_SUCCESS;

    printf("Committed and buffer_capacity sizes after shrinking: %lld, %lld\n\n", self->v_alloc->last_block->committed_block, self->buffer_capacity);

    return true;
}

/**
 * @brief It copies one hdsa_Dynamic_Array into another, including all the data
 * in the buffer, AKA a "deep copy". The only thing not copied is the allocator
 * interface.
 *
 * It may allocate a new buffer for the second hdsa_Dynamic_Array if
 * the current one isn't big enough to copy all the data
 * from the first array.
 *
 * If both arrays are the same this function does nothing.
 * @param[in] self The dynamic array pointer, must not be null. This one
 * holds to data to copied.
 * @param[out] destination_out The second array and the one that will receive
 * the data from self. Must have a buffer_capacity at least as big as
 * self->buffer_size. If destination_out and self are the same the function is
 * a no-op.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds or isn't needed, false if the
 * allocation fails but destination_out->buffer isn't set to null.
 */
bool hdsa_dynamic_array_copy_array(const hdsa_Dynamic_Array* const self, hdsa_Dynamic_Array* const destination_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The first dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc), "The Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The Allocator Virtual mode isn't Static.\n");
    HDSA_BASIC_ASSERT((destination_out), "The second dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((destination_out->v_alloc), "The destination_out Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((destination_out->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The destination_out Allocator Virtual mode isn't Static.\n");

    if ((uintptr_t)self == (uintptr_t)destination_out)
    {
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    if (!destination_out->buffer || (self->buffer_size > destination_out->buffer_capacity))
    {
        /**
         * Making sure the existing buffer in destination_out is deallocated before the allocation
         * to reduce memory leaks, and deallocation functions are supposed to be no-ops in
         * the case the buffer is null.
         *
         * First, there's a check to make sure the addresses of each buffer aren't inside
         * the other buffer.
         */
        if
        (
            !hdsa_common_is_pointer_inside_buffer(destination_out->buffer, self->buffer_capacity, self->buffer) &&
            !hdsa_common_is_pointer_inside_buffer(self->buffer, destination_out->buffer_capacity, destination_out->buffer)
        )
        {
            hdsa_allocator_virtual_give_back_memory(destination_out->v_alloc);

            destination_out->buffer = (unsigned char*)0;
            destination_out->type_size = 0;
            destination_out->type_alignment = 0;
            destination_out->buffer_capacity = 0;
            destination_out->buffer_size = 0;
        }

        unsigned char* new_buffer;

        ptrdiff_t header_difference = (ptrdiff_t)self->buffer - (ptrdiff_t)self->v_alloc->last_block;

        if (!hdsa_allocator_virtual_allocate(destination_out->v_alloc, self->v_alloc->last_block->reserved_block - header_difference, self->v_alloc->last_block->committed_block - header_difference, (void**)&new_buffer, error_out))
        {
            return false;
        }

        destination_out->buffer = new_buffer;
        destination_out->buffer_capacity = self->buffer_capacity;
    }

    destination_out->buffer_size = self->buffer_size;
    destination_out->type_size = self->type_size;
    destination_out->type_alignment = self->type_alignment;

    memcpy((void*)destination_out->buffer, (void*)self->buffer, (size_t)self->buffer_size);
    destination_out->v_alloc->last_block->used_block = self->v_alloc->last_block->used_block;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It copies "data" at the end of the array, if there's space for it.
 *
 * Given that there's no "type safety" in this hdsa_Dynamic_Array, copying the
 * new data must be done a single byte at a time, and incrementing the buffer
 * size at the end of the function must be done with the type's size.
 *
 * Passing data that has a bigger size than self->type_size only copies the
 * bytes up to self->type_size. Also, if the data being pushed has a different
 * alignment than self->type_alignment UB will happen.
 *
 * It may allocate a new buffer at least twice as big if the current one is
 * full.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] object Data to copy at the end of the array.
 * @param[out] index_out The pointer to the index to the just-pushed T sized
 * object (nth - 1 value). Must not be null.
 * @param[out] data_out Output pointer to a ponter that will contain the
 * address to the data that will be added to the array, must not be null.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeded or wasn't needed, false if the
 * allocation failed.
 */
bool hdsa_dynamic_array_push
(
    hdsa_Dynamic_Array* const self,
    const void* const object,
    ptrdiff_t* const index_out,
    void** const data_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((index_out), "The index pointer must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((object), "The object pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc), "The Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The Allocator Virtual mode isn't Static.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->last_block), "The Allocator Virtual last_block pointer is null.\n");

    if ((self->buffer_size + self->type_size) > self->buffer_capacity)
    {
        if (!hdsa_dynamic_array_grow(self, (self->buffer_size + self->type_size) - self->buffer_capacity, 0, hdsa_dynamic_array_last_index(self) + 1, error_out))
        {
            return false;
        }
    }

    ptrdiff_t new_object_offset = self->buffer_size;
    const unsigned char* const data_uc = object;

    hdsa_dynamic_array_copy_objects(self, data_uc, self->type_size, new_object_offset);
    *data_out = (void*)((ptrdiff_t)self->buffer + self->buffer_size);
    *index_out = self->buffer_size / self->type_size;
    self->buffer_size += self->type_size;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It decreases the buffer_size byt the type_size so the latest "block
 * of memory" can be used to push data on top of it again. If
 * self->buffer_size is 0 then this function does nothing.
 * @returns The index to the now last T sized object after the pop operation.
 * It goes from 0 to hdsa_dynamic_array_last_index.
 */
void hdsa_dynamic_array_pop(hdsa_Dynamic_Array* const self)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");

    if (self->buffer_size == 0) { return; }

    self->buffer_size -= self->type_size;
}

/**
 * @brief It inserts an array of "n" T sized objects in the middle of the
 * array, by first copying all the existing data after "position" to the
 * index after "object_array_size" and then copying "object" in the
 * provided position.
 *
 * A very slow operation, don't do it unless you really need to.
 *
 * Chances are a different kind of data structure is better fit
 * for fast insertion of multiple objects than this one.
 *
 * Pass an array of a different type other than T and you could get UB.
 *
 * It may allocate a new and bigger buffer is the current one is full.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] object_array Data array to copy at the offset specified by
 * "position".
 * @param[in] position Index of size T where object goes. It must be from 0 to
 * (hdsa_dynamic_array_last_index + 1).
 * @param[in] object_array_size Size of the data array to copy, must be bigger
 * than 0.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeded or wasn't needed, false if the
 * allocation failed.
 */
bool hdsa_dynamic_array_insert
(
    hdsa_Dynamic_Array* const self,
    const void* const object_array,
    ptrdiff_t position,
    ptrdiff_t object_array_size,
    void** const data_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((object_array), "The object array pointer is null.\n");
    HDSA_BASIC_ASSERT((object_array_size > 0), "The size of the object array pointer is 0 or negative.\n");
    HDSA_BASIC_ASSERT((self->v_alloc), "The Allocator Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC), "The Allocator Virtual mode isn't Static.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT((self->v_alloc->last_block), "The Allocator Virtual last_block pointer is null.\n");
    HDSA_BASIC_ASSERT(((position > -1) && (position <= hdsa_dynamic_array_size(self))), "The position is out of bounds.\n");

    ptrdiff_t real_object_array_size = object_array_size * self->type_size;
    ptrdiff_t real_pos = position * self->type_size;
    const unsigned char* const data = (unsigned char*)object_array;

    if ((self->buffer_size + real_object_array_size) > self->buffer_capacity)
    {
        if (!hdsa_dynamic_array_grow(self, (self->buffer_size + real_object_array_size) - self->buffer_capacity, real_object_array_size, position, error_out))
        {
            return false;
        }
    }
    else
    {
        if (hdsa_dynamic_array_last_index((const hdsa_Dynamic_Array* const)self) >= position)
        {
            hdsa_dynamic_array_copy_to_move_up(self, self->buffer, real_object_array_size, real_pos);
        }
    }

    hdsa_dynamic_array_copy_objects(self, data, real_object_array_size, real_pos);
    *data_out = (void*)((ptrdiff_t)self->buffer + real_pos);
    self->buffer_size += real_object_array_size;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It returns a pointer to the nth T object on the index "position",
 * similar to how the "subscript operator" works on static arrays (arr[x], x
 * being "position").
 *
 * Now, this index is about T objects in the array, not the real index in the
 * buffer, so it becomes easier for the user to think about the types being
 * stored rather than raw bytes.
 *
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] position Offset calculated win the size of the T object in mind,
 * it goes from 0 to (n - 1).  Other functions like push, pop and
 * last_index give those indexes to the user. It must be from 0 to
 * hdsa_dynamic_array_last_index.
 *
 * @returns The pointer to the initial address where that offset lives.
 * If the array is empty or the position is out of bounds null is returned.
 */
void* hdsa_dynamic_array_object_at(const hdsa_Dynamic_Array* const self, ptrdiff_t position)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT(((position > -1) && (position < hdsa_dynamic_array_size(self))), "The position is out of bounds.\n");

    if (!self->buffer || (self->buffer_size == 0) || (position < 0) || (position > hdsa_dynamic_array_last_index(self)))
    {
        return (void*)0;
    }

    return (void*)&self->buffer[position * self->type_size];
}

/**
 * @brief It overwrites the data at "position" with "object", but "position"
 * must be an existing position in the array already, as this function doesn't
 * increase the size of the array.
 *
 * If the array, its buffer or "object" are null this function does nothing.
 * The same applies if "position" is out of bounds.
 * @param[in] self Pointer to the Dynamic array, must not be null.
 * @param[in] object Data to copy.
 * @param[in] position T-size position where "object" will overwrite the
 * existing data. Must be from 0 to the last index in the array.
 */
void hdsa_dynamic_array_assign(hdsa_Dynamic_Array* const self, void* object, ptrdiff_t position)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT((object), "The pointer to the data to copy into the array is null.\n");
    HDSA_BASIC_ASSERT(((position > -1) && (position < hdsa_dynamic_array_size(self))), "The position is out of bounds.\n");

    if
    (
        !self ||
        !self->buffer ||
        !object ||
        (position < 0)||
        (position > hdsa_dynamic_array_last_index(self))
    )
    {
        return;
    }

    unsigned char* data = (unsigned char*)object;
    ptrdiff_t real_pos = position * self->type_size;
    hdsa_dynamic_array_copy_objects(self, data, self->type_size, real_pos);
}

/**
 * @brief It overwrites the data from "position" with "object", but "position"
 * must be an existing position in the array already. Unlike the "single"
 * variant of this function, "multiple" can expand the Dynamic Array if the
 * incoming other array to assign goes beyond the size of the Dynamic Array.
 *
 * If the array, its buffer or "object" are null this function does nothing.
 * The same applies if "position" is out of bounds or "object_array_size" is
 * less than 1.
 * @param[in] self Pointer to the Dynamic array, must not be null.
 * @param[in] object_array Array of data to copy.
 * @param[in] object_array_size Size of the incoming array to copy in terms of
 * T-size, not bytes. Must be at least 1.
 * @param[in] position The position where "object" will overwrite the existing
 * data. Must be from 0 to the last index in the array.
 */
bool hdsa_dynamic_array_assign_multiple
(
    hdsa_Dynamic_Array* const self,
    void* object_array,
    ptrdiff_t position,
    ptrdiff_t object_array_size,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT((object_array), "The pointer to the data to copy into the array is null.\n");
    HDSA_BASIC_ASSERT((object_array_size > 0), "The size of the data to copy into the array is 0 or negative.\n");
    HDSA_BASIC_ASSERT(((position > -1) && (position < hdsa_dynamic_array_size(self))), "The position is out of bounds.\n");

    if
    (
        !self ||
        !self->buffer ||
        !object_array ||
        (object_array_size <= 1) ||
        (position < 0)||
        (position > hdsa_dynamic_array_last_index(self))
    )
    {
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    unsigned char* data = (unsigned char*)object_array;
    ptrdiff_t real_pos = position * self->type_size;
    ptrdiff_t real_object_array_size = object_array_size * self->type_size;

    if ((real_pos + real_object_array_size) > self->buffer_size)
    {
        if ((real_pos + real_object_array_size) > self->buffer_capacity)
        {
            if (!hdsa_dynamic_array_grow(self, (real_pos + real_object_array_size) - self->buffer_capacity, 0, hdsa_dynamic_array_last_index(self) + 1, error_out))
            {
                return false;
            }
        }

        self->buffer_size += (real_pos + real_object_array_size) - self->buffer_size;
    }

    hdsa_dynamic_array_copy_objects(self, data, real_object_array_size, real_pos);
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It deletes a certain amount of T sized objects in the middle of the
 * array by copying the existing data after "position" n indexes above, with
 * "n" being "object_amount", to the previous indexes by that same distance.
 * If this distance is less than 1 or bigger than the size of the array this
 * function does nothing.
 *
 * After this the last "free spots" (how many of those are determined by
 * object_amount) are filled with 0s.
 *
 * Quite an expensive operation so it's not recommended to use it unless you
 * really need to.
 *
 * If self->buffer_size is 0 this function does nothing as well.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] position The first position in the array for the data to erase,
 * must be within the array size starting from 0.
 * @param[in] object_amount Amount of T sized object to delete, it must be at
 * least 1 and if it goes beyond the size of the array, all the data inside
 * the array will be erased.
 */
void hdsa_dynamic_array_erase(hdsa_Dynamic_Array* const self, ptrdiff_t position, ptrdiff_t object_amount)
{
    HDSA_BASIC_ASSERT((self), "The dynamic array pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The dynamic array buffer is null.\n");
    HDSA_BASIC_ASSERT(((position > -1) && (position < hdsa_dynamic_array_size(self))), "position is out of bounds.\n");
    HDSA_BASIC_ASSERT((object_amount > 0), "object_amount is negative or 0.\n");

    if
    (
        !self ||
        !self->buffer ||
        (self->buffer_size == 0) ||
        (position < 0) ||
        (position > hdsa_dynamic_array_last_index(self))||
        (object_amount < 1)
    )
    {
        return;
    }

    ptrdiff_t real_pos = position * self->type_size;
    object_amount = (object_amount > (hdsa_dynamic_array_size(self) - position)) ? (hdsa_dynamic_array_size(self) - position) : object_amount; // Making sure that object_amount doesn't go above the array's size
    ptrdiff_t real_erase_size = object_amount *self->type_size;

    /**
     * If "end" isn't after the last index in the array this loop
     * copies the existing data to the previous indexes "n" bytes forward,
     * with "n" being "real_erase_size" and starting from real_pos.
     */
    if (hdsa_dynamic_array_last_index((const hdsa_Dynamic_Array* const)self) >= position)
    {
        for (ptrdiff_t i = real_pos; (i + real_erase_size) < self->buffer_size; ++i)
        {
            self->buffer[i] = self->buffer[i + real_erase_size];
        }
    }

    ptrdiff_t last_index = hdsa_dynamic_array_last_index(self);

    hdsa_dynamic_array_zero_fill(self, last_index - object_amount + 1, object_amount);
    self->buffer_size -= real_erase_size;
}
