#include "virtual_alloc.h"

/**
 * @brief OS-dependent allocation page size.
 */
ptrdiff_t hdsa_virtual_page_size(void)
{
    static ptrdiff_t page_size = 0;

#if defined(_WIN32) || defined(_WIN64)

    if (page_size == 0)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        page_size = (ptrdiff_t)sys_info.dwPageSize;
    }

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    if (page_size == 0)
    {
        page_size = (ptrdiff_t)sysconf(_SC_PAGESIZE);
    }

#endif
    return page_size;
}

/**
 * @brief "Granularity size". Given this is always 64 KB, it's a constant value
 * in UNIX-like OSes.
 */
ptrdiff_t hdsa_virtual_granularity_size(void)
{
    static ptrdiff_t granu_size = 0;
#if defined(_WIN32) || defined(_WIN64)

    if (granu_size == 0)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        granu_size = (ptrdiff_t)sys_info.dwAllocationGranularity;
    }

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    if (granu_size == 0)
    {
        granu_size = 65536; // 4096 * 16
    }

#endif
    return granu_size;
}

/**
 * @brief A different init function for when the user wants to provide a
 * buffer, regardless of where it comes from.
 * No allocation will be done in this function.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Arena. It can be null
 * but make sure to change it later on. If it is null both sizes in the
 * allocator are set to 0.
 * @param[in] size Size of the user-provided buffer.
 */
void hdsa_allocator_virtual_init_external(hdsa_Allocator_Virtual* const self, void* buffer, ptrdiff_t size)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "The size for the user-provided buffer is negative.\n");

    self->buffer = (unsigned char*)0;
    self->reserved_total = 0;
    self->committed_total = 0;
    self->last_block = (hdsa_Allocator_Memory_Block*)0;
    self->mode = HDSA_ALLOCATOR_VIRTUAL_EXTERNAL;

    if (buffer && (size > 0))
    {
        self->buffer = buffer;
        self->reserved_total = size;
        self->committed_total = size;
    }
}

/**
 * @brief It initializes the Allocator_Virtual to zeroes on all the data
 * fields and performs an allocation for an initial buffer of the user wants
 * to.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @param[in] reserved_size Size for the reserved memory to get on the first
 * allocation. This will be the maximum amount of memory the user can get from
 * future allocations on the same buffer. It will be rounded up to a multiple
 * of hdsa_virtual_granularity_size and can't be negative.
 *
 * If 0 is passed to the function the allocation will not be done and
 * committed_size will be ignored.
 * @param[in] committed_size Size of the usable memory for the first
 * allocation.
 * It will be rounded up to a multiple of hdsa_virtual_page_size and cannot be
 * negative. If it's 0 only the first page will be committed.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds or if there's no allocation,
 * false if the allocation fails.
 */
bool hdsa_allocator_virtual_init_static(hdsa_Allocator_Virtual* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The initial reserved size to initialize the hdsa_Allocator_Virtual is negative.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The initial reserved size to initialize the hdsa_Allocator_Virtual is negative.\n");

    self->buffer = (unsigned char*)0;
    self->reserved_total = 0;
    self->committed_total = 0;
    self->last_block = (hdsa_Allocator_Memory_Block*)0;
    self->mode = HDSA_ALLOCATOR_VIRTUAL_STATIC;

    void* temp_buffer; // Just a dummy buffer to pass to the allocation function, nothing will be done with it

    if (reserved_size > 0)
    {
        if (!hdsa_allocator_virtual_allocate(self, reserved_size, committed_size, (void**)&temp_buffer, error_out))
        {
            return false;
        }
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It initializes the Allocator_Virtual to zeroes on all the data
 * fields and performs an allocation for an initial memory block. Given that
 * the information of the memory block is store within the buffer itself,
 * new blocks will always have at least one page committed after creation.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @param[in] reserved_size Size for the reserved memory to get on the first
 * allocation. This will be the maximum amount of memory the user can get from
 * future allocations on the same buffer. It will be rounded up to a multiple
 * of hdsa_virtual_granularity_size and can't be negative.
 *
 * If 0 is passed to the function the allocation will not be done and
 * committed_size will be ignored.
 * @param[in] committed_size Size of the usable memory for the first
 * allocation.
 * It will be rounded up to a multiple of hdsa_virtual_page_size and cannot be
 * negative. If 0 is passed no memory will be commited on the allocation.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds or if there's no allocation,
 * false if the allocation fails.
 */
bool hdsa_allocator_virtual_init_dynamic(hdsa_Allocator_Virtual* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The initial reserved size to initialize the hdsa_Allocator_Virtual is negative.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The initial reserved size to initialize the hdsa_Allocator_Virtual is negative.\n");

    self->buffer = (unsigned char*)0;
    self->reserved_total = 0;
    self->committed_total = 0;
    self->last_block = (hdsa_Allocator_Memory_Block*)0;
    self->mode = HDSA_ALLOCATOR_VIRTUAL_DYNAMIC;

    void* temp_buffer; // Just a dummy buffer to pass to the allocation function, nothing will be done with it

    if (reserved_size > 0)
    {
        if (!hdsa_allocator_virtual_allocate(self, reserved_size, committed_size, (void**)&temp_buffer, error_out))
        {
            return false;
        }
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @returns The amount of commited pages in the allocator
 */
ptrdiff_t hdsa_allocator_virtual_commited_pages_amount(const hdsa_Allocator_Virtual* const self)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((self->reserved_total > -1), "reserved_total is smaller than 0.\n");
    HDSA_BASIC_ASSERT((self->committed_total > -1), "committed_total is smaller than 0.\n");

    return self->committed_total / hdsa_virtual_page_size();
}

/**
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @returns The first usable address in the last memory block or self->buffer
 * depending on the mode. It returns null if the integer used for the mode
 * isn't one of the three modes.
 */
void* hdsa_allocator_virtual_buffer(const hdsa_Allocator_Virtual* const self)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_EXTERNAL))
    {
        return (void*)0;
    }

    if ((self->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) || (self->mode == HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        return (void*)((ptrdiff_t)self->last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
    }

    return (void*)self->buffer;
}

/**
 * @brief It returns the size of the buffer in External Mode, and usable
 * committed memory in the last block for the other 2 modes.
 * If buffer/last block is null the function returns 0;
 */
ptrdiff_t hdsa_allocator_virtual_remaining_space(const hdsa_Allocator_Virtual* const self)
{
    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_EXTERNAL))
    {
        return -1;
    }

    if (!self)
    {
        return 0;
    }

    if (self->mode == HDSA_ALLOCATOR_VIRTUAL_EXTERNAL)
    {
        if (!self->buffer)
        {
            return 0;
        }

        return self->committed_total;
    }

    if (!self->last_block)
    {
        return 0;
    }

    return (self->last_block->committed_block - (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
}

/**
 * @brief This function reserves virtual memory from the OS. Depending on the
 * Allocator_Virtual's mode the function's behavior changes:
 *
 * 1) External: It returns an error as virtual memory can't be obtained for
 * user-provided buffers.
 *
 * 2) Dynamic: It creates a new Memory Block, allocates the requested reserved
 * size rounded up to a a multiple of hdsa_virtual_granularity_size (64 KB),
 * and commits the first page to be able to store the Memory Block header.
 *
 * 3) Static: Just like Dynamic, but if there's a Memory Block created already
 * the function returns an error.
 * @param[in] self The Allocator_Virtual's struct pointer, must not be null.
 * @param[in] size Size for the memory reservation, must be at least 0.
 * @param[out] allocation_out Pointer to a pointer for the result buffer of
 * the allocation. The pointer it's pointing to will be set to null if the
 * allocation fails at any point.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the reservation (and committing of one page) succeeds,
 * false otherwise.
 */
bool hdsa_allocator_virtual_reserve(hdsa_Allocator_Virtual* const self, ptrdiff_t size, void** const allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "reserve_size is smaller than 0.\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC, __LINE__ - 3, "New memory blocks cannot be created in External mode.");
        *allocation_out = (void*)0;

        return false;
    }

    puts("###############   RESERVATION!   ################\n");

    if (!self->last_block)
    {
        self->buffer = (unsigned char*)0;
        self->reserved_total = 0;
        self->committed_total = 0;
    }

    if ((self->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->last_block))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_RESERVE_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, (self->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->last_block), __LINE__ - 3, "The Allocator_Virtual is in Static mode and it can't reserve memory because it already has some.");
        *allocation_out = (void*)0;

        return false;
    }

    /**
     * Making sure real_reserved_size fulfills its requested size after
     * adding the size of the Memory Block's header. For
     * real_commit_size only the header size is necessary.
     */
    ptrdiff_t real_reserve_size = hdsa_common_round_to_multiple(size + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block), hdsa_virtual_granularity_size());
    ptrdiff_t real_commit_size = hdsa_common_round_to_multiple((ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block), hdsa_virtual_page_size());

    if (real_commit_size > real_reserve_size)
    {
        real_reserve_size = hdsa_common_round_to_multiple(real_commit_size, hdsa_virtual_granularity_size());
    }

    printf("hdsa_Allocator_Virtual real reserve size: %lld\n", real_reserve_size);

    /**
     * From this point until the end of the allocation self->buffer will be used as a
     * temporary buffer.
     * OS selection for the virtual memory allocation function to reserve blocks.
     */
#if defined(_WIN32) || defined(_WIN64)

    self->buffer = (unsigned char*)(VirtualAlloc((void*)0, (size_t)real_reserve_size, MEM_RESERVE, PAGE_READWRITE));

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    self->buffer = (unsigned char*)(mmap((void*)0, (size_t)real_reserve_size, PROT_NONE, MAP_ANONYMOUS | MAP_MAP_PRIVATE, -1, 0));

#endif

    if (!self->buffer)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_RESERVE_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->buffer, __LINE__ - 3, "The virtual memory allocation failed at the point of reserving, allocation_out will be set to null.");
        *allocation_out = (void*)0;

        return false;
    }

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_reserve_size) >= self->reserved_total), "self->reserved_total += real_reserve_size went above the limits of ptrdiff_t.\n");
    self->reserved_total += real_reserve_size;

    // Now to commit the first page to store the Memory Block header
#if defined(_WIN32) || defined(_WIN64)

    self->buffer = (unsigned char*)(VirtualAlloc(self->buffer, (size_t)real_commit_size, MEM_COMMIT, PAGE_READWRITE));

    if (!self->buffer)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, VirtualAlloc failed to commit one or more pages, __LINE__ - 6, "The virtual memory allocation failed at the point of committing, allocation_out will be set to null.");
        *allocation_out = self->buffer;

        return false;
    }

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    if (mprotect(self->buffer, (size_t)real_commit_size, PROT_READ | PROT_WRITE) == -1)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, mprotect failed to open one or more pages for reading and writing, __LINE__ - 3, "The virtual memory allocation failed at the point of committing, allocation_out will be set to null.");
        *allocation_out = self->buffer;

        return false;
    }

#endif

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->committed_total), "self->committed_total += real_commit_size went above the limits of ptrdiff_t.\n");
    self->committed_total += real_commit_size;

    /**
     * new_block->buffer is assigned the address after the header to make it easier to
     * see where the "usable memory" starts.
     */
    hdsa_Allocator_Memory_Block* new_block = (hdsa_Allocator_Memory_Block*)self->buffer;
    new_block->reserved_block = real_reserve_size;
    new_block->committed_block = real_commit_size;
    new_block->previous_block = self->last_block;
    new_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->last_block = new_block;

    /**
     * Now that the allocation succeeded self->buffer's work as a temporary buffer
     * isn't needed anymore.
     */
    self->buffer = (unsigned char*)0;

    *allocation_out = (void*)((ptrdiff_t)new_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
    error_out->code = HDSA_ERROR_SUCCESS;

    printf("hdsa_Allocator_Virtual new memory block's reserved size: %lld\n", new_block->reserved_block);
    printf("hdsa_Allocator_Virtual new memory block's commited size: %lld\n\n", new_block->committed_block);

    return true;
}

/**
 * @brief This function commits virtual memory from the OS on an address that
 * already has reserved memory. Depending on the Allocator_Virtual's mode the
 * function's behavior changes:
 *
 * 1) External: It returns an error as virtual memory can't be obtained for
 * user-provided buffers.
 *
 * 2) Dynamic: It commits memory according to the requested size rounded up to
 * a multiple of hdsa_virtual_page_size (4 KB) in the last existing memory
 * block. Committing without a single memory block isn't allowed. If there's
 * not enough space for committing the requested size an error is returned.
 *
 * 3) Static: Exactly the same as Dynamic.
 * @param[in] self The Allocator_Virtual's struct pointer, must not be null.
 * @param[in] size Size for the memory committing, must be at least 0, but
 * if it's 0 the function returns an error.
 * @param[out] allocation_out Pointer to a pointer for the result buffer of
 * the allocation. The pointer it's pointing to will be set to null if the
 * allocation fails at any point.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the committing succeeds, false otherwise.
 */
bool hdsa_allocator_virtual_commit(hdsa_Allocator_Virtual* const self, ptrdiff_t size, void** const allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "reserve_size is smaller than 1.\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC, __LINE__ - 3, "New pages cannot be committed in External mode.");
        *allocation_out = (void*)0;

        return false;
    }

    if (size == 0)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_SIZE_ZERO;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, real_commit_size == 0, __LINE__ - 3, "The user passed 0 as a size for an allocation inside an existing memory block.");
        *allocation_out = (void*)0;

        return false;
    }

    puts("###############   COMMITTING!   ################\n");

    HDSA_BASIC_ASSERT((self->last_block), "Trying to commit where there's not a single memory block in the allocator (Dynamic Mode) isn't possible.\n");
    HDSA_BASIC_ASSERT((self->last_block->reserved_block >= hdsa_virtual_granularity_size()), "Trying to commit (Dynamic Mode) when there's no reserved memory isn't possible.\n");

    ptrdiff_t real_commit_size = hdsa_common_round_to_multiple(size, hdsa_virtual_page_size());

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->last_block->committed_block), "real_commit_size + self->last_block->committed_block went above the limits of ptrdiff_t.\n");

    if ((real_commit_size + self->last_block->committed_block) > self->last_block->reserved_block)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, (real_commit_size + self->last_block->committed_block) > self->last_block->reserved_block, __LINE__ - 3, "The Allocator_Virtual doesn't have enough space to commit the requested size.");
        *allocation_out = (void*)0;

        return false;
    }

    /**
     * Given that the actual starting point of the buffer is the same block's pointer,
     * it is used for calculating the next committing address instead of the block's
     * "buffer".
     * Now to commit the requested size in pages since the address right after the last
     * one.
     */
    void* next_address = (void*)((ptrdiff_t)self->last_block + self->last_block->committed_block);

#if defined(_WIN32) || defined(_WIN64)

    next_address = VirtualAlloc(next_address, (size_t)real_commit_size, MEM_COMMIT, PAGE_READWRITE);

    if (!next_address)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, VirtualAlloc failed to commit one or more pages, __LINE__ - 5, "The virtual memory allocation failed at the point of reserving, allocation_out will be set to null.");
        *allocation_out = (void*)0;

        return false;
    }

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    if (mprotect(next_address, (size_t)real_commit_size, PROT_READ | PROT_WRITE) == -1)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, mprotect failed to open one or more pages for reading and writing, __LINE__ - 3, "The virtual memory allocation failed at the point of committing, allocation_out will be set to null.");
        *allocation_out = (void*)0;

        return false;
    }

#endif

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->last_block->committed_block), "self->last_block->committed_block += real_commit_size went above the limits of ptrdiff_t.\n");
    self->last_block->committed_block += real_commit_size;

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->committed_total), "self->committed_total += real_commit_size went above the limits of ptrdiff_t.\n");
    self->committed_total += real_commit_size;
    *allocation_out = next_address;
    error_out->code = HDSA_ERROR_SUCCESS;

    printf("hdsa_Allocator_Virtual current memory block's reserved size: %lld\n", self->last_block->reserved_block);
    printf("hdsa_Allocator_Virtual current memory block's commited size: %lld\n\n", self->last_block->committed_block);

    return true;
}

/**
 * @brief It calls the OS virtual memory allocation function, followed by the
 * creation or modification of a "Memory Block". This function is used for both
 * Dynamic and Static mode, External mode returns an error as it can't get
 * allocate virtual memory from a user-provided buffer.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @param[in] reserved_size Amount of bytes to reserve the from OS so they can
 * be "committed" later on (actual usable memory), when there are no Memory
 * Blocks or when committed_size (rounded up to hdsa_virtual_page_size) doesn't
 * fit in the current block's reserved memory. If there's no need to create a
 * new block reserved_size will be ignored.
 * Right after reserving memory, one or more pages will be committed based on
 * committed_size rounded up to hdsa_virtual_page_size to be able to store the
 * Memory Block info as a header and the requested commit size.
 * Valid values from 0 and it will be rounded to the next multiple of
 * hdsa_virtual_granularity_size.
 * @param[in] committed_size Bytes to allocate requested by the user starting
 * from 0. If it is 0 the function will return 0 and a null pointers. It
 * will be rounded to the next multiple of hdsa_virtual_page_size.
 * @param[out] allocation_out Output parameter for the result of the
 * allocation. If the allocation fails at any point of the function the pointer
 * that allocation_out points to will be set to null.
 * @param[out] error_out Parameter for error reporting, must not be null.
 * @returns True if both reservation and commiting succeed, false otherwise.
 */
bool hdsa_allocator_virtual_allocate(hdsa_Allocator_Virtual* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, void** const allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC, __LINE__ - 3, "New memory blocks cannot be created in External mode.");
        *allocation_out = (void*)0;

        return false;
    }

    HDSA_BASIC_ASSERT((reserved_size > -1), "reserve_size is smaller than 0.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "reserve_size is smaller than 0.\n");

    puts("###############   ALLOCATION!   ################\n");

    if (!self->last_block)
    {
        self->buffer = (unsigned char*)0;
        self->reserved_total = 0;
        self->committed_total = 0;
    }

    ptrdiff_t real_commit_size = hdsa_common_round_to_multiple(committed_size, hdsa_virtual_page_size());

    if (self->last_block)
    {
        HDSA_BASIC_ASSERT((self->last_block && ((PTRDIFF_MAX - real_commit_size) >= self->last_block->committed_block)), "real_commit_size + self->last_block->committed_block went above the limits of ptrdiff_t.\n");
    }

    // When there's not a single memory block created or a new one must be added
    if ((!self->last_block) || ((self->mode == HDSA_ALLOCATOR_VIRTUAL_DYNAMIC) && ((real_commit_size + self->last_block->committed_block) > self->last_block->reserved_block)))
    {
        /**
         * Making sure both reserved and committed sizes fulfill their
         * requested sizes after adding the size of the Memory Block's header.
         */
        real_commit_size = hdsa_common_round_to_multiple(committed_size + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block), hdsa_virtual_page_size());
        ptrdiff_t real_reserve_size = hdsa_common_round_to_multiple(reserved_size + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block), hdsa_virtual_granularity_size());

        if (real_reserve_size < real_commit_size)
        {
            real_reserve_size = hdsa_common_round_to_multiple(real_commit_size, hdsa_virtual_granularity_size());
        }

        printf("hdsa_Allocator_Virtual real reserve size: %lld\n", real_reserve_size);

        // OS selection for the virtual memory allocation function to reserve blocks.
    #if defined(_WIN32) || defined(_WIN64)

        self->buffer = (unsigned char*)(VirtualAlloc((void*)0, (size_t)real_reserve_size, MEM_RESERVE, PAGE_READWRITE));

    #elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

        self->buffer = (unsigned char*)(mmap((void*)0, (size_t)real_reserve_size, PROT_NONE, MAP_ANONYMOUS | MAP_MAP_PRIVATE, -1, 0));

    #endif

        if (!self->buffer)
        {
            error_out->code = HDSA_ERROR_ALLOCATION_RESERVE_FAILED;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->buffer, __LINE__ - 3, "The virtual memory allocation failed at the point of reserving, allocation_out will be set to null.");
            *allocation_out = (void*)0;

            return false;
        }

        HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_reserve_size) >= self->reserved_total), "self->reserved_total += real_reserve_size went above the limits of ptrdiff_t.\n");
        self->reserved_total += real_reserve_size;

        // Now to commit the requested size in pages
    #if defined(_WIN32) || defined(_WIN64)

        self->buffer = (unsigned char*)(VirtualAlloc(self->buffer, (size_t)real_commit_size, MEM_COMMIT, PAGE_READWRITE));

        if (!self->buffer)
        {
            error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, VirtualAlloc failed to commit one or more pages, __LINE__ - 6, "The virtual memory allocation failed at the point of committing, allocation_out will be set to null.");
            *allocation_out = self->buffer;

            return false;
        }

    #elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

        if (mprotect(self->buffer, (size_t)real_commit_size, PROT_READ | PROT_WRITE) == -1)
        {
            error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, mprotect failed to open one or more pages for reading and writing, __LINE__ - 3, "The virtual memory allocation failed at the point of committing, allocation_out will be set to null.");
            *allocation_out = self->buffer;

            return false;
        }

    #endif

        HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->committed_total), "self->committed_total += real_commit_size went above the limits of ptrdiff_t.\n");
        self->committed_total += real_commit_size;

        /**
         * new_block->buffer is assigned the address after the header to make it easier to
         * see where the "usable memory" starts.
         */
        hdsa_Allocator_Memory_Block* new_block = (hdsa_Allocator_Memory_Block*)self->buffer;
        new_block->reserved_block = real_reserve_size;
        new_block->committed_block = real_commit_size;
        new_block->previous_block = self->last_block;
        new_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
        self->last_block = new_block;

        /**
         * Now that the allocation succeeded self->buffer's work as a temporary buffer
         * isn't needed anymore.
         */
        self->buffer = (unsigned char*)0;

        *allocation_out = (void*)((ptrdiff_t)new_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
        error_out->code = HDSA_ERROR_SUCCESS;

        printf("hdsa_Allocator_Virtual new memory block's reserved size: %lld\n", new_block->reserved_block);
        printf("hdsa_Allocator_Virtual new memory block's commited size: %lld\n\n", new_block->committed_block);

        return true;
    }

    if (real_commit_size == 0)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_SIZE_ZERO;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, real_commit_size == 0, __LINE__ - 3, "The user passed 0 as a size for an allocation inside an existing memory block.");
        *allocation_out = (void*)0;

        return false;
    }

    if ((self->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) && ((real_commit_size + self->last_block->committed_block) > self->last_block->reserved_block))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, (self->mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) && ((real_commit_size + self->last_block->committed_block) > self->last_block->reserved_block), __LINE__ - 3, "The Allocator_Virtual is in Static mode and it doesn't have enough space to commit the requested size.");
        *allocation_out = (void*)0;

        return false;
    }

    /**
     * Given that the actual starting point of the buffer is the same block's pointer,
     * it is used for calculating the next committing address instead of the block's
     * "buffer".
     * Now to commit the requested size in pages since the address right after the last
     * one.
     */
    void* next_address = (void*)((ptrdiff_t)self->last_block + self->last_block->committed_block);

#if defined(_WIN32) || defined(_WIN64)

    next_address = VirtualAlloc(next_address, (size_t)real_commit_size, MEM_COMMIT, PAGE_READWRITE);

    if (!next_address)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, VirtualAlloc failed to commit one or more pages, __LINE__ - 5, "The virtual memory allocation failed at the point of reserving, allocation_out will be set to null.");
        *allocation_out = (void*)0;

        return false;
    }

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    if (mprotect(next_address, (size_t)real_commit_size, PROT_READ | PROT_WRITE) == -1)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_COMMIT_FAILED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, mprotect failed to open one or more pages for reading and writing, __LINE__ - 3, "The virtual memory allocation failed at the point of committing, allocation_out will be set to null.");
        *allocation_out = (void*)0;

        return false;
    }

#endif

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->last_block->committed_block), "self->last_block->committed_block += real_commit_size went above the limits of ptrdiff_t.\n");
    self->last_block->committed_block += real_commit_size;

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - real_commit_size) >= self->committed_total), "self->committed_total += real_commit_size went above the limits of ptrdiff_t.\n");
    self->committed_total += real_commit_size;
    *allocation_out = next_address;
    error_out->code = HDSA_ERROR_SUCCESS;

    printf("hdsa_Allocator_Virtual current memory block's reserved size: %lld\n", self->last_block->reserved_block);
    printf("hdsa_Allocator_Virtual current memory block's commited size: %lld\n\n", self->last_block->committed_block);

    return true;
}

/**
 * @brief If the Allocator_Virtual is in Dynamic or Static mode this function
 * will deallocate the last memory block and subtract its reserved and commited
 * sizes from self->reserved_total and self->committed_total. In External mode
 * it's a no-op.
 *
 * If there are no memory blocks in the Allocator_Virtual this function does
 * nothing.
 * @param[in] self Pointer to the Allocator_Virtual's struct, must not be
 * null.
 */
void hdsa_allocator_virtual_deallocate(hdsa_Allocator_Virtual* const self)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        self->last_block = (hdsa_Allocator_Memory_Block*)0;

        return;
    }

    if (!self->last_block)
    {
        self->buffer = (unsigned char*)0;
        self->reserved_total = 0;
        self->committed_total = 0;

        return;
    }

    printf("----------------   VIRTUAL DEALLOCATION!   ------------------\n");

    hdsa_Allocator_Memory_Block* temp = (hdsa_Allocator_Memory_Block*)0;

    if (self->last_block->previous_block)
    {
        temp = self->last_block->previous_block;
    }

    self->committed_total -= self->last_block->committed_block;
    self->reserved_total -= self->last_block->reserved_block;

#if defined(_WIN32) || defined(_WIN64)

    VirtualFree((void*)self->last_block, 0, MEM_RELEASE);

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

    munmap((void*)self->last_block, self->last_block->reserved_block);

#endif

    self->last_block = temp;
}

/**
 * @brief If the Allocator_Virtual is in Dynamic mode this function will
 * deallocate all the memory in every block except the first one.
 * self->reserved_total and self->committed_total will have the same sizes as
 * in the only remaining block.
 *
 * If there are no memory blocks in the Allocator_Virtual the other data
 * members are reset to 0/null in Static and Dynamic modes.
 *
 * In External mode last_block is set to 0 just to be sure.
 * @param[in] self Pointer to the Allocator_Virtual's struct, must not be
 * null.
 */
void hdsa_allocator_virtual_reset(hdsa_Allocator_Virtual* const self)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        self->last_block = (hdsa_Allocator_Memory_Block*)0;

        return;
    }

    if (!self->last_block)
    {
        self->buffer = (unsigned char*)0;
        self->reserved_total = 0;
        self->committed_total = 0;

        return;
    }

    hdsa_Allocator_Memory_Block* temp = (hdsa_Allocator_Memory_Block*)0;

    while (self->last_block->previous_block)
    {
        temp = self->last_block->previous_block;
        self->committed_total -= self->last_block->committed_block;
        self->reserved_total -= self->last_block->reserved_block;

    #if defined(_WIN32) || defined(_WIN64)

        VirtualFree((void*)self->last_block, 0, MEM_RELEASE);

    #elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

        munmap((void*)self->last_block, self->last_block->reserved_block);

    #endif

        self->last_block = temp;
    }
}

/**
 * @brief It deallocates every Memory Block in Dynamic and Static mode and all
 * the allocator data is set to 0/null.
 *
 * In External mode the allocator's data will simply be set to 0/null.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 */
void hdsa_allocator_virtual_give_back_memory(hdsa_Allocator_Virtual* const self)
{
    HDSA_BASIC_ASSERT((self), "The hdsa_Allocator_Virtual pointer is null.\n");
    puts("&&&&&&&&&&&&&&&&   DEALLOCATION!   &&&&&&&&&&&&&&&&&&\n");

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_EXTERNAL))
    {
        return;
    }

    hdsa_allocator_virtual_reset(self); // Deallocating all blocks but the first one
    hdsa_allocator_virtual_deallocate(self); // And finally deallocating the first block

    self->buffer = (unsigned char*)0;
    self->reserved_total = 0;
    self->committed_total = 0;
    self->last_block = (hdsa_Allocator_Memory_Block*)0;

    printf("buffer after: %p\n", (void*)self->buffer);
    printf("reserved_total after: %lld\n", self->reserved_total);
}

/**
 * @brief For Dynamic and Static modes it will allocate a new buffer and create
 * a new block inside of a temporary new allocator, copy the data from the
 * existing self->last_block, deallocate self->last_block and make the
 * temporary block the new self->last_block.
 *
 * For External mode it just returns an error.
 *
 * This operation can be really expensive so think twice before using it.
 *
 * If the allocation inside of it fails, the allocator data is left untouched.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 *
 * If self->last_block is null only the allocation part of the function will
 * be done.
 * @param[in] new_size New size for the allocation, no negative values allowed.
 * If it is 0 the function will deallocate the last memory block in Dynamic
 * and Static modes, and set the pointer allocation_out is pointing to null.
 *
 * If after being rounded up to page size it's the same as the commited size
 * in the last block then nothing is done and true is returned, and
 * allocation_out will be set to the first usable address of the last block.
 * @param[out] allocation_out Output parameter for the allocation result, must
 * not be null. It will contain the address to a new allocation with the new
 * size (minus the size of the Memory Block header) and the same data as the
 * old_memory.
 *
 * In the case that the new allocation fails the pointer that allocation_out
 * is pointing too will be set to null.
 * @param[out] error_out Output parameter for error reporting, must not be
 * null.
 * @returns True if resizing succeeds in any of its variants, false otherwise.
 */
bool hdsa_allocator_virtual_resize(hdsa_Allocator_Virtual* const self, ptrdiff_t new_size, void** const allocation_out, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((error_out), "error_out must not be null.\n");
    HDSA_BASIC_ASSERT((self), "The Allocator_Virtual pointer is be null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((self->reserved_total > -1), "The Allocator_Virtual reserved_total is negative.\n");
    HDSA_BASIC_ASSERT((new_size > -1), "The size of the old allocation is negative.\n");

    puts("===================   REALLOCATION!   ==================\n");

    ptrdiff_t real_committed_size = hdsa_common_round_to_multiple(new_size + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block), hdsa_virtual_page_size());

    if ((self->mode != HDSA_ALLOCATOR_VIRTUAL_STATIC) && (self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_WRONG_MODE;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, self->mode != HDSA_ALLOCATOR_VIRTUAL_DYNAMIC, __LINE__ - 3, "New memory blocks cannot be created in External mode.");
        *allocation_out = (void*)0;

        return false;
    }

    // new_size being 0 means only deallocation of the last block
    if (new_size == 0)
    {
        hdsa_allocator_virtual_deallocate(self);
        *allocation_out = (void*)0;
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    // If the last block is null only do a new allocation
    if (!self->last_block)
    {
        if (!hdsa_allocator_virtual_allocate(self, new_size, new_size, allocation_out, error_out))
        {
            return false;
        }

        return true;
    }

    /**
     * If new_size rounded up to hdsa_virtual_page_size is the same as
     * self->last_block->committed_block then allocation_out will point to the
     * first usable address of that block.
     */
    if (self->last_block->committed_block == real_committed_size)
    {
        error_out->code = HDSA_ERROR_SUCCESS;
        *allocation_out = (void*)((ptrdiff_t)self->last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
        puts("Both the old and new sizes inside the Allocator_Virtual are the same, nothing will be done.\n");

        return true;
    }

    hdsa_Allocator_Virtual temp;

    temp.mode = self->mode;
    temp.last_block = (hdsa_Allocator_Memory_Block*)0;

    // Doing a new allocation on a temporary allocator with the new committed size and the old reserved one
    if (!hdsa_allocator_virtual_allocate(&temp, self->last_block->reserved_block - (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block), new_size, allocation_out, error_out))
    {
        return false;
    }

    // Choose the smallest of the 2 sizes and copy the data from old_memory to new_memory
    ptrdiff_t copy_size = (self->last_block->committed_block < real_committed_size) ? self->last_block->committed_block : real_committed_size;
    copy_size -= (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);

    /**
     * To copy the data the buffers must leave the headers untouched, so the
     * addresses after the headers are used instead of the beginning of the
     * blocks.
     */
    void* old_buffer = (void*)((ptrdiff_t)self->last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
    void* new_buffer = (void*)((ptrdiff_t)temp.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));

    /**
     * memmove is more safe to use when the buffers overlap by copying the data
     * to another temporal buffer and then copying it into the destination buffer,
     * but this isn't one of those cases as both buffers as different.
     */
    memcpy(new_buffer, old_buffer, (size_t)copy_size);
    hdsa_allocator_virtual_deallocate(self);

    temp.last_block->previous_block = self->last_block;
    self->last_block = temp.last_block;

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - self->last_block->reserved_block) >= self->reserved_total), "self->reserved_total += self->last_block->reserved_block went above the limits of ptrdiff_t.\n");
    self->reserved_total += self->last_block->reserved_block;

    HDSA_BASIC_ASSERT(((PTRDIFF_MAX - self->last_block->committed_block) >= self->committed_total), "self->committed_total += self->last_block->committed_block went above the limits of ptrdiff_t.\n");
    self->committed_total += self->last_block->committed_block;

    *allocation_out = (void*)((ptrdiff_t)self->last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief Generic wrapper function for the Allocator_Interface. It has all the
 * parameters any allocation function could need, but depending on the
 * Allocation_Type value some of the parameters will be ignored.
 */
bool hdsa_allocator_virtual_generic_function
(
    void* const allocator,
    hdsa_Allocator_Behavior alloc_func,
    void** old_allocation,
    ptrdiff_t old_size,
    ptrdiff_t new_size,
    ptrdiff_t new_alignment,
    void** const allocation_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((error_out), "The error struct must not be null.\n");
    HDSA_BASIC_ASSERT((allocator), "The Allocator_Virtual pointer is null.\n");
    HDSA_BASIC_ASSERT((allocation_out), "The allocation_out pointer is null.\n");
    HDSA_BASIC_ASSERT((old_size > -1), "The size for the existing allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_size > -1), "The size for the resized/new allocation can't be negative.\n");
    HDSA_BASIC_ASSERT((new_alignment > 0), "The new alignment can't be negative nor 0.\n");

    switch(alloc_func)
    {
        case HDSA_ALLOCATION_TYPE_ALLOCATION:
        {
            return hdsa_allocator_virtual_allocate((hdsa_Allocator_Virtual* const)allocator, new_size, new_size, allocation_out, error_out);
        } break;

        case HDSA_ALLOCATION_TYPE_DEALLOCATION:
        {
            hdsa_allocator_virtual_deallocate((hdsa_Allocator_Virtual* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_RESET:
        {
            hdsa_allocator_virtual_reset((hdsa_Allocator_Virtual* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_GIVE_BACK_MEMORY:
        {
            hdsa_allocator_virtual_give_back_memory((hdsa_Allocator_Virtual* const)allocator);
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        } break;

        case HDSA_ALLOCATION_TYPE_RESIZE:
        {
            return hdsa_allocator_virtual_resize((hdsa_Allocator_Virtual* const)allocator, new_size, allocation_out, error_out);
        } break;

        default:
        {
            error_out->code = HDSA_ERROR_NOT_IMPLEMENTED;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, Invalidad allocation type, __LINE__ - 1, "The provided allocation type doesn't exist or is yet to be implemented.");
            *allocation_out = (void*)0;

            return false;
        } break;
    }

    return false;
}

/**
 * @brief It creates a new Allocator_Interface struct from an Allocator_Virtual
 * pointer.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @returns A copy to a new Allocator_Interface struct with self as the
 * "allocator", and hdsa_allocator_virtual_generic_function as the
 * "generic_alloc_function".
 */
hdsa_Allocator_Interface hdsa_allocator_virtual_create_interface(const hdsa_Allocator_Virtual* const self)
{
    return (hdsa_Allocator_Interface){
        (void*)self,
        &hdsa_allocator_virtual_generic_function
    };
}
