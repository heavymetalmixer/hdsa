#ifndef HDSA_ALLOCATOR_VIRTUAL_H
#define HDSA_ALLOCATOR_VIRTUAL_H

// OS libraries
#if defined(_WIN32) || defined(_WIN64)

#include <memoryapi.h>
#include <sysinfoapi.h>

#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)

#include <sys/mman.h>
#include <unistd.h>

#endif // OS libraries

#include "common.h"
#include "allocator_interface.h"

/**
 * @brief OS-dependent allocation page size.
 */
ptrdiff_t hdsa_virtual_page_size(void);

/**
 * @brief "Granularity size". Given this is always 64 KB, it's a constant value
 * in UNIX-like OSes.
 */
ptrdiff_t hdsa_virtual_granularity_size(void);

/**
 * @brief This is an enum to change the behavior of the Allocator_Virtual.
 *
 * 1) Dynamic: It creates a linked list of "Memory Blocks" each one  with its
 * each reserved and commited memory. The data for each Memory block is stored
 * at the beginning of the block as a header.
 * This process can be done as many times as needed, but it's recommended not
 * to use Dynamic Mode unless it's necessary as it can create a lot of
 * memory fragmentation, because allocations of pages are only done in the
 * latest block.
 * It allows the deallocation of the latest block as if it was a Stack, but
 * not deallocation of individual committed pages. Also, it can deallocate all
 * the blocks at once or to leave one in case of a reset operation.
 *
 * 2) Static: It works like Dynamic mode but only with one single memory block.
 *
 * 3) External: A fixed buffer provided by the user. This allocator holds a
 * pointer to the buffer and its size, but it trying to get memory from the OS
 * in this mode results in an error.
 */
typedef enum hdsa_Allocator_Virtual_Mode
{
    HDSA_ALLOCATOR_VIRTUAL_DYNAMIC = 0,
    HDSA_ALLOCATOR_VIRTUAL_STATIC,
    HDSA_ALLOCATOR_VIRTUAL_EXTERNAL
} hdsa_Allocator_Virtual_Mode;

/**
 * @brief This is a Memory Block for the Allocator_Virtual in Static or Dynamic
 * modes.
 *
 */
typedef struct hdsa_Allocator_Memory_Block
{
    ptrdiff_t reserved_block;
    ptrdiff_t committed_block;
    ptrdiff_t used_block;
    struct hdsa_Allocator_Memory_Block* previous_block;
} hdsa_Allocator_Memory_Block;

/**
 * @brief It makes allocations of "virtual memory" directly using the OS API or
 * with a buffer provided by the user, depending on the value in "mode". It
 * works like Allocator_Arena while being able to get memory on its own.
 *
 * Each Allocator_Virtual struct only holds a single private memory allocation,
 * which has 2 types of "memory" called Reserved and Committed, except in
 * "External Mode".
 *
 * Reserved is the maximum amount of memory the buffer can hold but it's not
 * really usable memory in the moment, it means other parts of the program
 * can't access that range of addresses until the allocator gives them back to
 * the OS. To get a bigger reserved space the existing buffer must be
 * deallocated first and then the allocator can ask for another buffer with
 * the new Reserved Length.
 *
 * Reserved allocations are done in chunks of 64 KB (65536 bytes). This size
 * due to a certain particularity on the Windows API that allocates in
 * boundaries determined by the "system granularity" which is 64 KB. Even if
 * you allocate a single page of 4KB in Windows, the next allocation would be
 * done 64 KB apart and that's a lot of wasted memory.
 *
 * To know why Windows allocates in 64 KB granularity visit this link:
 * https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223
 *
 * Commited memory is actual usable memory inside the Reserved one, it's
 * smallest unit is called a "page" and in most OSes it's 4 KB in size.
 *
 * Any arbitrary allocation size passed by the user is rounded up to a multiple
 * of 64 KB for the Reserved memory and a multiple of 4 KB for the Commited
 * memory. this is to preserve the same behavior across Windows, Mac, Linux
 * and other UNIX-like OSes for 32 and 64 bits PCs.
 *
 * After the allocation is done you can give the buffer to other allocators
 * to do more precise allocations without having to make syscalls until they
 * run out of memory.
 *
 * This allocator also has a different "mode" to hold a user-provided buffer
 * instead which doesn't grow, so any calls to allocate more memory result in
 * failure.
 */
typedef struct hdsa_Allocator_Virtual
{
    unsigned char* buffer;
    hdsa_Allocator_Virtual_Mode mode;
    ptrdiff_t reserved_total;
    ptrdiff_t committed_total;
    hdsa_Allocator_Memory_Block* last_block;
} hdsa_Allocator_Virtual;

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
bool hdsa_allocator_virtual_init_static(hdsa_Allocator_Virtual* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out);

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
 * negative. If it's 0 only the first page will be committed.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds or if there's no allocation,
 * false if the allocation fails.
 */
bool hdsa_allocator_virtual_init_dynamic(hdsa_Allocator_Virtual* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, hdsa_Error* const error_out);

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
void hdsa_allocator_virtual_init_external(hdsa_Allocator_Virtual* const self, void* buff, ptrdiff_t size);

/**
 * @returns The amount of commited pages in the allocator
 */
ptrdiff_t hdsa_allocator_virtual_commited_pages_amount(const hdsa_Allocator_Virtual* const self);

/**
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @returns The first usable address in the last memory block or self->buffer
 * depending on the mode. It returns null if the integer used for the mode
 * isn't one of the three modes.
 */
void* hdsa_allocator_virtual_buffer(const hdsa_Allocator_Virtual* const self);

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
bool hdsa_allocator_virtual_reserve(hdsa_Allocator_Virtual* const self, ptrdiff_t size, void** const allocation_out, hdsa_Error* const error_out);

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
bool hdsa_allocator_virtual_commit(hdsa_Allocator_Virtual* const self, ptrdiff_t size, void** const allocation_out, hdsa_Error* const error_out);

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
bool hdsa_allocator_virtual_allocate(hdsa_Allocator_Virtual* const self, ptrdiff_t reserved_size, ptrdiff_t committed_size, void**const allocation_out, hdsa_Error* const error_out);

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
void hdsa_allocator_virtual_deallocate(hdsa_Allocator_Virtual* const self);

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
void hdsa_allocator_virtual_reset(hdsa_Allocator_Virtual* const self);

/**
 * @brief It deallocates every Memory Block in Dynamic and Static mode and all
 * the allocator data is set to 0/null.
 *
 * In External mode the allocator's data will simply be set to 0/null.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 */
void hdsa_allocator_virtual_give_back_memory(hdsa_Allocator_Virtual* const self);

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
bool hdsa_allocator_virtual_resize(hdsa_Allocator_Virtual* const self, ptrdiff_t new_size, void** const allocation_out, hdsa_Error* const error_out);

/**
 * @brief Generic wrapper function for the Allocator_Interface. It has all the
 * parameters any allocation function could need, but depending on the
 * Allocation_Type value some of the parameters will be ignored.
 */
bool hdsa_allocator_virtual_generic_function
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
 * @brief It creates a new Allocator_Interface struct from an Allocator_Virtual
 * pointer.
 * @param[in] self Pointer to the Allocator_Virtual, must not be null.
 * @returns A copy to a new Allocator_Interface struct with self as the
 * "allocator", and hdsa_allocator_virtual_generic_function as the
 * "generic_alloc_function".
 */
hdsa_Allocator_Interface hdsa_allocator_virtual_create_interface(const hdsa_Allocator_Virtual* const self);

#endif // HDSA_ALLOCATOR_VIRTUAL_H
