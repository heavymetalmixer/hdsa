#ifndef HDSA_ALLOCATOR_FREE_LIST_H
#define HDSA_ALLOCATOR_FREE_LIST_H

#include <stdio.h>

#include "common.h"
#include "allocator_interface.h"
#include "virtual_alloc.h"

typedef struct hdsa_Allocator_Free_List_Header
{
    ptrdiff_t section_size;
    ptrdiff_t padding;
} hdsa_Allocator_Free_List_Header;

typedef struct hdsa_Allocator_Free_List_Node hdsa_Allocator_Free_List_Node;

typedef struct hdsa_Allocator_Free_List_Node
{
    ptrdiff_t section_size; // section_size includes the size of hdsa_Allocator_Free_List_Node
    hdsa_Allocator_Free_List_Node* next;
} hdsa_Allocator_Free_List_Node;

typedef enum hdsa_Allocator_Placement_Policy
{
    HDSA_ALLOCATOR_FIND_FIRST = 0,
    HDSA_ALLOCATOR_FIND_BEST
} hdsa_Allocator_Placement_Policy;

typedef struct hdsa_Allocator_Free_List
{
    hdsa_Allocator_Virtual v_alloc; // v_alloc.committed_total is used as the buffer length
    ptrdiff_t used_bytes;
    hdsa_Allocator_Free_List_Node* head;
    hdsa_Allocator_Placement_Policy policy;
    ptrdiff_t allocation_amount;
    ptrdiff_t used_total;
} hdsa_Allocator_Free_List;


#endif // HDSA_ALLOCATOR_FREE_LIST_H
