#include "free_list_alloc.h"


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
void hdsa_allocator_free_list_init_external(hdsa_Allocator_Free_List* const self, void* buffer, ptrdiff_t size, hdsa_Allocator_Placement_Policy policy)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "The buffer size to initialize the Allocator_List is less than 1.\n");
    HDSA_BASIC_ASSERT(((policy == HDSA_ALLOCATOR_FIND_BEST) || (policy == HDSA_ALLOCATOR_FIND_BEST)), "Incorrect value for the Placement_Policy.\n");

    hdsa_allocator_virtual_init_external(&self->v_alloc, buffer, size);
    self->allocation_amount = 0;
    self->policy = policy;
    self->used_total = 0;
    self->head = (hdsa_Allocator_Free_List_Node*)0;

    if (buffer)
    {
        self->head = (hdsa_Allocator_Free_List_Node*)buffer;
        self->head->next = (hdsa_Allocator_Free_List_Node*)0;
        self->head->section_size = self->v_alloc.committed_total - (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node);
    }
}

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
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Free_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The reserved_size to initialize the Allocator_Free_List is less than 1.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The buffer size to initialize the Allocator_Free_List is less than 1.\n");

    self->allocation_amount = 0;
    self->policy = policy;
    self->used_total = 0;
    self->head = (hdsa_Allocator_Free_List_Node*)0;

    if (!hdsa_allocator_virtual_init_static(&self->v_alloc, reserved_size, committed_size, error_out))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        self->head = (hdsa_Allocator_Free_List_Node*)hdsa_allocator_virtual_buffer(&self->v_alloc);
        self->head->next = (hdsa_Allocator_Free_List_Node*)0;
        self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node);
        self->head->section_size = self->v_alloc.committed_total - self->v_alloc.last_block->used_block;

        self->used_total += self->v_alloc.last_block->used_block;
        self->allocation_amount += 1;
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It looks for the first free node that can fit the requested size and
 * padding (for alignment).
 * @param[in] self Allocator_Free_List pointer, must not be null.
 * @param[in] size Size required by the user in bytes.
 * @param[in] alignment Alignment required by the user in bytes, it must be
 * a power of 2.
 * @param[out] padding_out Output pointer to a variable for storing the padding
 * value, must not be null.
 * @param[out] previous_node_out Output pointer to a pointer for the previous
 * node to the one this function will look for, must not be null.
 * @param[out] node_out Output pointer to a pointer for the node that will be
 * used for the allocation, must not be null.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
static bool hdsa_allocator_free_list_find_first
(
    const hdsa_Allocator_Free_List* const self,
    ptrdiff_t size,
    ptrdiff_t alignment,
    ptrdiff_t* const padding_out,
    hdsa_Allocator_Free_List_Node** const previous_node_out,
    hdsa_Allocator_Free_List_Node** const node_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Free_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((padding_out), "padding_out is null.\n");
    HDSA_BASIC_ASSERT((previous_node_out), "previous_node_out is null.\n");
    HDSA_BASIC_ASSERT((node_out), "node_out is null.\n");
    HDSA_BASIC_ASSERT((size > 0), "size to find a node is 0 or negative.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "size to find a node is 0 or negative.\n");

    puts(".............................. FREE LIST FIND FIRST ..................................\n");

    hdsa_Allocator_Free_List_Node* node = self->head;
    hdsa_Allocator_Free_List_Node* previous_node = (hdsa_Allocator_Free_List_Node*)0;
    ptrdiff_t padding = 0;

    printf("head and node: %p\n", (void*)node);

    if (!self->head)
    {
        error_out->code = HDSA_ERROR_NULL_POINTER_INTERNAL_DATA;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->head, __LINE__ - 3, "The Allocator_Free_List headis null, a fitting node can't be found.");
        *node_out = (hdsa_Allocator_Free_List_Node*)0;

        return false;
    }

    while (node)
    {
        padding = hdsa_common_calc_padding_with_header((ptrdiff_t)node, (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header), alignment);
        ptrdiff_t required_space = size + padding;

        printf("padding: %lld\n", padding);
        printf("allocation_size + padding: %lld\n", required_space);

        // if (node->section_size >= required_space)
        if ((node->section_size + (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node)) >= required_space)
        {
            printf("Found node: %p\n", (void*)node);
            printf("Found node->section_size: %lld\n", node->section_size);
            break;
        }

        previous_node = node;
        node = node->next;

        printf("previous_node: %p\n", (void*)previous_node);
        printf("node: %p\n", (void*)node);
    }

    if (!node)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_NO_FITTING_NODE_FOUND;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !node, __LINE__ - 3, "A node with the requiered size and alignment couldn't be found.");
        *node_out = (hdsa_Allocator_Free_List_Node*)0;

        return false;
    }

    *padding_out = padding;
    *previous_node_out = previous_node;
    *node_out = node;

    printf("*padding_out: %lld\n", *padding_out);
    printf("*previous_node_out: %p\n", (void*)(*previous_node_out));
    printf("Chosen node: %p\n", (void*)*node_out);

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It looks for first node with the smallest size that fits the
 * requested size and padding. It takes way longer than find_first but it
 * tends to cause less memory fragmentation.
 * @param[in] self Allocator_Free_List pointer, must not be null.
 * @param[in] size Size required by the user in bytes.
 * @param[in] alignment Alignment required by the user in bytes, it must be
 * a power of 2.
 * @param[out] padding_out Output pointer to a variable for storing the padding
 * value, must not be null.
 * @param[out] previous_node_out Output pointer to a pointer for the previous
 * node to the one this function will look for, must not be null.
 * @param[out] node_out Output pointer to a pointer for the node that will be
 * used for the allocation, must not be null.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
static bool hdsa_allocator_free_list_find_best
(
    const hdsa_Allocator_Free_List* const self,
    ptrdiff_t size,
    ptrdiff_t alignment,
    ptrdiff_t* const padding_out,
    hdsa_Allocator_Free_List_Node** const previous_node_out,
    hdsa_Allocator_Free_List_Node** const node_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Free_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((padding_out), "padding_out is null.\n");
    HDSA_BASIC_ASSERT((previous_node_out), "previous_node_out is null.\n");
    HDSA_BASIC_ASSERT((node_out), "node_out is null.\n");
    HDSA_BASIC_ASSERT((size > 0), "size to find a node is 0 or negative.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "size to find a node is 0 or negative.\n");

    puts(".............................. FREE LIST FIND BEST ..................................\n");

    hdsa_Allocator_Free_List_Node* node = self->head;
    hdsa_Allocator_Free_List_Node* previous_node = (hdsa_Allocator_Free_List_Node*)0;
    hdsa_Allocator_Free_List_Node* best_node = (hdsa_Allocator_Free_List_Node*)0;
    ptrdiff_t padding = 0;
    ptrdiff_t smallest_difference = PTRDIFF_MAX;
    ptrdiff_t total_node_size = 0;

    printf("head and node: %p\n", (void*)node);

    if (!self->head)
    {
        error_out->code = HDSA_ERROR_NULL_POINTER_INTERNAL_DATA;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->head, __LINE__ - 3, "The Allocator_Free_List headis null, a fitting node can't be found.");
        *node_out = (hdsa_Allocator_Free_List_Node*)0;

        return false;
    }

    while (node)
    {
        padding = hdsa_common_calc_padding_with_header((ptrdiff_t)node, (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header), alignment);
        ptrdiff_t required_space = size + padding;
        total_node_size = node->section_size + (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node);

        printf("padding: %lld\n", padding);
        printf("allocation_size + padding: %lld\n", required_space);

        // if ((node->section_size >= required_space) && ((node->section_size - required_space) < smallest_difference))
        if ((total_node_size >= required_space) && ((total_node_size - required_space) < smallest_difference))
        {
            best_node = node;
            printf("best_node: %p\n", (void*)best_node);

            // smallest_difference = node->section_size - required_space;
            smallest_difference = total_node_size - required_space;
            printf("smallest_difference: %lld\n", smallest_difference);
        }

        if ((smallest_difference == 0) || !node->next)
        {
            printf("previous_node: %p\n", (void*)previous_node);
            printf("node: %p\n", (void*)node);

            break;
        }

        previous_node = node;
        node = node->next;
        printf("previous_node: %p\n", (void*)previous_node);
        printf("node: %p\n", (void*)node);
    }

    *padding_out = padding;
    *previous_node_out = previous_node;
    *node_out = best_node;

    printf("*padding_out: %lld\n", *padding_out);
    printf("*previous_node_out: %p\n", (void*)(*previous_node_out));
    printf("Chosen node: %p\n", (void*)*node_out);

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

// /**
//  * It adds new_node to the "free list" of nodes by making it the next of head. If new_node is null this function does nothing
//  */
// void hdsa_allocator_free_list_node_insert(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Free_List_Node* const new_node_out)
// {
//     if (!new_node_out)
//     {
//         puts("new_node_out is null, no nodes will be inserted in the list.\n");
//         return;
//     }

//     if (!self->head)
//     {
//         self->head = new_node_out;
//         return;
//     }

//     if (self->head == new_node_out)
//     {
//         puts("The tail and new_node are the same, no new node will be inserted in the free list.\n");
//         return;
//     }

//     new_node_out->next = self->head->next;
//     self->head->next = new_node_out;
// }

// /**
//  * It removes del_node from the "free list" of nodes
//  */
// void hdsa_allocator_free_list_node_remove(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Free_List_Node* previous_node_out, hdsa_Allocator_Free_List_Node* const delete_node)
// {
//     if (!delete_node)
//     {
//         puts("del_node is null, no nodes will be removed from the list.\n");
//         return;
//     }

//     if (!self->head)
//     {
//         puts("head is null, no nodes will be removed from the list.\n");
//         return;
//     }

//     if (!previous_node_out)
//     {
//         self->head = delete_node->next;
//         printf("head: %p\n", (void*)self->head);

//         return;
//     }

//     if (previous_node_out == self->head)
//     {
//         if (previous_node_out == delete_node)
//         {
//             self->head = delete_node->next;
//             printf("head: %p\n", (void*)self->head);
//         }
//         else
//         {
//             self->head->next = delete_node->next;
//             printf("head->next: %p\n", (void*)self->head->next);
//         }
//     }
//     else
//     {
//         if (previous_node_out == delete_node)
//         {
//             previous_node_out = delete_node->next;
//             printf("previous_node_out: %p\n", (void*)previous_node_out);
//         }
//         else
//         {
//             previous_node_out->next = delete_node->next;
//             printf("previous_node_out->next: %p\n", (void*)previous_node_out->next);
//         }
//     }
// }

// /**
//  * Tries to combine free_node and_free_node->next into a single free block
//  * and tries to do the same with previous_node and free_node afterwards
//  */
// void hdsa_allocator_free_list_coalescence(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Free_List_Node* const previous_node_out, hdsa_Allocator_Free_List_Node* const free_node_out)
// {
//     puts("------------------------------ FREE LIST COALESCENCE ----------------------------------\n");

//     if (!previous_node_out || !free_node_out)
//     {
//         puts("One or both nodes are null, coalescence isn't possible.\n");
//         return;
//     }

//     if (free_node_out->next && (((ptrdiff_t)free_node_out + free_node_out->section_size) == (ptrdiff_t)free_node_out->next))
//     {
//         free_node_out->section_size += free_node_out->next->section_size;
//         hdsa_allocator_free_list_node_remove(self, free_node_out, free_node_out->next);
//         puts("Combining free_node_out and free_node_out->next.\n");
//     }

//     if (previous_node_out->next && (((ptrdiff_t)previous_node_out + previous_node_out->section_size) == (ptrdiff_t)free_node_out))
//     {
//         previous_node_out->section_size += free_node_out->section_size;
//         hdsa_allocator_free_list_node_remove(self, previous_node_out, free_node_out);
//         puts("Combining previous_node_out and free_node_out.\n");
//     }

//     if ((self->head->next == previous_node_out) && previous_node_out->next)
//     {
//         if (((ptrdiff_t)previous_node_out + previous_node_out->section_size) == (ptrdiff_t)self->head)
//         {
//             previous_node_out->section_size += self->head->section_size;
//             hdsa_allocator_free_list_node_remove(self, previous_node_out, self->head);
//             puts("Combining head into previous_node_out.\n");
//         }
//         else if (((ptrdiff_t)self->head + self->head->section_size) == (ptrdiff_t)previous_node_out)
//         {
//             self->head->section_size += previous_node_out->section_size;
//             hdsa_allocator_free_list_node_remove(self, self->head, previous_node_out);
//             puts("Combining previous_node_out into head.\n");
//         }
//     }

//     printf("free_node->next isn't null. free_node->block_size: %lld\n", free_node_out->section_size);
//     printf("previous_node_out->next isn't null. previous_node_out->block_size: %lld\n", previous_node_out->section_size);
// }

// /**
//  * Allocates within a node according to the PlacementPolicy in use
//  */
// bool hdsa_allocator_free_list_allocate(hdsa_Allocator_Free_List* const self, ptrdiff_t size, ptrdiff_t alignment, void** allocation_out, hdsa_Error* const error_out)
// {
//     /**
//      * remaining_node_space: Remaining space inside the node after the
//      * allocation size and padding were substracted from it.
//      */
//     ptrdiff_t padding = 0, alignment_padding = 0, required_space = 0, remaining_node_space = 0;
//     hdsa_Allocator_Free_List_Node* node = (hdsa_Allocator_Free_List_Node*)0;
//     hdsa_Allocator_Free_List_Node* previous_node = (hdsa_Allocator_Free_List_Node*)0;
//     hdsa_Allocator_Free_List_Header* header_ptr = (hdsa_Allocator_Free_List_Header*)0;

//     puts("************************** FREE LIST ALLOCATION ****************************\n");
//     printf("size before: %lld\n", size);
//     printf("alignment before: %lld\n", alignment);
//     printf("allocation_amount before: %lld\n", self->allocation_amount);

//     size = (size < (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node)) ? (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node) : size;
//     printf("size after: %lld\n", size);

//     alignment = (alignment < (ptrdiff_t)_Alignof(hdsa_Allocator_Free_List_Node)) ? (ptrdiff_t)_Alignof(hdsa_Allocator_Free_List_Node) : alignment;
//     printf("alignment after: %lld\n", alignment);

//     if (self->policy == HDSA_ALLOCATOR_FIND_BEST)
//     {
//         node = hdsa_allocator_free_list_find_best(self, size, alignment, &padding, &previous_node);
//     }
//     else
//     {
//         node = hdsa_allocator_free_list_find_first(self, size, alignment, &padding, &previous_node);
//     }

//     HDSA_BASIC_ASSERT((node), "Can't allocate more space for the FreeListAlloc.\n");

//     required_space = size + padding;
//     printf("required_space: %lld\n", required_space);

//     /**
//      * Thanks to the functions free_list_find_best and free_list_find_first
//      * (node->block_size - required_space) will always be positive or 0.
//      */
//     remaining_node_space = node->section_size - required_space;
//     printf("remaining_node_space: %lld\n", remaining_node_space);

//     /**
//      * If remaining_space is bigger than the size of a FreeListNode, then
//      * proceed to create a new node and add it to the free list of nodes.
//      */
//     if (remaining_node_space > (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node))
//     {
//         hdsa_Allocator_Free_List_Node* new_node = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)node + (ptrdiff_t)required_space);
//         new_node->section_size = remaining_node_space;
//         printf("new_node: %p\n", (void*)new_node);

//         hdsa_allocator_free_list_node_insert(self, new_node);
//     }

//     // Remove node from the free list of nodes, AKA it's now being used
//     hdsa_allocator_free_list_node_remove(self, previous_node, node);

//     /**
//      * Set the header for the node being allocated, this header is located
//      * (most of the time) in a different address from node.
//      * The whole node->section_size is supposed to be able to hold the padding and
//      * the bytes being allocated, so the padding starts from the address of node,
//      * the data starts after the padding, and the header_ptr is right before the
//      * data, hence why "alignment_padding" is smaller than padding.
//      */
//     alignment_padding = padding - (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header);
//     header_ptr = (hdsa_Allocator_Free_List_Header*)((ptrdiff_t)node + (ptrdiff_t)alignment_padding);

//     /**
//      * Unlike with free blocks, block_size for a used block is only the
//      * allocation size plus the size of the header.
//      */
//     header_ptr->section_size = size + (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header);
//     header_ptr->padding = alignment_padding;

//     hdsa_Allocator_Free_List_Node* result = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)header_ptr + (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header));
//     self->used_total += required_space;
//     self->allocation_amount += 1;

//     printf("alignment_padding: %lld\n", alignment_padding);
//     printf("header_ptr: %p\n", (void*)header_ptr);
//     printf("header_ptr->section_size: %lld\n", header_ptr->section_size);
//     printf("header_ptr->padding: %lld\n", header_ptr->padding);
//     printf("used_total: %lld\n", self->used_total);
//     printf("allocation_amount after: %lld\n", self->allocation_amount);
//     printf("self->head: %p\n", (void*)self->head);
//     printf("previous_node: %p\n", (void*)previous_node);
//     printf("Result: %p\n", (void*)result);

//     *allocation_out = (void*)result;
//     error_out->code = HDSA_ERROR_SUCCESS;

//     return true;
// }

// void hdsa_allocator_free_list_deallocate(hdsa_Allocator_Free_List* const self, const void* const ptr)
// {
//     puts("------------------------------ FREE LIST DEALLOCATION ----------------------------------\n");

//     hdsa_Allocator_Free_List_Header* header = (hdsa_Allocator_Free_List_Header*)0;
//     hdsa_Allocator_Free_List_Node* free_node = (hdsa_Allocator_Free_List_Node*)0;
//     hdsa_Allocator_Free_List_Node* node = (hdsa_Allocator_Free_List_Node*)0;

//     printf("allocation_amount before: %lld\n", self->allocation_amount);

//     if (!ptr)
//     {
//         puts("The pointer to deallocate from the Allocator_Free_List is null.\n");
//         return;
//     }

//     // Put the free_node where the header of ptr is
//     header = (hdsa_Allocator_Free_List_Header*)((ptrdiff_t)ptr - (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header));
//     free_node = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)header - (ptrdiff_t)header->padding);
//     free_node->section_size = header->section_size + header->padding;
//     free_node->next = (hdsa_Allocator_Free_List_Node*)0;

//     printf("header: %p\n", (void*)header);
//     printf("free_node: %p\n", (void*)free_node);
//     printf("free_node->section_size: %lld\n", free_node->section_size);
//     printf("node/head: %p\n", (void*)node);

//     hdsa_allocator_free_list_node_insert(self, free_node);

//     printf("used_total before deallocation: %lld\n", self->used_total);

//     self->used_total -= free_node->section_size;
//     self->allocation_amount -= 1;
//     hdsa_allocator_free_list_coalescence(self, self->head, free_node);

//     printf("allocation_amount after: %lld\n", self->allocation_amount);
//     printf("used_total after deallocation: %lld\n", self->used_total);
// }

// void hdsa_allocator_free_list_reset(hdsa_Allocator_Free_List* const self)
// {
//     HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");

//     printf(".............................. FREE LIST RESET ..................................\n");

//     hdsa_allocator_virtual_reset(&self->v_alloc);

//     if (!self->v_alloc.last_block) // External mode always has a null last block
//     {
//         self->used_total = 0;
//         self->used_total = 0;
//         self->allocation_amount = 0;

//         // Setting up the first node
//         self->head = (hdsa_Allocator_Free_List_Node*)self->v_alloc.buffer;

//         if (self->v_alloc.buffer)
//         {
//             self->head->section_size = self->v_alloc.committed_total;
//             self->head->next = (hdsa_Allocator_Free_List_Node*)0;
//         }

//         puts("The Allocator_Free_List was empty or in External mode when trying to reset it. All the allocator data was set to 0.\n");

//         return;
//     }

//     self->v_alloc.last_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
//     self->used_total = self->v_alloc.last_block->used_block;

//     self->used_total = self->v_alloc.last_block->used_block;
//     self->allocation_amount = 1;

//     // Setting up the first free node
//     self->head = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)self->v_alloc.last_block + self->v_alloc.last_block->used_block);
//     self->head->section_size = self->v_alloc.committed_total - self->v_alloc.last_block->used_block;
//     self->head->next = (hdsa_Allocator_Free_List_Node*)0;

//     puts("The Allocator_Free_List has been reset!\n");
// }
