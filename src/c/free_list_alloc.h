#ifndef HDSA_ALLOCATOR_FREE_LIST_H
#define HDSA_ALLOCATOR_FREE_LIST_H

#include <stdio.h>

#include "common.h"
#include "allocator_interface.h"
#include "virtual_alloc.h"

typedef struct hdsa_Allocator_Free_List_Node hdsa_Allocator_Free_List_Node;

/**
 * @brief This allocator, like most linked lists out there, needs a way to find
 * fitting nodes for the user, and this allocator specifically has 2 functions
 * that follow the "find first" and "find best" placement policies.
 *
 * "Find first" is the fastest of the 2 as it looks for the first node that has
 * enough space for what the user requested, but can create more fragmentation
 * in the process. "Find best" looks for the node that has the smallest size
 * that fits what the user needs, it takes way longer but it tends to cause
 * less fragmentation.
 *
 * Bear in mind: In practice it's better to try both and measure depending on
 * the situation, it's not a 50/50.
 *
 * Use this enum to initialize the allocator with one of the 2 policies.
 */
typedef enum hdsa_Allocator_Placement_Policy
{
    HDSA_ALLOCATOR_FIND_FIRST = 0,
    HDSA_ALLOCATOR_FIND_BEST
} hdsa_Allocator_Placement_Policy;

typedef struct hdsa_Allocator_Free_List
{
    hdsa_Allocator_Virtual v_alloc; // v_alloc.committed_total is used as the buffer length
    hdsa_Allocator_Free_List_Node* head;
    hdsa_Allocator_Placement_Policy policy;
    ptrdiff_t allocation_amount;
    ptrdiff_t used_total;
} hdsa_Allocator_Free_List;

/**
 * @brief Header for the "free nodes".
 *
 * "section_size" is the available space after the header, the space the user
 * can actually use.
 *
 * "next" The next node in the free list.
 */
typedef struct hdsa_Allocator_Free_List_Node
{
    ptrdiff_t section_size; // section_size includes the size of hdsa_Allocator_Free_List_Node
    hdsa_Allocator_Free_List_Node* next;
} hdsa_Allocator_Free_List_Node;

/**
 * @brief Header for the nodes currently used for the user.
 *
 * "section_size" exactly the same as in hdsa_Allocator_Free_List_Node.
 *
 * "padding" is how many bytes are needed from the beginning of the header to
 * align the address that will be delivered to the user. Its minimum size will
 * always be the size of the header.
 */
typedef struct hdsa_Allocator_Free_List_Header
{
    ptrdiff_t section_size;
    ptrdiff_t padding;
} hdsa_Allocator_Free_List_Header;

/**
 * @brief It initializes the Allocator_Free_List with an existing buffer of a
 * certain size. It doesn't use the Memory Block's header nor the
 * Allocator_Virtual_Next's header.
 * @param[in] self Allocator_Free_List pointer, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Free_List. It can be
 * null but make sure to change it later on. If it is null or size is 0, all
 * the allocator's data is set to 0/null.
 * @param[in] size Size of the buffer in bytes, must be at least 0.
 */
void hdsa_allocator_free_list_init_external(hdsa_Allocator_Free_List* const self, void* buffer, ptrdiff_t size, hdsa_Allocator_Placement_Policy policy);

/**
 * @brief It initializes the Allocator_Free_List with sizes for reserved and
 * committed memory for a first virtual memory allocation inside the function.
 *
 * The allocator will only hold one Memory Block at a time so to change the
 * reserved amount, the current block must be deallocated first and then to
 * perform a new allocation with the new reserved amount.
 *
 * The Memory Block will store it's own header and the Allocator_Virtual_Next's
 * header as the first 2 allocations.
 * @param[in] self Allocator_Free_List pointer, must not be null.
 * @param[in] reserved_size Maximum size of the block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_granularity_size.
 *
 * If it's 0 no allocation will be done.
 * @param[in] committed_size Size of the usable block in bytes, must be at
 * least 0. It will be rounded to a multiple of hdsa_virtual_page_size.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
bool hdsa_allocator_free_list_init_static
(
    hdsa_Allocator_Free_List* const self,
    ptrdiff_t reserved_size,
    ptrdiff_t committed_size,
    hdsa_Allocator_Placement_Policy policy,
    hdsa_Error* const error_out
);




#endif // HDSA_ALLOCATOR_FREE_LIST_H
