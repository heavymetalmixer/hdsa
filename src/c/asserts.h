#ifndef HDSA_ASSERTS_HPP
#define HDSA_ASSERTS_HPP

#include <stdlib.h>
#include <stdio.h>


#ifndef HDSA_DISABLE_ASSERTS

/**
 * @brief This kind of assert is more readable than the one in
 * the standard library.
 *
 * If you compare 2 or more values/variables for the "condition",
 * make sure to wrap them in parenthesis.
 * Add -DHDSA_DISABLE_ASSERTS to your compilation options
 * to disable this assert without causing compilation errors.
 */
#define HDSA_BASIC_ASSERT(plain_text_condition, string_literal_message)                                                                                                         \
if (!plain_text_condition)                                                                                                                                                      \
{                                                                                                                                                                               \
    printf("\n\nAssertion failed: %s\n  Condition: %s\n  File: %s\n  Function: %s\n  Line: %d\n", string_literal_message, #plain_text_condition, __FILE__, __func__, __LINE__); \
    exit(1);                                                                                                                                                                    \
}

#else
#define HDSA_BASIC_ASSERT(condition, message)
#endif // HDSA_DISABLE_ASSERTS

#endif // HDSA_ASSERTS_HPP
