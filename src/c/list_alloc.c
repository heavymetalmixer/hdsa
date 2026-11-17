#include "list_alloc.h"

/**
 * @brief It adds a used a node to the free list.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] new_node Node to add to the free list, if it's null or the
 * same as self-head this function does nothing.
 */
static void hdsa_allocator_list_free_list_add(hdsa_Allocator_List* const self, hdsa_Allocator_List_Free* const new_node)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((new_node), "new_node is null.\n");

    if (self->head == new_node)
    {
        puts("The tail and new_node are the same, no new node will be inserted in the free list.\n");
        return;
    }

    if (!self->head)
    {
        self->head = new_node;
        self->head->previous = (hdsa_Allocator_List_Free*)0;
        self->head->next = (hdsa_Allocator_List_Free*)0;
        return;
    }

    hdsa_Allocator_List_Free* head_next = self->head->next;

    head_next->previous = new_node;
    new_node->next = head_next;

    self->head->next = new_node;
    new_node->previous = self->head;
}

/**
 * @brief It removes a node from the free list so it can be used for an
 * allocation.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] delete_node Node to remove from the free list, must not be null.
 * @param[in] error_out Pointer to the error struct, must not be null.
 * @returns True if delete_node was removed from the used list, false
 * othwerwise.
 */
static bool hdsa_allocator_list_free_list_remove(hdsa_Allocator_List* const self, hdsa_Allocator_List_Free* const * const delete_node, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((delete_node), "delete_node is null.\n");

    if (!self->head)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_FREE_LIST_IS_EMPTY;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->used_head, __LINE__ - 3, "The free list in the Allocator_List is empty, self->head is null.\n");

        return false;
    }

    if (!(*delete_node))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_FREE_NODE_WASNT_REMOVED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !(*delete_node), __LINE__ - 3, "A pointer provided by user isn't part of the free list in the Allocator_List.\n");

        return false;
    }

    hdsa_Allocator_List_Free* previous_node = (*delete_node)->previous;
    hdsa_Allocator_List_Free* next_node = (*delete_node)->next;

    if (self->head == (*delete_node))
    {
        self->head->next = (hdsa_Allocator_List_Free*)0;
        next_node->previous = (hdsa_Allocator_List_Free*)0;
        self->head = next_node;
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    if (previous_node == self->head)
    {
        self->head->next = next_node;
        next_node->previous = self->head;
        (*delete_node)->next = (hdsa_Allocator_List_Free*)0;
        (*delete_node)->previous = (hdsa_Allocator_List_Free*)0;
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    previous_node->next = next_node;
    next_node->previous = previous_node;
    (*delete_node)->next = (hdsa_Allocator_List_Free*)0;
    (*delete_node)->previous = (hdsa_Allocator_List_Free*)0;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It adds a used a node to the used list to be delivered to the user.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] new_node Node to add to the free list, must not be null not the
 * same as self->used_head.
 */
static void hdsa_allocator_list_used_list_add(hdsa_Allocator_List* const self, hdsa_Allocator_List_Used* const * const new_node)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((new_node), "new_node is null.\n");
    HDSA_BASIC_ASSERT((*new_node), "*new_node is null.\n");
    HDSA_BASIC_ASSERT(((ptrdiff_t)(*new_node) != (ptrdiff_t)self->used_head), "*new_node is self->used_head.\n");

    if (!self->used_head)
    {
        self->used_head = *new_node;
        (*new_node)->next = (hdsa_Allocator_List_Used*)0;
        return;
    }

    (*new_node)->next = self->used_head->next;
    self->used_head->next = *new_node;
}

/**
 * @brief It removes a node from the used list so it can be turned into a free
 * list node.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] previous_node Previous used node to delete_node in the list, must
 * not be null.
 * @param[in] delete_node Node to remove from the free list, must not be null.
 * @param[in] error_out Pointer to the error struct, must not be null.
 * @returns True if delete_node was removed from the used list, false
 * othwerwise.
 */
static bool hdsa_allocator_list_used_list_remove(hdsa_Allocator_List* const self, hdsa_Allocator_List_Used** const previous_node, hdsa_Allocator_List_Used** const delete_node, hdsa_Error* const error_out)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_Free_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((previous_node), "previous_node is null.\n");
    HDSA_BASIC_ASSERT((delete_node), "delete_node is null.\n");

    if (!self->used_head)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_USED_LIST_IS_EMPTY;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->used_head, __LINE__ - 3, "The used list in the Allocator_List is empty, self->used_head is null.\n");

        return false;
    }

    if (!(*delete_node))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_USED_NODE_WASNT_REMOVED;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !(*delete_node), __LINE__ - 3, "A pointer provided by user isn't part of the used list in the Allocator_List.\n");

        return false;
    }

    if (!(*previous_node))
    {
        if ((ptrdiff_t)(*delete_node) != (ptrdiff_t)self->used_head)
        {
            error_out->code = HDSA_ERROR_ALLOCATION_USED_NODE_WASNT_REMOVED;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, (!(*previous_node)) && ((ptrdiff_t)(*delete_node) != (ptrdiff_t)self->used_head), __LINE__ - 3, "A pointer provided by user isn't part of the used_list in the Allocator_List.\n");

            return false;
        }

        self->used_head = self->used_head->next;
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    HDSA_BASIC_ASSERT(((ptrdiff_t)(*previous_node)->next == (ptrdiff_t)(*delete_node)), "*delete_node isn't the next of *previous_node");

    hdsa_Allocator_List_Used* next_node = (*delete_node)->next;

    (*previous_node)->next = next_node;
    (*delete_node)->next = (hdsa_Allocator_List_Used*)0;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It creates a properly aligned hdsa_Allocator_List_Footer for a new
 * free node without it. It may reduce the node's section size further than
 * just the size of hdsa_Allocator_List_Footer.
 * @param[in] node Free node to set the footer on.
 * @param[in] new_section_size section_size of node. It's called "new"
 * because a smaller one can be passed to set a footer in the middle of the
 * old section_size to create a new free node afterwards in
 * hdsa_allocator_list_create_free_node().
 * @param[out] new_footer_out Output pointer for the resulting footer, must
 * not be null.
 */
static void hdsa_allocator_list_create_footer(hdsa_Allocator_List_Free* const * const node, ptrdiff_t new_section_size, hdsa_Allocator_List_Footer** const new_footer_out)
{
    hdsa_Allocator_List_Footer* footer = (hdsa_Allocator_List_Footer*)((ptrdiff_t)*node + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + new_section_size);
    footer->node_offset = (ptrdiff_t)footer - (ptrdiff_t)*node;
    footer->is_free = true;
    *new_footer_out = footer;
}

/**
 * @brief Create a new free list node from an existing one or from memory space
 * not being used by any of the 2 lists.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] old_node Existing free node from which the new free node is gonna
 * be made. If the pointer old_node is pointing to is null, a new free node
 * will be made using the remaining extra space in the buffer/memory block.
 * @param[in] required_space Sum of the hdsa_Allocator_List_Free header and
 * old_node->section_size, it must be bigger than 0.
 * @param[out] node_out Output pointer for the resulting new free node, must
 * not be null.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if there's enough space to create a new free node, false
 * otherwise.
 */
static bool hdsa_allocator_list_create_free_node
(
    hdsa_Allocator_List* const self,
    hdsa_Allocator_List_Free** const old_node,
    ptrdiff_t required_space,
    hdsa_Allocator_List_Free** const node_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((required_space > 0), "required_space is 0 or negative.\n");
    HDSA_BASIC_ASSERT((old_node), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((node_out), "node_out is null.\n");

    if (!(*old_node))
    {
        ptrdiff_t remaining_buffer_space = self->v_alloc.committed_total - self->allocated_offset;
        remaining_buffer_space = hdsa_common_round_to_multiple(remaining_buffer_space, (ptrdiff_t)_Alignof(hdsa_Allocator_List_Footer));
        remaining_buffer_space = (remaining_buffer_space > (self->v_alloc.committed_total - self->allocated_offset)) ? remaining_buffer_space - (ptrdiff_t)_Alignof(hdsa_Allocator_List_Footer) : remaining_buffer_space;

        if (remaining_buffer_space < ((ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)_Alignof(hdsa_Allocator_List_Footer) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer)))
        {
            error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !node, __LINE__ - 3, "There's not enough extra space in the last_block/buffer to create a new free node.");
            *node_out = (hdsa_Allocator_List_Free*)0;

            return false;
        }

        // Case: There are no free nor used nodes
        if (!self->head && !self->used_head)
        {
            hdsa_Allocator_List_Free* new_node = (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) ? (hdsa_Allocator_List_Free*)(hdsa_common_align_forward((ptrdiff_t)self->v_alloc.last_block + self->allocated_offset, (ptrdiff_t)_Alignof(hdsa_Allocator_List_Free))) : (hdsa_Allocator_List_Free*)(hdsa_common_align_forward((ptrdiff_t)self->v_alloc.buffer, (ptrdiff_t)_Alignof(hdsa_Allocator_List_Free)));

            self->allocated_offset += ((ptrdiff_t)new_node - (ptrdiff_t)self->v_alloc.buffer);

            new_node->section_size = remaining_buffer_space - (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) - (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);
            hdsa_allocator_list_free_list_add(self, new_node);

            hdsa_Allocator_List_Footer* new_footer = (hdsa_Allocator_List_Footer*)0;
            hdsa_allocator_list_create_footer(&new_node, new_node->section_size, &new_footer);

            if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
            {
                self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);
            }

            self->allocated_offset += (((ptrdiff_t)new_footer + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer)) - (ptrdiff_t)new_node);
            self->used_total += (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);

            *node_out = new_node;
            error_out->code = HDSA_ERROR_SUCCESS;

            return true;
        }

        // Case: Create a new free node from extra space in memory
        hdsa_Allocator_List_Free* new_node = (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC) ? (hdsa_Allocator_List_Free*)((ptrdiff_t)self->v_alloc.last_block + self->allocated_offset) : (hdsa_Allocator_List_Free*)((ptrdiff_t)self->v_alloc.buffer + self->allocated_offset);

        new_node->section_size = remaining_buffer_space - (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) - (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);
        hdsa_allocator_list_free_list_add(self, new_node);

        hdsa_Allocator_List_Footer* new_footer = (hdsa_Allocator_List_Footer*)0;
        hdsa_allocator_list_create_footer(&new_node, new_node->section_size, &new_footer);

        if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
        {
            self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);
        }

        self->allocated_offset += (((ptrdiff_t)new_footer + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer)) - (ptrdiff_t)new_node);
        self->used_total += (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);

        *node_out = new_node;
        error_out->code = HDSA_ERROR_SUCCESS;

        return true;
    }

    // Case: Create a new free node from an existing one
    ptrdiff_t smaller_section_size = required_space - (ptrdiff_t)sizeof(hdsa_Allocator_List_Free);

    // required_space doesn't include the size of the footer
    if (required_space < ((ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)_Alignof(hdsa_Allocator_List_Footer)))
    {
        error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !node, __LINE__ - 3, "There's not enough extra space in the last_block/buffer to create a new free node.");
        *node_out = (hdsa_Allocator_List_Free*)0;

        return false;
    }

    hdsa_Allocator_List_Footer* old_footer = (hdsa_Allocator_List_Footer*)((ptrdiff_t)*old_node + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (*old_node)->section_size); // This one will be reused by the new second free node done from remaining_size

    hdsa_Allocator_List_Footer* new_footer = (hdsa_Allocator_List_Footer*)0; // Footer for soon smaller free node
    hdsa_allocator_list_create_footer(old_node, smaller_section_size, &new_footer);
    (*old_node)->section_size = smaller_section_size;

    hdsa_Allocator_List_Free* second_node = (hdsa_Allocator_List_Free*)((ptrdiff_t)new_footer + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer));
    second_node->section_size = (ptrdiff_t)old_footer - (ptrdiff_t)second_node - (ptrdiff_t)sizeof(hdsa_Allocator_List_Free);
    hdsa_allocator_list_free_list_add(self, second_node);
    old_footer->node_offset = (ptrdiff_t)old_footer - (ptrdiff_t)second_node;

    if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
    {
        self->v_alloc.last_block->used_block += (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);
    }

    self->used_total += (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);

    *node_out = second_node;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It initializes the Allocator_List with an existing buffer of a
 * certain size.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] buff Buffer being assigned to the Allocator_Free_List. It can be
 * null but make sure to change it later on. If it is null or size is 0, all
 * the allocator's data is set to 0/null.
 * @param[in] size Size of the buffer in bytes, must be at least 0.
 */
void hdsa_allocator_list_init_external(hdsa_Allocator_List* const self, void* buffer, ptrdiff_t size, hdsa_Allocator_Placement_Policy policy)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((size > -1), "The buffer size to initialize the Allocator_List is less than 1.\n");
    HDSA_BASIC_ASSERT(((policy == HDSA_ALLOCATOR_FIND_BEST) || (policy == HDSA_ALLOCATOR_FIND_BEST)), "Incorrect value for the Placement_Policy.\n");

    hdsa_allocator_virtual_init_external(&self->v_alloc, buffer, size);
    self->allocation_amount = 0;
    self->policy = policy;
    self->used_total = 0;
    self->allocated_offset = 0;
    self->head = (hdsa_Allocator_List_Free*)0;
    self->used_head = (hdsa_Allocator_List_Used*)0;

    if (buffer)
    {
        hdsa_Allocator_List_Free* dummy_node = (hdsa_Allocator_List_Free*)0;
        hdsa_Error dummy_error;
        hdsa_allocator_list_create_free_node(self, &self->head, 1, &dummy_node, &dummy_error);
    }
}

/**
 * @brief It initializes the Allocator_List with sizes for reserved and
 * committed memory for a first virtual memory allocation inside the function.
 *
 * The allocator will only hold one Memory Block at a time so to change the
 * reserved amount, the current block must be deallocated first and then to
 * perform a new allocation with the new reserved amount.
 * @param[in] self Allocator_List pointer, must not be null.
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
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((reserved_size > -1), "The reserved_size to initialize the Allocator_List is less than 1.\n");
    HDSA_BASIC_ASSERT((committed_size > -1), "The buffer size to initialize the Allocator_List is less than 1.\n");

    self->allocation_amount = 0;
    self->policy = policy;
    self->used_total = 0;
    self->allocated_offset = 0;
    self->head = (hdsa_Allocator_List_Free*)0;
    self->used_head = (hdsa_Allocator_List_Used*)0;

    if (!hdsa_allocator_virtual_init_static(&self->v_alloc, reserved_size, committed_size, error_out))
    {
        return false;
    }

    if (reserved_size > 0)
    {
        self->allocated_offset = self->v_alloc.last_block->used_block;
        hdsa_Allocator_List_Free* dummy_node = (hdsa_Allocator_List_Free*)0;
        hdsa_allocator_list_create_free_node(self, &self->head, 1, &dummy_node, error_out);
        self->allocation_amount += 1;
    }

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It looks for the first free node that can fit the requested size and
 * padding (for alignment).
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] size Size required by the user in bytes.
 * @param[in] alignment Alignment required by the user in bytes, it must be
 * a power of 2.
 * @param[out] padding_out Output pointer to a variable for storing the padding
 * value, must not be null.
 * @param[out] node_out Output pointer to a pointer for the node that will be
 * used for the allocation, must not be null.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
static bool hdsa_allocator_list_find_first
(
    const hdsa_Allocator_List* const self,
    ptrdiff_t size,
    ptrdiff_t alignment,
    ptrdiff_t* const padding_out,
    hdsa_Allocator_List_Free** const node_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((padding_out), "padding_out is null.\n");
    HDSA_BASIC_ASSERT((node_out), "node_out is null.\n");
    HDSA_BASIC_ASSERT((size > 0), "size to find a node is 0 or negative.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "size to find a node is 0 or negative.\n");

    puts(".............................. LIST FIND FIRST ..................................\n");

    hdsa_Allocator_List_Free* node = self->head;
    ptrdiff_t padding = 0;

    printf("head and node: %p\n", (void*)node);

    if (!self->head)
    {
        error_out->code = HDSA_ERROR_NULL_POINTER_INTERNAL_DATA;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->head, __LINE__ - 3, "The Allocator_List head is null, a fitting node can't be found.");
        *node_out = (hdsa_Allocator_List_Free*)0;

        return false;
    }

    while (node)
    {
        padding = hdsa_common_padding_with_header((ptrdiff_t)node, (ptrdiff_t)sizeof(hdsa_Allocator_List_Used), alignment);
        ptrdiff_t required_space = size + padding;

        printf("padding: %lld\n", padding);
        printf("allocation_size + padding: %lld\n", required_space);

        // if (node->section_size >= required_space)
        if ((node->section_size + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free)) >= required_space)
        {
            printf("Found node: %p\n", (void*)node);
            printf("Found node->section_size: %lld\n", node->section_size);
            break;
        }

        node = node->next;

        printf("node: %p\n", (void*)node);
    }

    if (!node)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_NO_FITTING_NODE_FOUND;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !node, __LINE__ - 3, "A node with the requiered size and alignment couldn't be found.");
        *node_out = (hdsa_Allocator_List_Free*)0;

        return false;
    }

    *padding_out = padding;
    *node_out = node;

    printf("*padding_out: %lld\n", *padding_out);
    printf("Chosen node: %p\n", (void*)*node_out);

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It looks for first node with the smallest size that fits the
 * requested size and padding. It takes way longer than find_first but it
 * tends to cause less memory fragmentation.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] size Size required by the user in bytes.
 * @param[in] alignment Alignment required by the user in bytes, it must be
 * a power of 2.
 * @param[out] padding_out Output pointer to a variable for storing the padding
 * value, must not be null.
 * @param[out] node_out Output pointer to a pointer for the node that will be
 * used for the allocation, must not be null.
 * @param[in] error_out The error struct pointer, must not be null.
 * @returns True if the first allocation inside succeeds, false otherwise.
 */
static bool hdsa_allocator_list_find_best
(
    const hdsa_Allocator_List* const self,
    ptrdiff_t size,
    ptrdiff_t alignment,
    ptrdiff_t* const padding_out,
    hdsa_Allocator_List_Free** const node_out,
    hdsa_Error* const error_out
)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");
    HDSA_BASIC_ASSERT((error_out), "The hdsa_Error pointer is null.\n");
    HDSA_BASIC_ASSERT((padding_out), "padding_out is null.\n");
    HDSA_BASIC_ASSERT((node_out), "node_out is null.\n");
    HDSA_BASIC_ASSERT((size > 0), "size to find a node is 0 or negative.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "size to find a node is 0 or negative.\n");

    puts(".............................. LIST FIND BEST ..................................\n");

    hdsa_Allocator_List_Free* node = self->head;
    hdsa_Allocator_List_Free* best_node = (hdsa_Allocator_List_Free*)0;
    ptrdiff_t padding = 0;
    ptrdiff_t smallest_difference = PTRDIFF_MAX;
    ptrdiff_t node_size = 0;

    printf("head and node: %p\n", (void*)node);

    if (!self->head)
    {
        error_out->code = HDSA_ERROR_NULL_POINTER_INTERNAL_DATA;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->head, __LINE__ - 3, "The Allocator_List head is null, a fitting node can't be found.");
        *node_out = (hdsa_Allocator_List_Free*)0;

        return false;
    }

    while (node)
    {
        padding = hdsa_common_padding_with_header((ptrdiff_t)node, (ptrdiff_t)sizeof(hdsa_Allocator_List_Used), alignment);
        ptrdiff_t required_space = size + padding;
        node_size = (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + node->section_size;

        printf("padding: %lld\n", padding);
        printf("allocation_size + padding: %lld\n", required_space);

        if ((node_size >= required_space) && ((node_size - required_space) < smallest_difference))
        {
            best_node = node;
            printf("best_node: %p\n", (void*)best_node);

            smallest_difference = node_size - required_space;
            printf("smallest_difference: %lld\n", smallest_difference);
        }

        if ((smallest_difference == 0) || !node->next)
        {
            printf("node: %p\n", (void*)node);

            break;
        }

        node = node->next;
        printf("node: %p\n", (void*)node);
    }

    if (!node)
    {
        error_out->code = HDSA_ERROR_ALLOCATION_NO_FITTING_NODE_FOUND;
        HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !node, __LINE__ - 3, "A node with the requiered size and alignment couldn't be found.");
        *node_out = (hdsa_Allocator_List_Free*)0;

        return false;
    }

    *padding_out = padding;
    *node_out = best_node;

    printf("*padding_out: %lld\n", *padding_out);
    printf("Chosen node: %p\n", (void*)*node_out);

    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

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
bool hdsa_allocator_list_allocate(hdsa_Allocator_List* const self, ptrdiff_t size, ptrdiff_t alignment, void** const allocation_out, hdsa_Error* const error_out)
{
    /**
     * remaining_node_space: Remaining space inside the node after the
     * allocation size and padding were substracted from it.
     */
    ptrdiff_t padding = 0, required_space = 0, remaining_node_space = 0;
    hdsa_Allocator_List_Free* node = (hdsa_Allocator_List_Free*)0;
    hdsa_Allocator_List_Free* previous_node = (hdsa_Allocator_List_Free*)0;

    puts("************************** LIST ALLOCATION ****************************\n");

    // Of both heads are null, it means that there are no nodes in the buffer
    if (!self->head && !self->used_head)
    {
        if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
        {
            if(!self->v_alloc.last_block)
            {
                ptrdiff_t maximum_size = (size + alignment - 1);

                if (!hdsa_allocator_list_init_static(self, maximum_size * 4, maximum_size, HDSA_ALLOCATOR_FIND_BEST, error_out))
                {
                    *allocation_out = (void*)0;
                    return false;
                }
            }
            else
            {
                self->allocated_offset = self->v_alloc.last_block->used_block;
                hdsa_Allocator_List_Free* dummy_node = (hdsa_Allocator_List_Free*)0;
                hdsa_allocator_list_create_free_node(self, &self->head, 1, &dummy_node, error_out);
                self->allocation_amount = 1;
            }
        }
        else
        {
            if (!self->v_alloc.buffer)
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NULL_BUFFER;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->v_alloc.buffer, __LINE__ - 3, "The Allocator_List's buffer is null, the pointer inside allocation_out will be set to null.\n");
                *allocation_out = (void*)0;

                return false;
            }
            else
            {
                hdsa_Allocator_List_Free* dummy_node = (hdsa_Allocator_List_Free*)0;
                hdsa_Error dummy_error;
                hdsa_allocator_list_create_free_node(self, &self->head, 1, &dummy_node, &dummy_error);
            }
        }
    }

    printf("size before: %lld\n", size);
    printf("alignment before: %lld\n", alignment);
    printf("allocation_amount before: %lld\n", self->allocation_amount);

    // Necessary to make the creation and alignment of footers and new nodes easier
    size = hdsa_common_round_to_multiple(size, (ptrdiff_t)_Alignof(hdsa_Allocator_List_Footer));
    printf("size after: %lld\n", size);
    alignment = (alignment < (ptrdiff_t)_Alignof(hdsa_Allocator_List_Used)) ? (ptrdiff_t)_Alignof(hdsa_Allocator_List_Used) : alignment;
    printf("alignment after: %lld\n", alignment);

    // Case: There are no free nodes available, all nodes are used nodes.
    if (!self->head && self->used_head)
    {
        if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
        {
            ptrdiff_t maximum_size = size + alignment -1;

            if (!hdsa_allocator_virtual_commit(&self->v_alloc, maximum_size, allocation_out, error_out))
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !hdsa_allocator_virtual_commit(&self->v_alloc, maximum_size, allocation_out, error_out), __LINE__ - 3, "There's not enough extra space in the last_block/buffer to allocate, and new committed memory couldn't be adquired.");
                *allocation_out = (void*)0;

                return false;
            }

            // A new free node for the recently adquired committed memory
            hdsa_Allocator_List_Free* new_node = (hdsa_Allocator_List_Free*)0;
            hdsa_Allocator_List_Free* dummy_old_node = (hdsa_Allocator_List_Free*)0;

            if (!hdsa_allocator_list_create_free_node(self, &dummy_old_node, maximum_size, &new_node, error_out))
            {
                error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
                HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !hdsa_allocator_list_create_free_node(self, &dummy_old_node, maximum_size, &new_node, error_out), __LINE__ - 3, "There's not enough extra space in the last_block/buffer to create a new free node for the allocation.");
                *allocation_out = (void*)0;

                return false;
            }
        }
        else
        {
            error_out->code = HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY;
            HDSA_ERROR_FORMATED_STRING(error_out->description, error_out->code, !self->head && self->used_head, __LINE__ - 20, "There's not enough extra space in the last_block/buffer to allocate.");
            *allocation_out = (void*)0;

            return false;
        }
    }


    if (self->policy == HDSA_ALLOCATOR_FIND_BEST)
    {
        if (!hdsa_allocator_list_find_best(self, size, alignment, &padding, &node, error_out))
        {
            *allocation_out = (void*)0;
            return false;
        }
    }
    else
    {
        if (!hdsa_allocator_list_find_first(self, size, alignment, &padding, &node, error_out))
        {
            *allocation_out = (void*)0;
            return false;
        }
    }

    HDSA_BASIC_ASSERT((node), "Can't allocate more space for the Allocator_List.\n");

    previous_node = node->previous;
    required_space = size + padding;
    printf("required_space: %lld\n", required_space);

    hdsa_Allocator_List_Footer* node_footer = (hdsa_Allocator_List_Footer*)((ptrdiff_t)node + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + node->section_size);
    node_footer->is_free =  false;

    // Both operands add the size of a footer, so they cancel each other
    remaining_node_space = ((ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + node->section_size) - required_space;
    printf("remaining_node_space: %lld\n", remaining_node_space);

    /**
     * If remaining_node_space is bigger than the size of
     * hdsa_Allocator_List_Free + hdsa_Allocator_List_Footer,then proceed to create
     * a new free node and add it to the free list.
     */
    hdsa_Allocator_List_Free* new_node = (hdsa_Allocator_List_Free*)0;

    if (hdsa_allocator_list_create_free_node(self, &node, required_space, &new_node, error_out))
    {
        printf("new_node: %p\n", (void*)new_node);
    }

    // Remove node from the free list of nodes
    if (!hdsa_allocator_list_free_list_remove(self, &node, error_out))
    {
        *allocation_out = (void*)0;
        return false;
    }

    /**
     * Set the used list header for the node being allocated, this header is located
     * (most of the time) in a different address from node.
     * The padding starts from the address of the previously free list node, the
     * data starts after the padding, and the used list header is right before
     * the data.
     */
    hdsa_Allocator_List_Used* used_node = (hdsa_Allocator_List_Used*)((ptrdiff_t)node + padding - (ptrdiff_t)sizeof(hdsa_Allocator_List_Used));
    used_node->padding = padding;
    used_node->section_size = size;
    hdsa_allocator_list_used_list_add(self, &used_node);

    // The usable address by the user starts right after the padding ends
    void* result = (void*)((ptrdiff_t)node + padding);

    if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
    {
        self->v_alloc.last_block->used_block -= (ptrdiff_t)sizeof(hdsa_Allocator_List_Free);
        self->v_alloc.last_block->used_block += required_space;
    }

    self->used_total -= (ptrdiff_t)sizeof(hdsa_Allocator_List_Free);
    self->used_total += required_space;
    self->allocation_amount += 1;

    printf("padding: %lld\n", padding);
    printf("used_node: %p\n", (void*)used_node);
    printf("used_node->section_size: %lld\n", used_node->section_size);
    printf("used_node->padding: %lld\n", used_node->padding);
    printf("used_total: %lld\n", self->used_total);
    printf("allocation_offset: %lld\n", self->allocated_offset);
    printf("allocation_amount after: %lld\n", self->allocation_amount);
    printf("self->head: %p\n", (void*)self->head);
    printf("previous_node: %p\n", (void*)previous_node);
    printf("Result: %p\n", result);

    *allocation_out = result;
    error_out->code = HDSA_ERROR_SUCCESS;

    return true;
}

/**
 * @brief It merges *free_node with the node right after in memory, not
 * (*free_node)->next.
 * @param[in] self Pointer to the Allocator_List, must not be null.
 * @param[in] free_node Pointer to the node that will be merged with
 * the node that is right after in memory. Both free_node and *free_node
 * must not be null.
 * @param[in] free_footer Pointer to the footer than belongs to *free_node.
 * Both free_footer and *free_footer must not be null.
 * @param[in] node_after Pointer to the node right after *free_node in memory.
 * Both node_after and *node_after must not be null.
 * @param[in] footer_after Pointer to the footer than belongs to *node_after.
 * Both footer_after and *footer_after must not be null.
 */
static void hdsa_allocator_list_merge_right
(
    hdsa_Allocator_List* const self,
    hdsa_Allocator_List_Free* const * const free_node,
    hdsa_Allocator_List_Footer** const free_footer,
    hdsa_Allocator_List_Free* const * const node_after,
    hdsa_Allocator_List_Footer* const * const footer_after
)
{
    /**
     * If *node_address goes above the address pool inside committed_total,
     * merging cannot be done.
     */
    if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
    {
        if ((ptrdiff_t)*node_after >= ((ptrdiff_t)self->v_alloc.last_block + self->v_alloc.committed_total))
        {
            return;
        }
    }
    else
    {
        if ((ptrdiff_t)*node_after >= ((ptrdiff_t)self->v_alloc.buffer + self->v_alloc.committed_total))
        {
            return;
        }
    }

    (*free_node)->section_size += (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (*node_after)->section_size;
    (*footer_after)->node_offset = (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (*free_node)->section_size;
    (*free_footer)->is_free = false;
    (*free_footer)->node_offset = 0;
    *free_footer = *footer_after;

    hdsa_Allocator_List_Free* temp_previous = (*node_after)->previous;
    hdsa_Allocator_List_Free* temp_next = (*node_after)->next;

    if (temp_previous)
    {
        temp_previous->next = temp_next;
    }

    if (temp_next)
    {
        temp_next->previous = temp_previous;
    }

    (*node_after)->next = (hdsa_Allocator_List_Free*)0;
    (*node_after)->previous = (hdsa_Allocator_List_Free*)0;

    if ((ptrdiff_t)self->head == (ptrdiff_t)node_after)
    {
        self->head = temp_next;
    }
}

/**
 * @brief It merges *free_node with the node right before in memory, not
 * (*free_node)->previous.
 * @param[in] self Pointer to the Allocator_List, must not be null.
 * @param[in] free_node Pointer to the node that will be merged with
 * the node that is right before in memory. Both free_node and *free_node
 * must not be null.
 * @param[in] free_footer Pointer to the footer than belongs to *free_node.
 * Both free_footer and *free_footer must not be null.
 * @param[in] node_before Pointer to the node right before *free_node in
 * memory. Both node_before and *node_before must not be null.
 * @param[in] footer_before Pointer to the footer than belongs to *node_before.
 * Both footer_before and *footer_before must not be null.
 */
static void hdsa_allocator_list_merge_left
(
    hdsa_Allocator_List* const self,
    hdsa_Allocator_List_Free* const * const free_node,
    hdsa_Allocator_List_Footer* const * const free_footer,
    hdsa_Allocator_List_Free* const * const node_before,
    hdsa_Allocator_List_Footer** const footer_before
)
{
    /**
     * If *node_address touches the Memory Block's pool, or buffer in External Mode
     * merging cannot be done.
     */
    if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
    {
        if ((ptrdiff_t)*node_before < ((ptrdiff_t)self->v_alloc.last_block + (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block)))
        {
            return;
        }
    }
    else
    {
        if ((ptrdiff_t)*node_before < (ptrdiff_t)self->v_alloc.buffer)
        {
            return;
        }
    }

    (*node_before)->section_size = (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer) + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (*free_node)->section_size;
    (*free_footer)->node_offset = (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + (*node_before)->section_size;
    (*footer_before)->is_free = false;
    (*footer_before)->node_offset = 0;
    *footer_before = *free_footer;

    hdsa_Allocator_List_Free* temp_previous = (*free_node)->previous;
    hdsa_Allocator_List_Free* temp_next = (*free_node)->next;

    if (temp_previous)
    {
        temp_previous->next = temp_next;
    }

    if (temp_next)
    {
        temp_next->previous = temp_previous;
    }

    (*free_node)->next = (hdsa_Allocator_List_Free*)0;
    (*free_node)->previous = (hdsa_Allocator_List_Free*)0;
}

/**
 * @brief It tries to combine node that sits in memory right after free_node
 * with free_node (not necessarily free_node->next), and the same done with the
 * node just before free_node.
 * @param[in] self Allocator_List pointer, must not be null. If self->head
 * is null this function does nothing.
 * @param[in] previous_node Previous node to the one that will be removed.
 * It can be null as long as "delete_node" isn't.
 * @param[in] delete_node Node to remove from the free list, must not be null.
 */
static void hdsa_allocator_list_coalescence(hdsa_Allocator_List* const self, hdsa_Allocator_List_Free* const free_node)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");

    puts("------------------------------ FREE LIST COALESCENCE ----------------------------------\n");

    hdsa_Allocator_List_Free* previous_node = free_node->previous;

    if (!self->head)
    {
        puts("The free list head is null, coalescence isn't possible.\n");
        return;
    }

    if (!free_node)
    {
        puts("free_node is null, coalescence isn't possible.\n");
        return;
    }

    if (!previous_node)
    {
        puts("previous_node is null, coalescence isn't possible.\n");
        return;
    }

    ptrdiff_t free_node_size = (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + free_node->section_size + (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer);

    hdsa_Allocator_List_Footer* free_footer = (hdsa_Allocator_List_Footer*)((ptrdiff_t)free_node + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + free_node->section_size);

    hdsa_Allocator_List_Free* node_after = (hdsa_Allocator_List_Free*)((ptrdiff_t)free_node + free_node_size);
    hdsa_Allocator_List_Footer* footer_after = (hdsa_Allocator_List_Footer*)((ptrdiff_t)node_after + (ptrdiff_t)sizeof(hdsa_Allocator_List_Free) + node_after->section_size);

    hdsa_Allocator_List_Footer* footer_before = (hdsa_Allocator_List_Footer*)((ptrdiff_t)free_node - (ptrdiff_t)sizeof(hdsa_Allocator_List_Footer));
    hdsa_Allocator_List_Free* node_before = (hdsa_Allocator_List_Free*)((ptrdiff_t)footer_before - footer_before->node_offset);

    /**
     * head->previous is null, and this if statement covers the case of
     * head being previous_node.
     */
    if ((ptrdiff_t)node_after == (ptrdiff_t)self->head)
    {
        hdsa_allocator_list_merge_right(self, &free_node, &free_footer, &self->head, &footer_after);
    }
    else if (((ptrdiff_t)node_after == (ptrdiff_t)previous_node) && ((ptrdiff_t)previous_node != (ptrdiff_t)self->head))
    {
        hdsa_allocator_list_merge_right(self, &free_node, &free_footer, &previous_node, &footer_after);
    }
    else if (footer_after->is_free)
    {
        hdsa_allocator_list_merge_right(self, &free_node, &free_footer, &node_after, &footer_after);
    }

    /**
     * Just as before, this if statement covers the case of self-head being
     * previous_node.
     */
    if ((ptrdiff_t)node_before == (ptrdiff_t)self->head)
    {
        hdsa_allocator_list_merge_left(self, &free_node, &free_footer, &self->head, &footer_before);
    }
    else if (((ptrdiff_t)node_before == (ptrdiff_t)previous_node) && ((ptrdiff_t)previous_node != (ptrdiff_t)self->head))
    {
        hdsa_allocator_list_merge_left(self, &free_node, &free_footer, &previous_node, &footer_before);
    }
    else if (footer_before->is_free)
    {
        hdsa_allocator_list_merge_left(self, &free_node, &free_footer, &node_before, &footer_before);
    }
}

/**
 * @brief Deallocates a used node if the user-provided pointer is a used node.
 * @param[in] self Allocator_List pointer, must not be null.
 * @param[in] ptr Pointer to the pointer being used byt the user. Both ptr and
 * *ptr must not be null. If the deallocation succeeds, then *ptr is set to
 * null.
 */
void hdsa_allocator_list_deallocate(hdsa_Allocator_List* const self, void** const ptr)
{
    puts("------------------------------ LIST DEALLOCATION ----------------------------------\n");

    hdsa_Allocator_List_Used* ptr_node = (hdsa_Allocator_List_Used*)0;
    hdsa_Allocator_List_Used* temp_used = (hdsa_Allocator_List_Used*)0;
    hdsa_Allocator_List_Used* temp_previous_used = (hdsa_Allocator_List_Used*)0;
    hdsa_Allocator_List_Free* free_node = (hdsa_Allocator_List_Free*)0;

    if (!ptr)
    {
        puts("The pointer to deallocate from the Allocator_List is null.\n");
        return;
    }

    if (!self->used_head)
    {
        puts("The allocator has 0 allocated nodes.\n");
    }

    ptr_node = (hdsa_Allocator_List_Used*)((ptrdiff_t)ptr - (ptrdiff_t)sizeof(hdsa_Allocator_List_Used));
    temp_previous_used = self->used_head;
    temp_used = self->used_head->next;

    // Find if ptr/used_node is really inside this allocator, and if so, where
    if ((ptrdiff_t)temp_previous_used != (ptrdiff_t)ptr_node)
    {
        while (temp_used)
        {
            if ((ptrdiff_t)temp_used == (ptrdiff_t)ptr_node)
            {
                break;
            }

            temp_previous_used = temp_previous_used->next;
            temp_used = temp_used->next;
        }

        if (!ptr_node)
        {
            return;
        }
    }
    else
    {
        temp_used = temp_previous_used;
        temp_previous_used = (hdsa_Allocator_List_Used*)0;
    }

    printf("allocation_amount before: %lld\n", self->allocation_amount);

    // Put the free_node in the same node ptr is
    free_node = (hdsa_Allocator_List_Free*)((ptrdiff_t)ptr - (ptrdiff_t)temp_used->padding);
    free_node->section_size = (temp_used->section_size + temp_used->padding) - (ptrdiff_t)sizeof(hdsa_Allocator_List_Free);
    free_node->next = (hdsa_Allocator_List_Free*)0;
    free_node->previous = (hdsa_Allocator_List_Free*)0;

    hdsa_Error dummy_error;
    if (!hdsa_allocator_list_used_list_remove(self, &temp_previous_used, &temp_used, &dummy_error))
    {
        return;
    }

    hdsa_allocator_list_free_list_add(self, free_node);

    printf("temp_used: %p\n", (void*)temp_used);
    printf("free_node: %p\n", (void*)free_node);
    printf("free_node->section_size: %lld\n", free_node->section_size);
    printf("used_total before deallocation: %lld\n", self->used_total);

    if (self->v_alloc.mode == HDSA_ALLOCATOR_VIRTUAL_STATIC)
    {
        self->v_alloc.last_block->used_block -= free_node->section_size;
    }

    self->used_total -= free_node->section_size;
    self->allocation_amount -= 1;
    hdsa_allocator_list_coalescence(self, free_node);
    *ptr = (void*)0;

    printf("allocation_amount after: %lld\n", self->allocation_amount);
    printf("used_total after deallocation: %lld\n", self->used_total);
}

/**
 * @brief In External mode it resets everything to 0 except
 * self->v_alloc.committed total, the placement policy and the buffer/memory
 * block. Then self->head is set to a node that covers the entire buffer (or
 * the rest of the memory block after its header in Static Mode).
 * @param[in] self Allocator_List pointer, must not be null.
 */
void hdsa_allocator_list_reset(hdsa_Allocator_List* const self)
{
    HDSA_BASIC_ASSERT((self), "The Allocator_List pointer is null.\n");

    printf(".............................. LIST RESET ..................................\n");

    hdsa_allocator_virtual_reset(&self->v_alloc);

    if (!self->v_alloc.last_block) // External mode always has a null last block
    {
        self->allocation_amount = 0;
        self->used_total = 0;
        self->allocated_offset = 0;
        self->head = (hdsa_Allocator_List_Free*)0;
        self->used_head = (hdsa_Allocator_List_Used*)0;

        // Setting up the first node
        self->head = (hdsa_Allocator_List_Free*)self->v_alloc.buffer;

        if (self->v_alloc.buffer)
        {
            hdsa_Allocator_List_Free* dummy_node = (hdsa_Allocator_List_Free*)0;
            hdsa_Error dummy_error;
            hdsa_allocator_list_create_free_node(self, &self->head, 1, &dummy_node, &dummy_error);
        }

        puts("The Allocator_List was empty or in External mode when trying to reset it. All the allocator data was set to 0 and a single free node was a assigned to self->head.\n");

        return;
    }

    self->head = (hdsa_Allocator_List_Free*)0;
    self->used_head = (hdsa_Allocator_List_Used*)0;
    self->v_alloc.last_block->used_block = (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block);
    self->used_total = self->v_alloc.last_block->used_block;

    self->allocated_offset = self->v_alloc.last_block->used_block;
    hdsa_Allocator_List_Free* dummy_node = (hdsa_Allocator_List_Free*)0;
    hdsa_Error dummy_error;
    hdsa_allocator_list_create_free_node(self, &self->head, 1, &dummy_node, &dummy_error);
    self->allocation_amount = 1;

    puts("The Allocator_List has been reset!\n");
}
