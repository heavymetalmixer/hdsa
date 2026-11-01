#ifndef HDSA_COMMON_H
#define HDSA_COMMON_H

#include <stdbool.h>
#include <stdint.h>
#include <limits.h>
#include <string.h>

#include "errors.h"
#include "asserts.h"

/**
 * @brief A function to return the single-dimensional array index from arr[x][y]
 * "arr_x_size" is the amount of rows in the array.
 *
 * @param[in] x First index of the "2D" array, must be 0 or positive.
 * @param[in] y Second index of the "2D" array, must be 0 or positive.
 * @param[in] arr_x_size Size of every row in the array (x), must be 0 or positive.
 */
ptrdiff_t hdsa_common_dual_to_single_index(ptrdiff_t x, ptrdiff_t y, ptrdiff_t arr_x_size);

/**
 * @brief A function to return the single-dimensional array index from arr[y][x]
 * "arr_x_size" is the amount of rows in the array.
 *
 * This one is inverse order for code that needs that representation.
 * @param[in] x First index of the "2D" array, must be 0 or positive.
 * @param[in] y Second index of the "2D" array, must be 0 or positive.
 * @param[in] arr_x_size Size of every row in the array (x), must be 0 or positive.
 */
ptrdiff_t hdsa_common_dual_to_single_index_inverse(ptrdiff_t x, ptrdiff_t y, ptrdiff_t arr_x_size);

/**
 * @brief It checks if pointer is somewhere inside buffer.
 * @param[in] pointer Address to check inside buffer's range. Must not
 * be null.
 * @param[in] buffer_capacity Only positive values allowed from 1.
 * @param[in] buffer Must not be null.
 * @returns True if pointer is inside buffer's range, false otherwise
 * or if any of the buffers is null.
 */
bool hdsa_common_is_pointer_inside_buffer(const void* const pointer, ptrdiff_t buffer_capacity, const void* const buffer);

/**
 * @brief It checks if x is a power of 2. False for numbers smaller than 1,
 * even negative numbers.
 * @param[in] x Number to check, any value representable by ptrdiff_t is valid.
 * @returns True is x is a power of 2, false otherwise.
 */
bool hdsa_common_is_power_of_two(ptrdiff_t x);

/**
 * @brief If x isn't some power of 2, then increase it until it becomes a power of 2.
 * @param[in] requested_size Number to round up to a power of 2, any value
 * representable by ptrdiff_t is valid.
 * @returns A power of 2. If requested_size is already a power of 2 then the same
 * requested_size s returned. 1 when the requested_size is less than 2 and that includes
 * negative numbers.
 */
ptrdiff_t hdsa_common_round_pow_two(ptrdiff_t requested_size);

/**
 * @brief If initial_number isn't a multiple of "base", then increase it until it
 * becomes a multiple of base.
 * @param[in] initial_number Number to round up, must be 0 or bigger.
 * @param[in] base initial_number will be rounded up to a multiple of it. It
 * must be at least 1.
 * @returns A multiple of base that is bigger or equal to inital_number. In the
 * case initial_number is 0 the result value is 0.
 */
ptrdiff_t hdsa_common_round_to_multiple(ptrdiff_t initial_number, ptrdiff_t base);

/**
 * @brief It checks if "ptr" is correctly aligned to "alignment" or not.
 * @param[in] ptr Pointer to check the alignment for, must not be null.
 * @param[in] alignment Alignment value to use on ptr, must be 1 or bigger.
 * @returns True if ptr is aligned to alignment, false otherwise.
 */
bool hdsa_common_is_aligned(const void* const ptr, const ptrdiff_t alignment);

/**
 * @brief Aligns "ptr" to "alignment" to prevent slowdowns, crashes or UB
 * caused by un-aligned memory access by the CPU.
 * @param[in] ptr Pointer to align. The address inside of it must be
 * representable by ptrdiff_t and it must not be null.
 * @param[in] alignment Alignment value to use on ptr, if the value is smaller
 * than 1 then alignment will be changed to 1. If the value is bigger than 2
 * alignment will be changed to a power of 2.
 * @returns A new aligned address for ptr as a ptrdiff_t value. It will be 1 or
 * bigger.
 */
ptrdiff_t hdsa_common_align_forward(ptrdiff_t ptr, ptrdiff_t alignment);

/**
 * @brief Aligns "ptr" to "alignment" (while taking into account "header_size")
 * to prevent slowdowns, crashes or UB caused by un-aligned memory access by
 * the CPU and returns the necessary padding amount for it.
 * @param[in] ptr Pointer to align. The address inside of it must be
 * representable by ptrdiff_t and it must not be null.
 * @param[in] header_size Size of a header that will be used in the allocation
 * as well. This size will be used as the minimum padding, and it must be 1 or
 * bigger.
 * @param[in] alignment Alignment value to use on ptr, must be 1 or bigger.
 * @returns A new aligned address for ptr as a ptrdiff_t value. It will be at
 * least as big as header_size.
 */
ptrdiff_t hdsa_common_calc_padding_with_header(ptrdiff_t ptr, ptrdiff_t header_size, ptrdiff_t allocation_alignment);

#endif // HDSA_COMMON_H
