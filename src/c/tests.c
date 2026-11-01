#include "tests.h"

/**
 * Tests for hdsa_Allocator_Virtual functions
 */

/**
 * @brief Testing the struct members on both the failure and success paths,
 * given that if the users passes a bigger than 0 size, the function will do
 * an allocation.
 */
bool hdsa_test_allocator_virtual_init(hdsa_Allocator_Virtual* const self, ptrdiff_t size, void* initial_address, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT(!error_out, "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT(!self, "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "size is negative.\n");

    ptrdiff_t temp_size = size;

    // Failure path
    if (!hdsa_allocator_virtual_init(self, size, initial_address, error_out))
    {
        if (self->buffer)
        {
            printf("The Virtual_Page_Alloc allocation in the init function failed, but self->buffer isn't null.\n");
            return false;
        }

        if (self->buffer_length != 0)
        {
            printf("The Virtual_Page_Alloc allocation in the init function failed, but self->buffer_length isn't 0.\n");
            return false;
        }

        return true;
    }

    // From here it goes through the success path
    if (!self->buffer)
    {
        if (self->buffer_length != 0)
        {
            printf("The Virtual_Page_Alloc allocation in the init function was skipped, but self->buffer_length isn't 0.\n");
            return false;
        }
    }

    if ((self->buffer_length % hdsa_common_pages_block_size) != 0)
    {
        printf("The Virtual_Page_Alloc allocation in the init function was a success, but self->buffer_length isn't a multiple of hdsa_common_pages_block_size.\n");
        return false;
    }

    return true;
}

bool hdsa_test_allocator_virtual_after_last_address(const hdsa_Allocator_Virtual* const self)
{
    HDSA_BASIC_ASSERT(!self, "The hdsa_Allocator_Virtual pointer is null.\n");

    void* result = hdsa_allocator_virtual_after_last_address(self);

    // Checking the struct values if the result is 0 (the buffer is null)
    if (!result)
    {
        if (self->buffer)
        {
            printf("The Virtual_Page_Alloc after_last_address function returned 0, but the self->buffer isn't null.\n");
            return false;
        }

        if (self->buffer_length != 0)
        {
            printf("The Virtual_Page_Alloc after_last_address function returned 0, but the self->buffer_length isn't 0.\n");
            return false;
        }

        return true;
    }

    if (!self->buffer)
    {
        printf("The Virtual_Page_Alloc after_last_address function returned a positive number, but the self->buffer is null.\n");
        return false;
    }

    if (self->buffer_length == 0)
    {
        printf("The Virtual_Page_Alloc after_last_address function returned a positive number, but the self->buffer_length is 0.\n");
        return false;
    }

    return true;
}

bool hdsa_test_allocator_virtual_allocate(hdsa_Allocator_Virtual* const self, ptrdiff_t size, void* initial_address, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT(!error_out, "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT(!self, "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "size is negative.\n");

    ptrdiff_t temp_size = size;
    unsigned char* temp_buffer;

    if (!hdsa_allocator_virtual_allocate(self, size, initial_address, (void**)temp_buffer, error_out))
    {
        return false;
    }

    // Failure path
    if (!hdsa_allocator_virtual_allocate(self, size, initial_address, (void**)temp_buffer, error_out))
    {
        if (self->buffer)
        {
            printf("The Virtual_Page_Alloc allocation function failed, but self->buffer isn't null.\n");
            return false;
        }

        if (self->buffer_length != 0)
        {
            printf("The Virtual_Page_Alloc allocation function failed, but self->buffer_length isn't 0.\n");
            return false;
        }

        return true;
    }

    // From here it goes through the success path
    if (!self->buffer)
    {
        if (self->buffer_length != 0)
        {
            printf("The Virtual_Page_Alloc allocation function was skipped, but self->buffer_length isn't 0.\n");
            return false;
        }
    }

    if ((self->buffer_length % hdsa_common_pages_block_size) != 0)
    {
        printf("The Virtual_Page_Alloc allocation function was a success, but self->buffer_length isn't a multiple of hdsa_common_pages_block_size.\n");
        return false;
    }

    return true;
}

bool hdsa_test_allocator_virtual_deallocate(hdsa_Allocator_Virtual* const self)
{
    hdsa_allocator_virtual_deallocate(self);

    if (self->buffer)
    {
        printf("The Virtual_Page_Alloc deallocation function ran, but self->buffer isn't null.\n");
        return false;
    }

    if (self->buffer_length != 0)
    {
        printf("The Virtual_Page_Alloc deallocation function ran, but self->buffer_length isn't 0.\n");
        return false;
    }

    return true;
}

bool hdsa_test_allocator_virtual_reallocate(hdsa_Allocator_Virtual* const self, ptrdiff_t new_size, void** allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "error_out must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The Virtual_Page_Alloc pointer is be null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((self->buffer), "The Virtual_Page_Alloc buffer is be null.\n");
    HDSA_BASIC_ASSERT((new_size > 0), "The size of the old allocation is negative.\n");

    unsigned char* old_buffer = self->buffer;
    ptrdiff_t old_buffer_length = self->buffer_length;

    if (!hdsa_allocator_virtual_resize(self, new_size, allocation_out, error_out))
    {
        if (self->buffer != old_buffer)
        {
            printf("The Virtual_Page_Alloc reallocation function failed, but self->buffer isn't the same as before.\n");
            return false;
        }

        if (self->buffer_length != old_buffer_length)
        {
            printf("The Virtual_Page_Alloc reallocation function failed, but self->buffer_length isn't the same as before.\n");
            return false;
        }

        return true;
    }

    if (self->buffer == old_buffer)
    {
        printf("The Virtual_Page_Alloc reallocation function succeeded, but self->buffer is the same as before.\n");
        return false;
    }

    if (self->buffer_length < hdsa_common_pages_block_size)
    {
        printf("The Virtual_Page_Alloc reallocation function succeeded, but self->buffer_length is smaller than hdsa_common_pages_block_size.\n");
        return false;
    }

    return true;
}
