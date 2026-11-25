#include "sysinfoapi.h"
#include "c/allocator_interface.h"
#include "c/virtual_alloc.h"
#include "c/arena_alloc.h"
#include "c/stack_alloc.h"
#include "c/list_alloc.h"
#include "c/dynamic_array.h"



#define MAP_SIZE_X (4)
#define MAP_SIZE_Y (3)

/* A function to return the single-dimensional array index from arr[x][y] */
int map_get(int x, int y)
{
    return (x * MAP_SIZE_X) + y;
}

/* A function to return the single-dimensional array index from arr[x][y] */
int map_get_inverse(int x, int y)
{
    return x + (y * MAP_SIZE_X);
}

typedef struct Proginfo
{
    const char* name;
    const char* author;
    int version;
} Proginfo;

#if 0
#define GENERIC_DYNARRAY(T) T* data; \
size_t size; \
size_t capacity

#define GENERIC_ADD(return_type, T, U) \
return (return_type)((T)a + (U)b)

typedef struct DynInt
{
    GENERIC_DYNARRAY(int);
} DynInt;

int add_generic(int a, int b)
{
    GENERIC_ADD(float, float, float);
}
#endif

// ######################################################################################################

/**
 * To investigate and implement:
 * 1) Indexes instead of pointers in data structures, even for "next" and "previous" y linked lists.
 * 2) Nil structs as first data element in data structures.
 * 3) Intrusive node-based data structures.
 * 4) Fat structs.
 * 5) Replace unsigned sizes with signed ones. DONE
 * 6) Make a C callback interface with type erasure to replace the OOP one. DONE
 */

//##############################################################################################################################################################


#if 0
typedef struct IntruList
{
    int64_t data;
    size_t id;
    size_t head;
    size_t previous;
    size_t next;
} IntruList;

void int_list_init(IntruList* intru, int64_t data)
{
    static IntruList intru_nil = { 0, 0, 0, 0, 0 };

    intru->data = data;
    intru->id = 1;
    intru->head = 0;
    intru->previous = 0;
    intru->next = 0;

    intru_nil.next = 1;
    intru_nil.previous = 1;
}
#endif

// ######################################################################################################

typedef struct temp
{
    int32_t a;
    int16_t b;
} temp;


int main(void)
{
// #if 0
    #define OBJECT_AMOUNT 2

    // Using aligned_alloc (C11) for now instead of a custom allocator.
    // In GCC the functions are aligned_alloc(size_t alignment, size_t size) and free(void* memory)
    // In MinGW "_aligned_malloc(size_t s, size_t alignment)" and "_aligned_free(void* memory)" must be used instead
    // void* memory = _aligned_malloc(OBJECT_AMOUNT, _Alignof(int));

    // printf("Buffer from aligned_alloc: %p\n", memory);

    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);
    printf("Size of SYSTEM_INFO: %lld\n", (ptrdiff_t)sizeof(SYSTEM_INFO));
    printf("Page size: %lld\n", (ptrdiff_t)sys_info.dwPageSize);
    printf("Granularity size: %lld\n", (ptrdiff_t)sys_info.dwAllocationGranularity);
    printf("Round up test: %lld\n\n\n", hdsa_common_round_to_multiple(65535, (ptrdiff_t)sys_info.dwAllocationGranularity));

    hdsa_Error error_struct;
    error_struct.code = HDSA_ERROR_SUCCESS;


    hdsa_Allocator_Placement_Policy pol = HDSA_ALLOCATOR_FIND_FIRST;
    hdsa_Allocator_List list;

    if (!hdsa_allocator_list_init_static(&list, 1, 1, pol, &error_struct))
    {
        puts(error_struct.description);
        exit(1);
    }



    printf("list's head: %p\n", (void*)list.head);
    printf("list's used_head: %p\n", (void*)list.used_head);
    printf("list's policy: %lld\n", (ptrdiff_t)list.policy);
    printf("list's allocated_off_set: %lld\n", list.allocated_offset);
    printf("list's allocation_amount: %lld\n", list.allocation_amount);
    printf("list's used_total: %lld\n", list.used_total);




    // temp* x;

    // if (!hdsa_allocator_list_allocate(&list, (ptrdiff_t)sizeof(temp), (ptrdiff_t)_Alignof(temp), (void**)&x, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // printf("Is x aligned? %s\n", ((hdsa_common_is_aligned((void*)x, (ptrdiff_t)_Alignof(temp))) ? "Yes!" :  "No!"));
    // printf("x: %p\n", (void*)x);
    // x->a = 3;
    // x->b = 5;
    // printf("x->a: %d\n", x->a);
    // printf("x->b: %d\n\n\n", x->b);

    // hdsa_allocator_list_deallocate(&list, (void*)x);
    // x = (temp*)0;


    // char* uc;

    // if (!hdsa_allocator_list_allocate(&list, (ptrdiff_t)sizeof(char) * 5, (ptrdiff_t)_Alignof(char), (void**)&uc, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // printf("uc: %p\n", (void*)uc);
    // printf("Is uc aligned? %s\n", ((hdsa_common_is_aligned((void*)uc, 8))) ? "Yes!" :  "No!");
    // uc[0] = 'x';
    // uc[1] = 'y';
    // uc[2] = 'z';
    // uc[3] = '1';
    // uc[4] = '2';
    // printf("uc[0]: %c\n", uc[0]);
    // printf("uc[1]: %c\n", uc[1]);
    // printf("uc[2]: %c\n", uc[2]);
    // printf("uc[3]: %c\n", uc[3]);
    // printf("uc[4]: %c\n\n\n", uc[4]);



    // int32_t* star;

    // if (!hdsa_allocator_list_allocate(&list, (ptrdiff_t)sizeof(int32_t), (ptrdiff_t)_Alignof(int32_t), (void**)&star, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // printf("star: %p\n", (void*)star);
    // printf("Is star aligned? %s\n", ((hdsa_common_is_aligned((void*)star, (ptrdiff_t)_Alignof(int32_t)))) ? "Yes!" :  "No!");
    // *star = 10;
    // printf("*star: %d\n\n\n", *star);

    // hdsa_allocator_list_deallocate(&list, (void*)uc);
    // hdsa_allocator_list_deallocate(&list, (void*)star);
    // uc = (char*)0;
    // star = (int32_t*)0;



    // char* uc2;

    // if (!hdsa_allocator_list_allocate(&list, (ptrdiff_t)sizeof(char) * 5, (ptrdiff_t)_Alignof(char), (void**)&uc2, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // printf("uc2: %p\n", (void*)uc2);
    // printf("Is uc2 aligned? %s\n", ((hdsa_common_is_aligned((void*)uc2, 8))) ? "Yes!" :  "No!");
    // uc2[0] = 'a';
    // uc2[1] = 'b';
    // uc2[2] = 'c';
    // uc2[3] = 'd';
    // uc2[4] = 'e';
    // printf("uc2[0]: %c\n", uc2[0]);
    // printf("uc2[1]: %c\n", uc2[1]);
    // printf("uc2[2]: %c\n", uc2[2]);
    // printf("uc2[3]: %c\n", uc2[3]);
    // printf("uc2[4]: %c\n\n\n", uc2[4]);



    // short* us;

    // if (!hdsa_allocator_list_allocate(&list, (ptrdiff_t)sizeof(short) * 3, (ptrdiff_t)_Alignof(short), (void**)&us, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // printf("us: %p\n", (void*)us);
    // printf("Is us aligned? %s\n", ((hdsa_common_is_aligned((void*)us, (ptrdiff_t)_Alignof(short)))) ? "Yes!" :  "No!");
    // us[0] = 12;
    // us[1] = 14;
    // us[2] = 16;
    // printf("us[0]: %d\n", us[0]);
    // printf("us[1]: %d\n", us[1]);
    // printf("us[2]: %d\n\n\n", us[2]);

    // hdsa_allocator_list_deallocate(&list, (void*)uc2);
    // hdsa_allocator_list_deallocate(&list, (void*)us);



    temp* temp_out;

    // for (ptrdiff_t i = 0; i < (hdsa_virtual_page_size() - ((ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block)));  i += 1)
    for (ptrdiff_t i = 0; i < 2048;  i += 1)
    {
        if (!hdsa_allocator_list_allocate(&list, (ptrdiff_t)sizeof(temp), (ptrdiff_t)_Alignof(temp), (void**)&temp_out, &error_struct))
        {
            puts(error_struct.description);
            exit(1);
        }

        // printf("i: %lld\n", i);

        // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp);
        // printf("Temp object %lld: %d, %d\n\n\n", i + 10, t->a, t->b);
    }

    // hdsa_allocator_stack_deallocate(&stack);
    // hdsa_allocator_stack_deallocate(&stack);
    // hdsa_allocator_stack_deallocate(&stack);
    // hdsa_allocator_stack_give_back_memory(&stack);

    // temp* temp_buff;
    // hdsa_allocator_list_resize(&list, (void*)temp_out, (ptrdiff_t)sizeof(short) * 3, (ptrdiff_t)sizeof(short) * 16, (ptrdiff_t)_Alignof(short), (void**)&temp_buff, &error_struct);


////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////


    // hdsa_Dynamic_Array dynarr;
    // hdsa_dynamic_array_init(&dynarr, &vir, 10, (ptrdiff_t)sizeof(temp), (ptrdiff_t)_Alignof(temp), &error_struct);

    // printf("dynarr's Memory Block address: %lld\n\n", (ptrdiff_t)dynarr.v_alloc->last_block);
    // printf("dynarr's buffer address: %lld\n\n", (ptrdiff_t)dynarr.buffer);

    // temp t1 = { 2, 3 };
    // temp t2 = { 8, 5 };
    // temp t3 = { 4, 1 };
    // temp t4 = { 7, 7 };
    // temp t5 = { 3, 9 };
    // temp t6 = { 23, 41 };
    // temp t7 = { 11, 99 };
    // temp t8 = { 236, 214 };
    // temp t9 = { 623, 22 };
    // temp t10 = { 63214, 46 };


    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_1 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t1, &index_1, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // temp* t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_1);
    // printf("Temp object 1: %d, %d\n\n\n", t->a, t->b);


    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_2 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t2, &index_2, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_2);
    // printf("Temp object 2: %d, %d\n\n\n", t->a, t->b);


    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_3 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t3, &index_3, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_3);
    // printf("Temp object 3: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_4 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t4, &index_4, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_4);
    // printf("Temp object 4: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_5 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t5, &index_5, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_5);
    // printf("Temp object 5: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_6 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t6, &index_6, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_6);
    // printf("Temp object 6: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_7 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t7, &index_7, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_7);
    // printf("Temp object 7: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_8 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t8, &index_8, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_8);
    // printf("Temp object 8: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_9 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t9, &index_9, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_9);
    // printf("Temp object 9: %d, %d\n\n\n", t->a, t->b);




    // ///////////////////////////////////////////////////////////////////////////
    // ptrdiff_t index_10 = -1;

    // if (!hdsa_dynamic_array_push(&dynarr, (void*)&t10, &index_10, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_10);
    // printf("Temp object 10: %d, %d\n\n\n", t->a, t->b);



    // ///////////////////////////////////////////////////////////////////////////

    // // hdsa_dynamic_array_zero_fill_single(&dynarr, 5);
    // // hdsa_dynamic_array_zero_fill_array(&dynarr);

    // temp t11 = { 753, 951 };
    // temp t12 = { 456, 123 };
    // temp t13 = { 789, 11 };

    // temp tarray[3];
    // tarray[0] = t11;
    // tarray[1] = t12;
    // tarray[2] = t13;

    // ptrdiff_t index_temp = -1;

    // printf("Current t10's address: %p\n\n", hdsa_dynamic_array_object_at(&dynarr, index_10));

    // ptrdiff_t allocation_size = (hdsa_virtual_granularity_size() - (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block) - ((ptrdiff_t)sizeof(temp) * 13)) / dynarr.type_size;



    // // if (!hdsa_dynamic_array_shrink(&dynarr, dynarr.buffer_size / dynarr.type_size, &error_struct))


    // if (!hdsa_dynamic_array_allocate(&dynarr, allocation_size, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // if (!hdsa_dynamic_array_shrink(&dynarr, (dynarr.buffer_capacity - hdsa_virtual_page_size()) / dynarr.type_size, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }


    // // for (ptrdiff_t i = 0; i < ((hdsa_virtual_granularity_size()) - (ptrdiff_t)sizeof(hdsa_Allocator_Memory_Block) - ((ptrdiff_t)sizeof(temp) * 10));  i += (ptrdiff_t)sizeof(temp))
    // // {
    // //     if (!hdsa_dynamic_array_push(&dynarr, (void*)&t1, &index_temp, &error_struct))
    // //     {
    // //         puts(error_struct.description);
    // //         exit(1);
    // //     }

    // //     // printf("i: %lld\n", i);

    // //     // t = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp);
    // //     // printf("Temp object %lld: %d, %d\n\n\n", i + 10, t->a, t->b);
    // // }

    // if (!hdsa_dynamic_array_insert_multiple(&dynarr, (void*)&tarray, 0, 3, &error_struct))
    // {
    //     puts(error_struct.description);
    //     exit(1);
    // }

    // // hdsa_dynamic_array_zero_fill_array(&dynarr);

    // // printf("dynarr's new Memory Block address: %lld\n\n", (ptrdiff_t)dynarr.v_alloc->last_block);
    // // printf("dynarr's new buffer address: %lld\n\n", (ptrdiff_t)dynarr.buffer);

    // // index_temp = hdsa_dynamic_array_last_index(&dynarr);
    // index_temp = 0;

    // temp* t_insert_1 = (temp*)0;
    // temp* t_insert_2 = (temp*)0;
    // temp* t_insert_3 = (temp*)0;
    // temp* t_insert_4 = (temp*)0;
    // temp* t_insert_5 = (temp*)0;
    // temp* t_insert_6 = (temp*)0;
    // temp* t_insert_7 = (temp*)0;
    // temp* t_insert_8 = (temp*)0;
    // temp* t_insert_9 = (temp*)0;
    // temp* t_insert_10 = (temp*)0;
    // temp* t_insert_11 = (temp*)0;
    // temp* t_insert_12 = (temp*)0;
    // temp* t_insert_13 = (temp*)0;

    // temp* t_insert_14 = (temp*)0;
    // temp* t_insert_15 = (temp*)0;

    // // t_insert_1 = object_at(&dynarr, temp, index_temp);

    // t_insert_1 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp);
    // t_insert_2 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 1);
    // t_insert_3 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 2);
    // t_insert_4 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 3);
    // t_insert_5 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 4);
    // t_insert_6 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 5);
    // t_insert_7 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 6);
    // t_insert_8 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 7);
    // t_insert_9 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 8);
    // t_insert_10 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 9);
    // t_insert_11 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 10);
    // t_insert_12 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 11);
    // t_insert_13 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 12);

    // temp assign_test = { 333, 777 };
    // temp assign_test2 = { 2236, 2366 };
    // temp assign_test_arr[2];
    // assign_test_arr[0] = assign_test;
    // assign_test_arr[1] = assign_test2;

    // hdsa_dynamic_array_assign(&dynarr, (void*)&assign_test, index_temp + 11);
    // hdsa_dynamic_array_assign_multiple(&dynarr, (void*)&assign_test_arr, 2, index_temp + 12, &error_struct);

    // t_insert_14 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 12);
    // t_insert_15 = (temp*)hdsa_dynamic_array_object_at(&dynarr, index_temp + 13);

    // hdsa_dynamic_array_erase_multiple(&dynarr, hdsa_dynamic_array_size(&dynarr) - 1, 3);

    // printf("First 13 objects: \n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n%d, %d\n\n\n", t_insert_1->a, t_insert_1->b, t_insert_2->a, t_insert_2->b, t_insert_3->a, t_insert_3->b, t_insert_4->a, t_insert_4->b, t_insert_5->a, t_insert_5->b, t_insert_6->a, t_insert_6->b, t_insert_7->a, t_insert_7->b, t_insert_8->a, t_insert_8->b, t_insert_9->a, t_insert_9->b, t_insert_10->a, t_insert_10->b, t_insert_11->a, t_insert_11->b, t_insert_12->a, t_insert_12->b, t_insert_13->a, t_insert_13->b);

    // printf("2 last objects: \n\n%d, %d\n\n%d, %d\n\n", t_insert_14->a, t_insert_14->b, t_insert_15->a, t_insert_15->b);




    int d;
    scanf("%d", &d);

    #undef OBJECT_AMOUNT
    #undef MAP_SIZE_Y
    #undef MAP_SIZE_X


    return 0;
}
