#include "common.h"

/**
 * @brief A function to return the single-dimensional array index from arr[x][y]
 * "arr_x_size" is the amount of rows in the array.
 *
 * @param[in] x First index of the "2D" array, must be 0 or positive.
 * @param[in] y Second index of the "2D" array, must be 0 or positive.
 * @param[in] arr_x_size Size of every row in the array (x), must be 0 or positive.
 */
ptrdiff_t hdsa_common_dual_to_single_index(ptrdiff_t x, ptrdiff_t y, ptrdiff_t arr_x_size)
{
    HDSA_BASIC_ASSERT((x > -1), "Negative values for x not allowed for array indexes.\n");
    HDSA_BASIC_ASSERT((y > -1), "Negative values for y not allowed for array indexes.\n");
    HDSA_BASIC_ASSERT((arr_x_size > -1), "Negative values for arr_x_size not allowed for array sizes.\n");
    HDSA_BASIC_ASSERT((x < PTRDIFF_MAX / arr_x_size), "(x * arr_x_size) went above the limits of ptrdiff_t.\n");
    HDSA_BASIC_ASSERT(((x * arr_x_size) < PTRDIFF_MAX - y), "((x * arr_x_size) + y) went above the limits of ptrdiff_t.\n");

    return (x * arr_x_size) + y;
}

/**
 * @brief A function to return the single-dimensional array index from arr[y][x]
 * "arr_x_size" is the amount of rows in the array.
 *
 * This one is inverse order for code that needs that representation.
 * @param[in] x First index of the "2D" array, must be 0 or positive.
 * @param[in] y Second index of the "2D" array, must be 0 or positive.
 * @param[in] arr_x_size Size of every row in the array (x), must be 0 or positive.
 */
ptrdiff_t hdsa_common_dual_to_single_index_inverse(ptrdiff_t x, ptrdiff_t y, ptrdiff_t arr_x_size)
{
    HDSA_BASIC_ASSERT((x > -1), "Negative values for x not allowed for array indexes.\n");
    HDSA_BASIC_ASSERT((y > -1), "Negative values for y not allowed for array indexes.\n");
    HDSA_BASIC_ASSERT((arr_x_size > -1), "Negative values for arr_x_size not allowed for array sizes.\n");
    HDSA_BASIC_ASSERT((y < PTRDIFF_MAX / arr_x_size), "(y * arr_x_size) went above the limits of ptrdiff_t.\n");
    HDSA_BASIC_ASSERT(((y * arr_x_size) < PTRDIFF_MAX - x), "((y * arr_x_size) + x) went above the limits of ptrdiff_t.\n");

    return x + (y * arr_x_size);
}

/**
 * @brief It checks if pointer is somewhere inside buffer.
 * @param[in] pointer Address to check inside buffer's range. Must not
 * be null.
 * @param[in] buffer_capacity Only positive values allowed from 1.
 * @param[in] buffer Must not be null.
 * @returns True if pointer is inside buffer's range, false otherwise
 * or if any of the buffers is null.
 */
bool hdsa_common_is_pointer_inside_buffer(const void* const pointer, ptrdiff_t buffer_capacity, const void* const buffer)
{
    HDSA_BASIC_ASSERT((buffer_capacity > 0), "The size of the first buffer must be at least 1.\n");
    HDSA_BASIC_ASSERT(((size_t)buffer <= (size_t)PTRDIFF_MAX), "The first buffer goes beyond what is representable by ptrdiff_t.\n");
    HDSA_BASIC_ASSERT(((size_t)pointer <= (size_t)PTRDIFF_MAX), "The second buffer goes beyond what is representable by ptrdiff_t.\n");

    if ((!buffer) || (!pointer))
    {
        return false;
    }

    const ptrdiff_t buff = (ptrdiff_t)buffer;
    const ptrdiff_t poin = (ptrdiff_t)pointer;

    if ((poin < buff) || ((poin - buff) > (buffer_capacity - 1)))
    {
        return false;
    }

    return true;
}

/**
 * @brief It checks if x is a power of 2. False for numbers smaller than 1,
 * even negative numbers.
 * @param[in] x Number to check, any value representable by ptrdiff_t is valid.
 * @returns True is x is a power of 2, false otherwise.
 */
bool hdsa_common_is_power_of_two(ptrdiff_t x)
{
    if (x < 1) { return false; }

    return ((x & (x - 1)) == 0);
}

/**
 * @brief If x isn't some power of 2, then increase it until it becomes a power of 2.
 * @param[in] requested_size Number to round up to a power of 2, any value
 * representable by ptrdiff_t is valid.
 * @returns A power of 2. If requested_size is already a power of 2 then the same
 * requested_size s returned. 1 when the requested_size is less than 2 and that includes
 * negative numbers.
 */
ptrdiff_t hdsa_common_round_pow_two(ptrdiff_t requested_size)
{
    if (requested_size < 2)
    {
        return 1;
    }

    if (hdsa_common_is_power_of_two(requested_size))
    {
        return requested_size;
    }

    ptrdiff_t power = 2;

    while (requested_size > power)
    {
        HDSA_BASIC_ASSERT(((PTRDIFF_MAX >> 1) >= power), "power went above the limits the ptrdiff_t.\n");

        power = power << 1;
    }

    return power;
}

/**
 * @brief If initial_number isn't a multiple of "base", then increase it until it
 * becomes a multiple of base.
 * @param[in] initial_number Number to round up, must be 0 or bigger.
 * @param[in] base initial_number will be rounded up to a multiple of it. It
 * must be at least 1.
 * @returns A multiple of base that is bigger or equal to inital_number. In the
 * case initial_number is 0 the result value is 0.
 */
ptrdiff_t hdsa_common_round_to_multiple(ptrdiff_t initial_number, ptrdiff_t base)
{
    HDSA_BASIC_ASSERT((initial_number > -1), "initial_number is smaller than 0.\n");
    HDSA_BASIC_ASSERT((base > 0), "base for rounding up is smaller than 1.\n");

    if (initial_number == 0)
    {
        return 0;
    }

    ptrdiff_t reminder = initial_number % base;

    if (reminder > 0)
    {
        HDSA_BASIC_ASSERT((initial_number < (PTRDIFF_MAX - (base - reminder))), "The calculated virtual memory block calculated went above the limits the ptrdiff_t.\n");

        return initial_number + (base - reminder);
    }

    return initial_number;
}

/**
 * @brief It checks if "ptr" is correctly aligned to "alignment" or not.
 * @param[in] ptr Pointer to check the alignment for, must not be null.
 * @param[in] alignment Alignment value to use on ptr, must be 1 or bigger.
 * @returns True if ptr is aligned to alignment, false otherwise.
 */
bool hdsa_common_is_aligned(const void* const ptr, const ptrdiff_t alignment)
{
    HDSA_BASIC_ASSERT((ptr), "The pointer to compare for alignment is null.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "The alignment is smaller than 1.\n");

    return ((ptrdiff_t)ptr % alignment) == 0;
}

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
ptrdiff_t hdsa_common_align_forward(ptrdiff_t ptr, ptrdiff_t alignment)
{
    HDSA_BASIC_ASSERT((ptr > 0), "The address for alignment is negative or 0.\n");

    ptrdiff_t p = 0, a = 0, modulo = 0;

    // If alignment isn't a power of 2, increase it up to the next power of 2
    if (!hdsa_common_is_power_of_two(alignment))
    {
        alignment = hdsa_common_round_pow_two(alignment);
    }

    p = ptr;
    a = alignment;
    modulo = p & (a - 1); // Same as (size % alignment) but faster as 'alignment' is a power of two

    if (modulo != 0)
    {
        p += a - modulo;
    }

    return p;
}

/**
 * @brief Padding to align "ptr" to "alignment" (while taking into account
 * "header_size") to prevent slowdowns, crashes or UB caused by un-aligned
 * memory access by the CPU and returns the necessary padding amount for it.
 * @param[in] ptr Pointer to align. The address inside of it must be
 * representable by ptrdiff_t and it must not be null.
 * @param[in] header_size Size of a header that will be used in the allocation
 * as well. This size will be used as the minimum padding, and it must be 1 or
 * bigger.
 * @param[in] alignment Alignment value to use on ptr, must be 1 or bigger.
 * @returns Padding to align ptr as a ptrdiff_t value. It will be at least as
 * big as header_size.
 */
ptrdiff_t hdsa_common_padding_with_header(ptrdiff_t ptr, ptrdiff_t header_size, ptrdiff_t alignment)
{
    HDSA_BASIC_ASSERT((ptr > 0), "The address for alignment is negative or 0.\n");
    HDSA_BASIC_ASSERT((header_size > 0), "The header_size is smaller than 1.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "The allocation_alignment is smaller than 1.\n");

    ptrdiff_t p = 0, a = 0, modulo = 0, padding = 0, needed_space = 0;

    // If alignment isn't a power of 2, increase it up to the next power of 2
    if (!hdsa_common_is_power_of_two(alignment))
    {
        alignment = hdsa_common_round_pow_two(alignment);
    }

    p = ptr;
    a = alignment;
    modulo = p & (a - 1); // (p % a) as it assumes alignment is a power of two

    padding = 0;
    needed_space = 0;

    // Same logic as 'align_forward'
    if (modulo != 0)
    {
        padding = a - modulo;
    }

    needed_space = header_size;

    if (padding < needed_space)
    {
        needed_space -= padding;

        if ((needed_space & (a - 1)) != 0)
        {
            padding += a * (1 + (needed_space / a));
        }
        else
        {
            padding += a * (needed_space / a);
        }
    }

    return padding;
}

/**
 * @brief Padding to align "ptr" to "alignment" like
 * padding_with_header but to a previous address (subtract the
 * returned value from ptr) and for the alignment of the header.
 *
 * Commonly used to calculate the padding for a footer for a node inside a
 * linked-list.
 * @param[in] ptr Pointer to align. The address inside of it must be
 * representable by ptrdiff_t and it must not be null.
 * @param[in] header_size Size of a header that will be used in the allocation
 * as well. This size will be used as the minimum padding, and it must be 1 or
 * bigger.
 * @param[in] alignment Alignment value to use on ptr, must be 1 or bigger.
 * @returns Padding to align ptr as a ptrdiff_t value. It will be at least as
 * big as header_size.
 */
ptrdiff_t hdsa_common_inverse_padding_for_header(ptrdiff_t ptr, ptrdiff_t header_size, ptrdiff_t alignment)
{
    HDSA_BASIC_ASSERT((ptr > 0), "The address for alignment is negative or 0.\n");
    HDSA_BASIC_ASSERT((header_size > 0), "The header_size is smaller than 1.\n");
    HDSA_BASIC_ASSERT((alignment > 0), "The allocation_alignment is smaller than 1.\n");

    ptrdiff_t padding = hdsa_common_padding_with_header(ptr, header_size, alignment);
    ptrdiff_t padding_difference = (padding == header_size) ? 0 : (header_size + alignment) - padding;

    return header_size + padding_difference;
}
