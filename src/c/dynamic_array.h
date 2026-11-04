#ifndef HDSA_DYNAMIC_ARRAY_H
#define HDSA_DYNAMIC_ARRAY_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>

#include "common.h"
#include "allocator_interface.h"
#include "virtual_alloc.h"

/**
 * @brief A classic dynamic array. It allocates on the heap in some of its
 * functions when it requieres a new bigger buffer. The functions that
 * may allocate take an hdsa_Error pointer as an output parameter.
 *
 * It doesn't use macros to ensure type safety, it uses unsigned char*
 * and void* type puning instead.
 */
typedef struct hdsa_Dynamic_Array
{
    unsigned char* buffer;
    hdsa_Allocator_Virtual* v_alloc;
    ptrdiff_t buffer_size;
    ptrdiff_t buffer_capacity;
    ptrdiff_t type_size;
    ptrdiff_t type_alignment;
} hdsa_Dynamic_Array;

/**
 * @brief It initializes the Dynamic_Array with a given Allocator Virtual
 * pointer, and three numbers to determine the size of the buffer and how
 * big each "object" inside the array will be in bytes.
 *
 * @param[in] self Pointer to the dynamic array struct, must not be null.
 * @param[in] virtual_alloc An Allocator Virtual pointer to use internally, it
 * must be in Static Mode.
 * @param[in] object_amount Amount of "blocks" in array buffer will be divided
 * in. For the user this would be the "buffer's capacity" even though it's not
 * because the buffer is managed in bytes. It must be positive.
 * If object_amount is 0 the allocation is bypassed and the buffer is set to null.
 * @param[in] type_size Size of the object the user wants to store inside each
 * block of the array. It must be bigger than 0.
 * @param[in] type_alignment Alignment of the object to store inside the array.
 * It must be bigger than 0. If the value passed to it isn't a power of 2, the
 * function will round it up to the next power of 2.
 */
bool hdsa_dynamic_array_init
(
    hdsa_Dynamic_Array* const self,
    hdsa_Allocator_Virtual* const virtual_alloc,
    ptrdiff_t object_amount,
    ptrdiff_t type_size,
    ptrdiff_t type_alignment,
    hdsa_Error* const error_out
);

/**
 * @returns Current amount of objects of size of type T the array
 * holds, not the current buffer_size.
 */
ptrdiff_t hdsa_dynamic_array_size(const hdsa_Dynamic_Array* const self);

/**
 * @returns Maximum amount of objects of size of type T the array
 * can hold, not the current buffer_capacity.
 */
ptrdiff_t hdsa_dynamic_array_capacity(const hdsa_Dynamic_Array* const self);

/**
 * @brief It returns the index to the last T size object in the array, not the
 * last real index in the buffer.
 *
 * If self->buffer_size is 0 then -1 is returned.
 */
ptrdiff_t hdsa_dynamic_array_last_index(const hdsa_Dynamic_Array* const self);

/**
 * @brief It simply resets "buffer_size" to 0 so calling push() on this array
 * will be done from the beginning of the buffer.
 */
void hdsa_dynamic_array_reset_array(hdsa_Dynamic_Array* const self);

/**
 * @brief It fills with zeroes the data from the index "position" (T sized)
 * n indexes above, with n being "object_amount".
 *
 * It doesn't use memset to avoid the compiler optimizing it away.
 *
 * If the array is empty (self->buffer_size being 0) the function does nothing.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] position The first position in the array to fill with 0s, must
 * be positive or 0.
 * @param[in] object_amount Amount of T-sized objects to fill with 0s in the
 * array. It must be at least 1, and if it goes above the size of the array
 * the whole self->buffer is filled with 0s.
 */
void hdsa_dynamic_array_zero_fill(hdsa_Dynamic_Array* const self, ptrdiff_t position, ptrdiff_t object_amount);

/**
 * @brief This one fills the entire buffer until self->buffer_size with zeroes,
 * an expensive operation and just like other 2 "zero_fill" functions it's
 * completely optional.
 *
 * Bear in mind: self->buffer_size isn't modified with this function as the
 * data after beginning won't be moved.
 *
 * It doesn't use memset to avoid the compiler optimizing it away.
 *
 * If the array is empty (self->buffer_size being 0) the function does nothing.
 */
void hdsa_dynamic_array_zero_fill_all(hdsa_Dynamic_Array* const self);

/**
 * @brief Allocates more memory for the Dynamic Array as long as the allocator
 * can get more memory.
 *
 * WARNING: Do not use this function on a Dynamic Array unless init() was
 * already used on it beforehand.
 * @param[in] object_amount Number of T-sized objects you want more memory for.
 * It must be bigger than 1. If it is 0 then the function does nothing.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeded, false otherwise.
 */
bool hdsa_dynamic_array_allocate(hdsa_Dynamic_Array* const self, ptrdiff_t object_amount, hdsa_Error* const error_out);

/**
 * @brief It deallocates all the memory in the allocator that the
 * Dynamic Array has and sets all the array's data to 0/null.
 * @param[in] self The dynamic array pointer, must not be null.
 */
void hdsa_dynamic_array_deallocate_all(hdsa_Dynamic_Array* const self);

/**
 * @brief It reduces the buffer_capacity and even the amount of memory
 * allocated (if it results in less virtual memory pages) to fit the total
 * amount of T-size objects request by the user.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] object_amount The new smaller number of T-size objects the array
 * is allowed to have. If it is equal or bigger than self->buffer_capacity this
 * function does nothing. If it is smaller than self->buffer_size then
 * object_amount will be the new self->buffer_size.
 *
 * If after being rounded up to hdsa_virtual_page_size() it results in a
 * numbers of committed pages smaller than what the array currently holds, a
 * new buffer with this new number of pages is allocated, all the data inside
 * the current buffer is copied to the new one, the current one is deallocated
 * and the new one becomes the current buffer.
 *
 * This new buffer will hold the same amount of reserved memory as the
 * previous one.
 *
 * If it's 0 then all the memory inside the Dynamic Array is deallocated.
 * @returns False if an allocation fails, true otherwise.
 */
bool hdsa_dynamic_array_shrink(hdsa_Dynamic_Array* const self, ptrdiff_t object_amount, hdsa_Error* const error_out);

/**
 * @brief It copies one hdsa_Dynamic_Array into another, including all the data
 * in the buffer, AKA a "deep copy". The only thing not copied is the allocator
 * interface.
 *
 * It may allocate a new buffer for the second hdsa_Dynamic_Array if
 * the current one isn't big enough to copy all the data
 * from the first array.
 *
 * If both arrays are the same this function does nothing.
 * @param[in] self The dynamic array pointer, must not be null. This one
 * holds to data to copied.
 * @param[out] destination_out The second array and the one that will receive
 * the data from self. Must have a buffer_capacity at least as big as
 * self->buffer_size. If destination_out and self are the same the function is
 * a no-op.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeds or isn't needed, false if the
 * allocation fails but destination_out->buffer isn't set to null.
 */
bool hdsa_dynamic_array_copy_array(const hdsa_Dynamic_Array* const self, hdsa_Dynamic_Array* const destination_out, hdsa_Error* const error_out);

/**
 * @brief It copies "data" at the end of the array, if there's space for it.
 *
 * Given that there's no "type safety" in this hdsa_Dynamic_Array, copying the
 * new data must be done a single byte at a time, and incrementing the buffer
 * size at the end of the function must be done with the type's size.
 *
 * Passing data that has a bigger size than self->type_size only copies the
 * bytes up to self->type_size. Also, if the data being pushed has a different
 * alignment than self->type_alignment UB will happen.
 *
 * It may allocate a new buffer at least twice as big if the current one is
 * full.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] object Data to copy at the end of the array.
 * @param[out] index_out The pointer to the index to the just-pushed T sized
 * object (nth - 1 value). Must not be null.
 * @param[out] data_out Output pointer to a ponter that will contain the
 * address to the data after it's being added to the array, must not be null.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeded or wasn't needed, false if the
 * allocation failed.
 */
bool hdsa_dynamic_array_push
(
    hdsa_Dynamic_Array* const self,
    const void* const object,
    ptrdiff_t* const index_out,
    void** const data_out,
    hdsa_Error* const error_out
);

/**
 * @brief It decreases the buffer_size byt the type_size so the latest "block
 * of memory" can be used to push data on top of it again. If
 * self->buffer_size is 0 then this function does nothing.
 * @returns The index to the now last T sized object after the pop operation.
 */
void hdsa_dynamic_array_pop(hdsa_Dynamic_Array* const self);

/**
 * @brief It inserts an array of "n" T sized objects in the middle of the
 * array, by first copying all the existing data after "position" to the
 * index after "object_array_size" and then copying "object" in the
 * provided position.
 *
 * A very slow operation, don't do it unless you really need to.
 *
 * Chances are a different kind of data structure is better fit
 * for fast insertion of multiple objects than this one.
 *
 * Pass an array of a different type other than T and you could get UB.
 *
 * It may allocate a new and bigger buffer is the current one is full.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] object_array Data array to copy at the offset specified by
 * "position".
 * @param[in] position Index of size T where object goes. It must be from 0 to
 * (hdsa_dynamic_array_last_index + 1).
 * @param[in] object_array_size Size of the data array to copy, must be bigger
 * than 0.
 * @param[out] data_out Output pointer to a ponter that will contain the
 * address to the data after it's being added to the array, must not be null.
 * @param[out] error_out Pointer to the error struct, must not be null.
 * @returns True if the allocation succeeded or wasn't needed, false if the
 * allocation failed.
 */
bool hdsa_dynamic_array_insert
(
    hdsa_Dynamic_Array* const self,
    const void* const object_array,
    ptrdiff_t position,
    ptrdiff_t object_array_size,
    void** const data_out,
    hdsa_Error* const error_out
);

/**
 * @brief It returns a pointer to the nth T object on the index "position",
 * similar to how the "subscript operator" works on static arrays (arr[x], x
 * being "position").
 *
 * Now, this index is about T objects in the array, not the real index in the
 * buffer, so it becomes easier for the user to think about the types being
 * stored rather than raw bytes.
 *
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] position Offset calculated win the size of the T object in mind,
 * it goes from 0 to (n - 1).  Other functions like push, pop and
 * last_index give those indexes to the user. It must be from 0 to
 * hdsa_dynamic_array_last_index.
 *
 * @returns The pointer to the initial address where that offset lives.
 * If the array is empty or the position is out of bounds null is returned.
 */
void* hdsa_dynamic_array_object_at(const hdsa_Dynamic_Array* const self, ptrdiff_t position);

/**
 * @brief It overwrites the data at "position" with "object", but "position"
 * must be an existing position in the array already, as this function doesn't
 * increase the size of the array.
 *
 * If the array, its buffer or "object" are null this function does nothing.
 * The same applies if "position" is out of bounds.
 * @param[in] self Pointer to the Dynamic array, must not be null.
 * @param[in] object Data to copy.
 * @param[in] position T-size position where "object" will overwrite the
 * existing data. Must be from 0 to the last index in the array.
 */
void hdsa_dynamic_array_assign(hdsa_Dynamic_Array* const self, void* object, ptrdiff_t position);

/**
 * @brief It overwrites the data from "position" with "object", but "position"
 * must be an existing position in the array already. Unlike the "single"
 * variant of this function, "multiple" can expand the Dynamic Array if the
 * incoming other array to assign goes beyond the size of the Dynamic Array.
 *
 * If the array, its buffer or "object" are null this function does nothing.
 * The same applies if "position" is out of bounds or "object_array_size" is
 * less than 1.
 * @param[in] self Pointer to the Dynamic array, must not be null.
 * @param[in] object_array Array of data to copy.
 * @param[in] object_array_size Size of the incoming array to copy in terms of
 * T-size, not bytes. Must be at least 1.
 * @param[in] position The position where "object" will overwrite the existing
 * data. Must be from 0 to the last index in the array.
 */
bool hdsa_dynamic_array_assign_multiple
(
    hdsa_Dynamic_Array* const self,
    void* object_array,
    ptrdiff_t object_array_size,
    ptrdiff_t position,
    hdsa_Error* const error_out
);

/**
 * @brief It deletes a certain amount of T sized objects in the middle of the
 * array by copying the existing data after "position" n indexes above, with
 * "n" being "object_amount", to the previous indexes by that same distance.
 * If this distance is less than 1 or bigger than the size of the array this
 * function does nothing.
 *
 * After this the last "free spots" (how many of those are determined by
 * object_amount) are filled with 0s.
 *
 * Quite an expensive operation so it's not recommended to use it unless you
 * really need to.
 *
 * If self->buffer_size is 0 this function does nothing as well.
 * @param[in] self The dynamic array pointer, must not be null.
 * @param[in] position The first position in the array for the data to erase,
 * must be within the array size starting from 0.
 * @param[in] object_amount Amount of T sized object to delete, it must be at
 * least 1 and if it goes beyond the size of the array, all the data inside
 * the array will be erased.
 */
void hdsa_dynamic_array_erase(hdsa_Dynamic_Array* const self, ptrdiff_t beginning, ptrdiff_t end);

#endif // HDSA_DYNAMIC_ARRAY_H
