#include "free_list_alloc.h"

void hdsa_allocator_free_list_reset(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Placement_Policy policy)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");

    printf(".............................. FREE LIST RESET ..................................\n");

    hdsa_allocator_virtual_reset(&self->v_alloc);

    if (!self->v_alloc.last_block) // External mode always has a null last block
    {
        self->used_bytes = 0;
        self->policy = policy;
        self->used_total = 0;
        self->allocation_amount = 0;

        // Setting up the first node
        self->head = (hdsa_Allocator_Free_List_Node*)self->v_alloc.buffer;

        if (self->v_alloc.buffer)
        {
            self->head->section_size = self->v_alloc.committed_total;
            self->head->next = (hdsa_Allocator_Free_List_Node*)0;
        }

        puts("The Allocator_Free_List was empty or in External mode when trying to reset it. All the allocator data was set to 0.\n");

        return;
    }

    self->v_alloc.last_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->used_bytes = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->policy = policy;

    self->used_total = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->allocation_amount = 1;

    // Setting up the first node
    self->head = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block));
    self->head->section_size = self->v_alloc.committed_total;
    self->head->next = (hdsa_Allocator_Free_List_Node*)0;

    puts("The Allocator_Free_List has been reset!\n");
}

void hdsa_allocator_free_list_init_external(hdsa_Allocator_Free_List* const self, void* buff, ptrdiff_t size, hdsa_Allocator_Placement_Policy policy)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Stack pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "The buffer size to initialize the Allocator_Stack is less than 1.\n");

    hdsa_allocator_virtual_init_external(&self->v_alloc, buff, size);
    hdsa_allocator_free_list_reset(self, policy);
}

/**
 * It looks for the first free node that has at least the required size plus the size of FreeListHeader
 */
static hdsa_Allocator_Free_List_Node* hdsa_allocator_free_list_find_first
(
    hdsa_Allocator_Free_List* const self,
    ptrdiff_t allocation_size,
    ptrdiff_t alignment,
    ptrdiff_t* allocation_padding,
    hdsa_Allocator_Free_List_Node** const allocation_previous_node
)
{
    puts(".............................. FREE LIST FIND FIRST ..................................\n");

    hdsa_Allocator_Free_List_Node* node = self->head;
    hdsa_Allocator_Free_List_Node* previous_node = (hdsa_Allocator_Free_List_Node*)0;
    ptrdiff_t padding = 0;

    printf("head and node: %p\n", (void*)node);

    if (!node)
    {
        return (hdsa_Allocator_Free_List_Node*)0;
    }

    while (node)
    {
        padding = hdsa_common_calc_padding_with_header((ptrdiff_t)node, (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header), alignment);
        ptrdiff_t required_space = allocation_size + padding;

        printf("padding: %lld\n", padding);
        printf("allocation_size + padding: %lld\n", required_space);

        if (node->section_size >= required_space)
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

    if (allocation_padding)
    {
        *allocation_padding = padding;
        printf("*allocation_padding: %lld\n", *allocation_padding);
    }
    else
    {
        puts("*allocation_padding: 0\n");
    }

    if (allocation_previous_node)
    {
        *allocation_previous_node = previous_node;
    }

    printf("*allocation_previous_node: %p\n", (void*)(*allocation_previous_node));
    printf("Chosen node: %p\n", (void*)node);

    return node;
}

/**
 * It looks for the node has the smallest size that can fit the required size plus the size of FreeListHeader
 */
static hdsa_Allocator_Free_List_Node* hdsa_allocator_free_list_find_best
(
    hdsa_Allocator_Free_List* const self,
    ptrdiff_t allocation_size,
    ptrdiff_t alignment,
    ptrdiff_t* allocation_padding,
    hdsa_Allocator_Free_List_Node** const allocation_previous_node
)
{
    puts(".............................. FREE LIST FIND BEST ..................................\n");

    hdsa_Allocator_Free_List_Node* node = self->head;
    hdsa_Allocator_Free_List_Node* previous_node = (hdsa_Allocator_Free_List_Node*)0;
    hdsa_Allocator_Free_List_Node* best_node = (hdsa_Allocator_Free_List_Node*)0;
    ptrdiff_t padding = 0;
    ptrdiff_t smallest_difference = PTRDIFF_MAX;

    printf("head and node: %p\n", (void*)node);

    if (!node)
    {
        return (hdsa_Allocator_Free_List_Node*)0;
    }

    while (true)
    {
        padding = hdsa_common_calc_padding_with_header((ptrdiff_t)node, (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header), alignment);
        ptrdiff_t required_space = allocation_size + padding;

        printf("padding: %lld\n", padding);
        printf("allocation_size + padding: %lld\n", required_space);

        if ((node->section_size >= required_space) && ((node->section_size - required_space) < smallest_difference))
        {
            best_node = node;
            printf("best_node: %p\n", (void*)best_node);

            smallest_difference = node->section_size - required_space;
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

    if (allocation_padding)
    {
        *allocation_padding = padding;
        printf("*allocation_padding: %lld\n", *allocation_padding);
    }
    else
    {
        puts("*allocation_padding: 0\n");
    }

    if (allocation_previous_node)
    {
        *allocation_previous_node = previous_node;
    }

    printf("*allocation_previous_node: %p\n", (void*)(*allocation_previous_node));
    printf("Chosen node: %p\n", (void*)node);

    return best_node;
}

/**
 * It adds new_node to the "free list" of nodes by making it the next of head. If new_node is null this function does nothing
 */
void hdsa_allocator_free_list_node_insert(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Free_List_Node* const new_node)
{
    if (!new_node)
    {
        puts("new_node is null, no nodes will be inserted in the list.\n");
        return;
    }

    if (!self->head)
    {
        self->head = new_node;
        return;
    }

    if (self->head == new_node)
    {
        puts("The tail and new_node are the same, no new node will be inserted in the free list.\n");
        return;
    }

    new_node->next = self->head->next;
    self->head->next = new_node;
}

/**
 * It removes del_node from the "free list" of nodes
 */
void hdsa_allocator_free_list_node_remove(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Free_List_Node* previous_node, hdsa_Allocator_Free_List_Node* const delete_node)
{
    if (!delete_node)
    {
        puts("del_node is null, no nodes will be removed from the list.\n");
        return;
    }

    if (!self->head)
    {
        puts("head is null, no nodes will be removed from the list.\n");
        return;
    }

    if (!previous_node)
    {
        self->head = delete_node->next;
        printf("head: %p\n", (void*)self->head);

        return;
    }

    if (previous_node == self->head)
    {
        if (previous_node == delete_node)
        {
            self->head = delete_node->next;
            printf("head: %p\n", (void*)self->head);
        }
        else
        {
            self->head->next = delete_node->next;
            printf("head->next: %p\n", (void*)self->head->next);
        }
    }
    else
    {
        if (previous_node == delete_node)
        {
            previous_node = delete_node->next;
            printf("previous_node: %p\n", (void*)previous_node);
        }
        else
        {
            previous_node->next = delete_node->next;
            printf("previous_node->next: %p\n", (void*)previous_node->next);
        }
    }
}

/**
 * Tries to combine free_node and_free_node->next into a single free block
 * and tries to do the same with previous_node and free_node afterwards
 */
void hdsa_allocator_free_list_coalescence(hdsa_Allocator_Free_List* const self, hdsa_Allocator_Free_List_Node* const previous_node, hdsa_Allocator_Free_List_Node* const free_node)
{
    puts("------------------------------ FREE LIST COALESCENCE ----------------------------------\n");

    if (!previous_node || !free_node)
    {
        puts("One or both nodes are null, coalescence isn't possible.\n");
        return;
    }

    if (free_node->next && (((ptrdiff_t)free_node + free_node->section_size) == (ptrdiff_t)free_node->next))
    {
        free_node->section_size += free_node->next->section_size;
        hdsa_allocator_free_list_node_remove(self, free_node, free_node->next);
        puts("Combining free_node and free_node->next.\n");
    }

    if (previous_node->next && (((ptrdiff_t)previous_node + previous_node->section_size) == (ptrdiff_t)free_node))
    {
        previous_node->section_size += free_node->section_size;
        hdsa_allocator_free_list_node_remove(self, previous_node, free_node);
        puts("Combining previous_node and free_node.\n");
    }

    if ((self->head->next == previous_node) && previous_node->next)
    {
        if (((ptrdiff_t)previous_node + previous_node->section_size) == (ptrdiff_t)self->head)
        {
            previous_node->section_size += self->head->section_size;
            hdsa_allocator_free_list_node_remove(self, previous_node, self->head);
            puts("Combining head into previous_node.\n");
        }
        else if (((ptrdiff_t)self->head + self->head->section_size) == (ptrdiff_t)previous_node)
        {
            self->head->section_size += previous_node->section_size;
            hdsa_allocator_free_list_node_remove(self, self->head, previous_node);
            puts("Combining previous_node into head.\n");
        }
    }

    printf("free_node->next isn't null. free_node->block_size: %lld\n", free_node->section_size);
    printf("previous_node->next isn't null. previous_node->block_size: %lld\n", previous_node->section_size);
}

/**
 * Allocates within a node according to the PlacementPolicy in use
 */
bool hdsa_allocator_free_list_allocate(hdsa_Allocator_Free_List* const self, ptrdiff_t size, ptrdiff_t alignment, void** allocation_out, hdsa_Error* const error_out)
{
    /**
     * remaining_node_space: Remaining space inside the node after the
     * allocation size and padding were substracted from it.
     */
    ptrdiff_t padding = 0, alignment_padding = 0, required_space = 0, remaining_node_space = 0;
    hdsa_Allocator_Free_List_Node* node = (hdsa_Allocator_Free_List_Node*)0;
    hdsa_Allocator_Free_List_Node* previous_node = (hdsa_Allocator_Free_List_Node*)0;
    hdsa_Allocator_Free_List_Header* header_ptr = (hdsa_Allocator_Free_List_Header*)0;

    puts("************************** FREE LIST ALLOCATION ****************************\n");
    printf("size before: %lld\n", size);
    printf("alignment before: %lld\n", alignment);
    printf("allocation_amount before: %lld\n", self->allocation_amount);

    size = (size < (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node)) ? (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node) : size;
    printf("size after: %lld\n", size);

    alignment = (alignment < (ptrdiff_t)_Alignof(hdsa_Allocator_Free_List_Node)) ? (ptrdiff_t)_Alignof(hdsa_Allocator_Free_List_Node) : alignment;
    printf("alignment after: %lld\n", alignment);

    if (self->policy == HDSA_ALLOCATOR_FIND_BEST)
    {
        node = hdsa_allocator_free_list_find_best(self, size, alignment, &padding, &previous_node);
    }
    else
    {
        node = hdsa_allocator_free_list_find_first(self, size, alignment, &padding, &previous_node);
    }

    HDSA_BASIC_ASSERT((node), "Can't allocate more space for the FreeListAlloc.\n");

    required_space = size + padding;
    printf("required_space: %lld\n", required_space);

    /**
     * Thanks to the functions free_list_find_best and free_list_find_first
     * (node->block_size - required_space) will always be positive or 0.
     */
    remaining_node_space = node->section_size - required_space;
    printf("remaining_node_space: %lld\n", remaining_node_space);

    /**
     * If remaining_space is bigger than the size of a FreeListNode, then
     * proceed to create a new node and add it to the free list of nodes.
     */
    if (remaining_node_space > (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Node))
    {
        hdsa_Allocator_Free_List_Node* new_node = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)node + (ptrdiff_t)required_space);
        new_node->section_size = remaining_node_space;
        printf("new_node: %p\n", (void*)new_node);

        hdsa_allocator_free_list_node_insert(self, new_node);
    }

    // Remove node from the free list of nodes, AKA it's now being used
    hdsa_allocator_free_list_node_remove(self, previous_node, node);

    /**
     * Set the header for the node being allocated, this header is located
     * (most of the time) in a different address from node.
     * The whole node->section_size is supposed to be able to hold the padding and
     * the bytes being allocated, so the padding starts from the address of node,
     * the data starts after the padding, and the header_ptr is right before the
     * data, hence why "alignment_padding" is smaller than padding.
     */
    alignment_padding = padding - (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header);
    header_ptr = (hdsa_Allocator_Free_List_Header*)((ptrdiff_t)node + (ptrdiff_t)alignment_padding);

    /**
     * Unlike with free blocks, block_size for a used block is only the
     * allocation size plus the size of the header.
     */
    header_ptr->section_size = size + (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header);
    header_ptr->padding = alignment_padding;

    hdsa_Allocator_Free_List_Node* result = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)header_ptr + (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header));
    self->used_bytes += required_space;
    self->allocation_amount += 1;

    printf("alignment_padding: %lld\n", alignment_padding);
    printf("header_ptr: %p\n", (void*)header_ptr);
    printf("header_ptr->section_size: %lld\n", header_ptr->section_size);
    printf("header_ptr->padding: %lld\n", header_ptr->padding);
    printf("used_bytes: %lld\n", self->used_bytes);
    printf("allocation_amount after: %lld\n", self->allocation_amount);
    printf("self->head: %p\n", (void*)self->head);
    printf("previous_node: %p\n", (void*)previous_node);
    printf("Result: %p\n", (void*)result);

    return result;
}

void hdsa_allocator_free_list_deallocate(hdsa_Allocator_Free_List* const self, const void* const ptr)
{
    puts("------------------------------ FREE LIST DEALLOCATION ----------------------------------\n");

    hdsa_Allocator_Free_List_Header* header = (hdsa_Allocator_Free_List_Header*)0;
    hdsa_Allocator_Free_List_Node* free_node = (hdsa_Allocator_Free_List_Node*)0;
    hdsa_Allocator_Free_List_Node* node = (hdsa_Allocator_Free_List_Node*)0;

    printf("allocation_amount before: %lld\n", self->allocation_amount);

    if (!ptr)
    {
        puts("The pointer to deallocate from the Allocator_Free_List is null.\n");
        return;
    }

    // Put the free_node where the header of ptr is
    header = (hdsa_Allocator_Free_List_Header*)((ptrdiff_t)ptr - (ptrdiff_t)sizeof(hdsa_Allocator_Free_List_Header));
    free_node = (hdsa_Allocator_Free_List_Node*)((ptrdiff_t)header - (ptrdiff_t)header->padding);
    free_node->section_size = header->section_size + header->padding;
    free_node->next = (hdsa_Allocator_Free_List_Node*)0;

    printf("header: %p\n", (void*)header);
    printf("free_node: %p\n", (void*)free_node);
    printf("free_node->section_size: %lld\n", free_node->section_size);
    printf("node/head: %p\n", (void*)node);

    hdsa_allocator_free_list_node_insert(self, free_node);

    printf("used_bytes before deallocation: %lld\n", self->used_bytes);

    self->used_bytes -= free_node->section_size;
    self->allocation_amount -= 1;
    hdsa_allocator_free_list_coalescence(self, self->head, free_node);

    printf("allocation_amount after: %lld\n", self->allocation_amount);
    printf("used_bytes after deallocation: %lld\n", self->used_bytes);
}
