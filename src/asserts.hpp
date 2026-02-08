#ifndef HDSA_ASSERTS_HPP
#define HDSA_ASSERTS_HPP

#include <source_location>

// I don't like that C asserts don't work on Release builds so I made this one for the same purpose
// If you compare 2 or more values/variables for the "condition", make sure to wrap them in parenthesis
#define HDSA_BASIC_ASSERT(condition, message)               \
if (!condition)                                             \
{                                                           \
    auto loc { std::source_location::current() };           \
    std::cerr                                               \
        << "\n\nAssertion failed: " << message << '\n'      \
        << "  Condition: " << #condition << '\n'            \
        << "  File: " << loc.file_name() << '\n'            \
        << "  Function: " << loc.function_name() << '\n'    \
        << "  Line: " << loc.line() << '\n'                 \
        << "  Column: " << loc.column() << '\n';            \
    std::abort();                                           \
}                                                           \

#endif // HDSA_ASSERTS_HPP
