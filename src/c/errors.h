#ifndef HDSA_ERROR_H
#define HDSA_ERROR_H

#include <stddef.h>
#include <stdio.h>

typedef enum hdsa_Error_Code
{
    HDSA_ERROR_SUCCESS = 0,
    HDSA_ERROR_NOT_IMPLEMENTED,
    HDSA_ERROR_CUSTOM_ERROR,
    HDSA_ERROR_UNKNOWN_ERROR,
    HSDA_ERROR_OUT_OF_BOUNDS_TOP,
    HSDA_ERROR_OUT_OF_BOUNDS_BOTTOM,
    HDSA_ERROR_DIVISION_BY_ZERO,
    HDSA_ERROR_NULL_POINTER_PARAMETER,
    HDSA_ERROR_NULL_POINTER_RETURN,
    HDSA_ERROR_NULL_POINTER_GLOBAL_VARIABLE,
    HDSA_ERROR_NULL_POINTER_INTERNAL_DATA,
    HDSA_ERROR_NEGATIVE_NOT_ALLOWED,
    HDSA_ERROR_ZERO_NOT_ALLOWED,
    HDSA_ERROR_NEGATIVE_AND_ZERO_NOT_ALLOWED,
    HDSA_ERROR_POSITIVE_NOT_ALLOWED,
    HDSA_ERROR_POSITIVE_AND_ZERO_NOT_ALLOWED,
    HDSA_ERROR_POINTER_NOT_FROM_BUFFER,
    HDSA_ERROR_ALLOCATION_FAILED,
    HDSA_ERROR_ALLOCATION_ZERO_SIZE,
    HDSA_ERROR_ALLOCATION_NULL_BUFFER,
    HDSA_ERROR_ALLOCATION_WRONG_MODE,
    HDSA_ERROR_ALLOCATION_SIZE_ZERO,
    HDSA_ERROR_ALLOCATION_RESERVE_FAILED,
    HDSA_ERROR_ALLOCATION_COMMIT_FAILED,
    HDSA_ERROR_ALLOCATION_BUFFER_IS_FULL,
    HDSA_ERROR_ALLOCATION_NOT_ENOUGH_MEMORY,
    HDSA_ERROR_ALLOCATION_RESIZE_FAILED,
    HDSA_ERROR_ALLOCATION_RESIZE_SAME_SIZES,
    HDSA_ERROR_ALLOCATION_NO_FITTING_NODE_FOUND,
    HDSA_ERROR_DEALLOCATION_FAILED
} hdsa_Error_Code;

#define HDSA_ERROR_STRING_SIZE 512

/**
 * @brief "description" shows the file, function and line of the error, and some more
 * details depending on the function it happens in.
 */
typedef struct hdsa_Error
{
    hdsa_Error_Code code;
    char description[HDSA_ERROR_STRING_SIZE];
} hdsa_Error;

#define HDSA_ERROR_FORMATED_STRING(char_array_destination, hdsa_Error_Code_code, plain_text_condition, int_line, string_literal_message) \
snprintf(char_array_destination, (size_t)HDSA_ERROR_STRING_SIZE, "\nERROR CODE %d: %s\n  Condition: %s\n  File: %s\n  Function: %s\n  Line: %lld\n", hdsa_Error_Code_code, string_literal_message, #plain_text_condition, __FILE__, __func__, (ptrdiff_t)int_line)

#endif // HDSA_ERROR_H
