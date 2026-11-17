#ifndef HDSA_ALLOCATOR_FREE_LIST_H
#define HDSA_ALLOCATOR_FREE_LIST_H

#include <stdio.h>

#include "common.h"
#include "allocator_interface.h"
#include "virtual_alloc.h"

typedef struct hdsa_Allocator_List_Free hdsa_Allocator_List_Free;
typedef struct hdsa_Allocator_List_Used hdsa_Allocator_List_Used;

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

/**
 * @brief List allocator is slower and more memory-hungry than the arena
 * and stack allocators (and it causes more memory fragmentation), but it
 * allows the user to do arbitraty-sized allocations, and also individual
 * deallocations regardless of where each allocation is in the buffer, unlike
 * those 2.
 *
 * This list allocator mostly takes from "free list allocators", a kind of
 * allocator than uses a singly or doubly linked list of "free nodes" (doubly
 * in this case), nodes that aren't being used by the user just yet, but it
 * wasn't named that way because it also manages a singly linked list of "used
 * nodes" to make sure the pointer to deallocate by the user is an actual
 * allocation inside the buffer.
 *
 * The free node list, or just "free list" occupies the whole buffer with a
 * single free node, and as allocations are done free dones are split into 2
 * smaller ones to make subsequent allocations possible (nodes are NOT
 * contiguous in memory most of the time), and when a node is used for an
 * allocation it turns into a node of the "used list", node that starts
 * with padding followed by the usable memory by the user with an aligned
 * address according to the requested alignment (but it's rounded up to the
 * alignment of hdsa_Allocator_List_Footer).
 *
 * A free node has a hdsa_Allocator_List_Free header at the beginning and
 * a hdsa_Allocator_List_Footer at the end. A used has a
 * hdsa_Allocator_List_Used header (most of the time not in the same address
 * as the hdsa_Allocator_List_Free header because of the padding) and a
 * hdsa_Allocator_List_Footer. Allocation sizes are also rounded up to a
 * multiple of the alignment of hdsa_Allocator_List_Footer.
 *
 * It has 2 modes that change the arena's behavior: External and Static.
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
 * Just like the other allocators in this library can also reset to allocate
 * from the beginning without getting rid of all the available memory.
 *
 * "v_alloc" is the internal Allocator_Virtual that performs the virtual memory
 * allocations.
 *
 * "head" Head of the free list of nodes. The Allocator_List is initialized
 * with a single free node for head, and it's next and previous nodes are null.
 * If it's null it means the allocator has no nodes at all, or all the free
 * already turned into used nodes.
 *
 * "used_head" Head of the used list of nodes. the Allocator_List is
 * initialized with used_head being null. If it's null it means all nodes are
 * free nodes or the allocator has no nodes at all.
 *
 * "policy" Placement policy to use when trying to find a suitable free node
 * for the allocation requirements. It doesn't change when the allocator is
 * reset.
 *
 * "allocated_offset" keeps track of how much of the buffer is already in use
 * by both lists, it's necessary to make new free nodes in buffer spaces that
 * has no nodes at all (mostly for Static Mode when committing more memory).
 *
 * "allocation_amount" saves how many allocations the user has requested (plus
 * 1 in Static mode for the block's header), not the virtual committed nor
 * reserved memory. The "allocations" is also the amount of used nodes.
 *
 * "used_total" saves the sum of all the used memory the buffer/memory block,
 * the sum of everything inside used nodes, and the header and footer from free
 * nodes (and the size of the Memory Block for Static mode). In Static mode
 * v_alloc.last_block->used_block has the same number.
 *
 * "v_alloc.committed_total" is used as the v_alloc.buffer's (External mode) or
 * sum of all blocks' "length" or "capacity" (Static). Also, in External mode
 * "v_alloc.reserved_total" contains the same number.
 */
typedef struct hdsa_Allocator_List
{
    hdsa_Allocator_Virtual v_alloc; // v_alloc.committed_total is used as the buffer length
    hdsa_Allocator_List_Free* head;
    hdsa_Allocator_List_Used* used_head;
    hdsa_Allocator_Placement_Policy policy;
    ptrdiff_t allocated_offset;
    ptrdiff_t allocation_amount;
    ptrdiff_t used_total;
} hdsa_Allocator_List;

/**
 * @brief Header for the "free nodes".
 *
 * "section_size" is the available space after the header, the space the user
 * can actually use.
 *
 * "previous" The previous node in the free list. Only "head" has previous as
 * null.
 *
 * "next" The next node in the free list. Only the last node in the free list
 * has next as null.
 */
typedef struct hdsa_Allocator_List_Free
{
    ptrdiff_t section_size; // section_size includes the size of hdsa_Allocator_Free_List_Node
    hdsa_Allocator_List_Free* previous;
    hdsa_Allocator_List_Free* next;
} hdsa_Allocator_List_Free;

/**
 * @brief Header for the "used nodes".
 *
 * "section_size" it's smaller or equal to section_size in the free node.
 *
 * "padding" is how many bytes are needed from the beginning of the header to
 * align the address that will be delivered to the user. Its minimum size will
 * always be the size of the header.
 *
 * "next" The next node in the used list. Only the last node in the used list
 * has next as null.
 */
typedef struct hdsa_Allocator_List_Used
{
    ptrdiff_t section_size;
    ptrdiff_t padding;
    hdsa_Allocator_List_Used* next;
} hdsa_Allocator_List_Used;

/**
 * @brief Set after the section, at the end of the node. Necessary for
 * coalescence/merging of free nodes that are next to each other in memory to
 * reduce memory fragmentation (it has nothing to do with previous and next).
 *
 * "node_offset" Distance the beginning of the node to the footer.
 *
 * "is_free" true if the node is free, false if it's used.
 */
typedef struct hdsa_Allocator_List_Footer
{
    ptrdiff_t node_offset;
    bool is_free;
} hdsa_Allocator_List_Footer;

/**
 * @brief It initializes the Allocator_Free_List with an existing buffer of a
 * certain size.
 * @param[in] self Allocator_Free_List pointer, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Free_List. It can be
 * null but make sure to change it later on. If it is null or size is 0, all
 * the allocator's data is set to 0/null.
 * @param[in] size Size of the buffer in bytes, must be at least 0.
 */
void hdsa_allocator_list_init_external(hdsa_Allocator_List* const self, void* buffer, ptrdiff_t size, hdsa_Allocator_Placement_Policy policy);

/**
 * @brief It initializes the Allocator_Free_List with sizes for reserved and
 * committed memory for a first virtual memory allocation inside the function.
 *
 * The allocator will only hold one Memory Block at a time so to change the
 * reserved amount, the current block must be deallocated first and then to
 * perform a new allocation with the new reserved amount.
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
bool hdsa_allocator_list_init_static
(
    hdsa_Allocator_List* const self,
    ptrdiff_t reserved_size,
    ptrdiff_t committed_size,
    hdsa_Allocator_Placement_Policy policy,
    hdsa_Error* const error_out
);

/**
 * @brief Allocates within a free node according to the Placement_Policy in
 * use.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] size Size in bytes required by the user, must be at least 1.
 * It will be rounded up to a multiple of the alignment of
 * hdsa_Allocator_List_Footer.
 * @param[in] alignment Alignment in bytes required by the user, must be at
 * least 1. It will be rounded up to the alignment of
 * hdsa_Allocator_List_Footer.
 * @param[out] allocation_out Output pointer for the allocation, must not be
 * null.
 * @param[in] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds, false otherwise.
 */
bool hdsa_allocator_list_allocate(hdsa_Allocator_List* const self, ptrdiff_t size, ptrdiff_t alignment, void** const allocation_out, hdsa_Error* const error_out);

/**
 * @brief Deallocates a used node if the user-provided pointer is a used node.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] ptr Pointer to the pointer being used byt the user. Both ptr and
 * *ptr must not be null. If the deallocation succeeds, then *ptr is set to
 * null.
 */
void hdsa_allocator_list_deallocate(hdsa_Allocator_List* const self, void** const ptr);

/**
 * @brief In External mode it resets everything to 0 except
 * self->v_alloc.committed total, the placement policy and the buffer/memory
 * block. Then self->head is set to a node that covers the entire buffer (or
 * the rest of the memory block after its header in Static Mode).
 * @param[in] self Allocator_List pointer, must not be null.
 */
void hdsa_allocator_list_reset(hdsa_Allocator_List* const self);


#endif // HDSA_ALLOCATOR_FREE_LIST_H
