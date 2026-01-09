#ifndef HDSA_PMR
#define HDSA_PMR

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <iostream>
#include <type_traits>
#include <new>
#include <utility>
#include <limits>
#include <memory>
#include <memory_resource>
#include "asserts.hpp"

namespace hdsa
{

/**
 * Returns true if a struct/class is POD
 * Now, the term "POD" was deprecated from the C++ 20 because it has changed
 * many times since 2003, but the one thing that always remained is that a
 * "POD struct" must be a C struct in terms of the assembly code it compiles
 * to(just data without initialization), plus being able to declare static
 * variables inside of it, being able to use member functions that aren't
 * constructors, and being able to do operator overloads except operator= .
 * So, this templated function is here for you to determine if a struct is
 * POD, so you have an easier time making code compatible for other languages
 * (not just C) or if you don't wanna deal with many of the different
 * intricate rules of C++ and preffer a more C-like style for writting code.
 */
template<typename T>
bool is_pod()
{
    return
    std::is_standard_layout_v<T> &&
    std::is_trivially_default_constructible_v<T> &&
    std::is_trivially_copyable_v<T> &&
    std::is_trivially_copy_constructible_v<T> &&
    std::is_trivially_copy_assignable_v<T> &&
    std::is_trivially_move_constructible_v<T> &&
    std::is_trivially_move_assignable_v<T> &&
    std::is_trivially_destructible_v<T>;
}


// If x isn't some power of 2, then increase it until it becomes a power of 2
std::size_t round_pow_two(std::size_t x)
{
    if (x == 0) { return 1; }

    if (x == 1) { return x; }

    std::size_t i { 1 };

    while(i < x)
    {
        i = (i << 1);
        std::cout << i << '\n';
    }

    return i;
}

template<typename T>
std::size_t round_pow_two(T t) = delete;

// Memeory resource for linear allocators, AKA arena allocators,
// the most simple and basic kind of allocator
struct Linear_mem_resource : public std::pmr::memory_resource
{
    unsigned char* buffer { nullptr };
    std::size_t buffer_length {};
    std::size_t current_offset {};
    std::size_t previous_offset {};
    std::size_t remaining_space {};

    // std::size_t next_buffer_length {};
    // std::pmr::memory_resource* upstream { nullptr };


    // explicit Linear_mem_resource(memory_resource* const new_upstream) noexcept
    //     : upstream{new_upstream} {} // initialize this resource with upstream

    // Linear_mem_resource(const std::size_t buffer_size, memory_resource* const new_upstream) noexcept
    //     : next_buffer_length(buffer_size), upstream{ new_upstream } {
    //     // initialize this resource with upstream and initial allocation size
    // }

    // Linear_mem_resource(void* const buff, const size_t buffer_size,
    //     std::pmr::memory_resource* const new_upstream) noexcept
    //     : buffer(static_cast<std::byte*>(buff)), remaining_space(buffer_size),
    //         next_buffer_length(buffer_size), upstream{ new_upstream } {
    //     // initialize this resource with upstream and initial buffer
    // }

    Linear_mem_resource() = default;

    // Contruct a Linear_mem_resource without assigning a buffer to it
    // So make sure to assign a buffer to it before allocating with it
    explicit Linear_mem_resource(const std::size_t buffer_size) noexcept
    : buffer_length(buffer_size)
    {}

    Linear_mem_resource(void* const buff, const std::size_t buffer_size) noexcept
    : buffer(static_cast<unsigned char*>(buff)),
      buffer_length(buffer_size),
      remaining_space(buffer_size)
    {
        // If the buffer passed as an argument is empty, create a new one
        // if (arena_buffer == nullptr) {
        //     arena_buffer = static_cast<std::byte*>(malloc(sizeof(std::byte) * buffer_size));
        //     arena_buffer_length = buffer_size;
        // }

        HDSA_BASIC_ASSERT((buffer != nullptr), "The buffer is empty!\n");

        std::cout << "An Arena memory resource was created, and a buffer of " << buffer_length << " bytes was assigned to it\n";
    };

    ~Linear_mem_resource()
    {
        std::cout << "An Arena memory resource is about to be deleted. It has a buffer of " << buffer_length << " bytes assigned to it\n";
    }

    // Assign an existing buffer to this resource if the resource was already constructed without it
    // or if you just wanna change the buffer it points to
    void assign_buffer(void* const buff, std::size_t buffer_size)
    {
        buffer = static_cast<unsigned char*>(buff);
        buffer_length = buffer_size;
        previous_offset = 0;
        current_offset = 0;
        remaining_space = buffer_length;
    }

    // It doesn't really erase any memory nor objects, it only resets the offsets and
    // remaining_space so allocations can be done from the beginning of the buffer again
    // In other words: All the data in there will be overwritten once new allocations are made
    void free_memory()
    {
        previous_offset = 0;
        current_offset = 0;
        remaining_space = buffer_length;
    }

    uintptr_t align_forward(uintptr_t ptr, std::size_t alignment = 2 * sizeof(void*))
    {
        uintptr_t p, a, modulo;

        // If alignment isn't a power of 2, increase it up to the next power of 2
        if ((alignment & (alignment - 1)) == 0 )
        {
            alignment = round_pow_two(alignment);
        }

        //HDSA_BASIC_ASSERT(((alignment & (alignment - 1)) == 0), "The alignment provided isn't a power of 2!\n");

        p = ptr;
        a = static_cast<uintptr_t>(alignment);
        modulo = p & (a - 1); // Same as (p % a) but faster as 'a' is a power of two

        if (modulo != 0)
        {
            // If 'p' address is not aligned, push the address to the next value which is aligned
            p += a - modulo;
        }

        // std::cout << "The alignment for this allocation is " << alignof(decltype(p)) << " bytes\n";
        std::cout << "The alignment for this allocation is " << alignment << " bytes\n";
        return p;
    }

    void* linear_alloc(std::size_t number_of_bytes, std::size_t alignment = 2 * sizeof(void*))
    {
        if (number_of_bytes == 0)
        {
            std::cerr << "Using 0 as buffer size IS NOT ALLOWED. Returning nullptr.\n";
            return nullptr;
        }

        // Align current_offset forward to the specified alignment
        uintptr_t current_ptr { reinterpret_cast<uintptr_t>(buffer) + static_cast<uintptr_t>(current_offset) };
        std::cout << "current_ptr: " << current_ptr << '\n';

        uintptr_t aligned_offset { align_forward(current_ptr, alignment) };
        std::cout << "aligned_offset (absolute): " << aligned_offset << '\n';

        aligned_offset -= reinterpret_cast<uintptr_t>(buffer); // Subtract away the buffer adress number so
        // aligned_offset becomes an actual relative offset like previous_offset and current_offset,
        // instead of the huge address number it got from current_ptr
        std::cout << "aligned offset (relative): " << aligned_offset << '\n';

        // Check to see if the backing memory has space left
        // if (current_offset + number_of_bytes <= buffer_length) {
        if (aligned_offset + number_of_bytes <= remaining_space)
        {
            void* ptr { &buffer[aligned_offset] };
            previous_offset = aligned_offset;
            std::cout << "previous_offset: " << previous_offset << '\n';

            current_offset = aligned_offset + number_of_bytes;
            std::cout << "current_offset: " << current_offset << '\n';

            remaining_space = buffer_length - current_offset;
            std::cout << "remaining_space: " << remaining_space << '\n';

            // Zero new memory by default
            memset(ptr, 0, number_of_bytes);
            std::cout << "The allocation of " << number_of_bytes << " bytes was successful!\n";
            return ptr;
        }

        // Return nullptr if the arena is out of memory
        std::cerr << "The buffer is full. The latest allocation couldn't be performed. Returning nullptr\n";
        return nullptr;
    }

    // It changes the size of the latest allocation, not the buffer nor the object
    void* linear_alloc_resize(void* old_memory, std::size_t old_size, std::size_t new_size, std::size_t alignment = 2 * sizeof(void*))
    {
        unsigned char* old_mem { static_cast<unsigned char*>(old_memory) };

        if ((alignment & (alignment - 1)) == 0 )
        {
            alignment = round_pow_two(alignment);
        }

        if (old_mem == nullptr || old_size == 0)
        {
            return linear_alloc(new_size, alignment);
        }
        // Checking if old mem can still fit in the space available in the buffer
        else if (buffer <= old_mem && old_mem < (buffer + buffer_length))
        {
            if (buffer + previous_offset == old_mem)
            {
                current_offset = previous_offset + new_size;

                if (new_size > old_size)
                {
                    // Zero ONLY the new memory by default
                    memset(&buffer[current_offset], 0, new_size - old_size);
                }

                return old_memory;
            }
            else
            {
                void* new_memory { linear_alloc(new_size, alignment) };
                std::size_t copy_size { (old_size < new_size) ? old_size : new_size };

                // Copy across old memory to the new memory
                memmove(new_memory, old_memory, copy_size);
                return new_memory;
            }
        }
        else
        {
            // Return nullptr if the arena is out of memory
            std::cerr << "The buffer is full. The latest allocation couldn't be performed. Returning nullptr\n";
            return nullptr;
        }
    }

    void* do_allocate(std::size_t number_of_bytes, std::size_t alignment = 2 * sizeof(void*)) override
    {
        return linear_alloc(number_of_bytes, alignment);
    }

    // It does nothing
    void do_deallocate(void* p, std::size_t number_of_bytes, std::size_t alignment) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == &other;
    }
};

// Think of this temporal arena as a checkpoint that saves the latest allocation offsets
// so once the original Linear_mem_resource allocates once more and deallocates, it can
// go back to that previous allocation that Temp_arena saved instead of having to
// reset the whole buffer
struct Temp_arena
{
	Linear_mem_resource *lmm { nullptr };
	std::size_t prev_offset {};
	std::size_t curr_offset {};

    Temp_arena() = delete;

    explicit Temp_arena(Linear_mem_resource *arena) noexcept
    : lmm(arena),
      prev_offset(arena->previous_offset),
      curr_offset(arena->current_offset)
    {}

    void temp_arena_end()
    {
        if (lmm == nullptr || curr_offset == 0)
        {
            std::cerr << "This temporal arena is empty. No changes will be performed to the Linear memory resource.\n";
            return;
        }

        lmm->previous_offset = prev_offset;
        lmm->current_offset = curr_offset;
    }
};

struct Stack_header
{
    uint8_t padding {};
};

struct Stack_resource : public std::pmr::memory_resource
{
    unsigned char* buffer { nullptr };
    std::size_t buffer_length {};
    std::size_t offset {};

    Stack_resource() = default;

    explicit Stack_resource(void* backing_buffer, std::size_t backing_buffer_length)  noexcept
    : buffer(static_cast<unsigned char*>(backing_buffer)),
      buffer_length(backing_buffer_length),
      offset(0)
    {}

    // It gives the padding neccesary to fit the header right after, followed by the next aligned allocation
    uintptr_t calc_padding_with_header(uintptr_t ptr, std::size_t header_size, std::size_t alignment = 2 * sizeof(void*))
    {
        uintptr_t p, a, modulo, padding, needed_space;

        // If alignment isn't a power of 2, increase it up to the next power of 2
        if ((alignment & (alignment - 1)) == 0 )
        {
            alignment = round_pow_two(alignment);
        }

        //HDSA_BASIC_ASSERT(((alignment & (alignment - 1)) == 0), "The alignment provided isn't a power of 2!\n");

        p = ptr;
        a = static_cast<uintptr_t>(alignment);
        modulo = p & (a - 1); // Same as (p % a) but faster as 'a' is a power of two

        padding = 0;
        needed_space = 0;

        if (modulo != 0)
        {
            // If 'p' address is not aligned, push the address to the next value which is aligned
            padding = a - modulo;
        }

        needed_space = static_cast<uintptr_t>(header_size);

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

        // std::cout << "The alignment for this allocation is " << alignof(decltype(p)) << " bytes\n";
        std::cout << "The alignment for this allocation is " << alignment << " bytes\n";
        return padding;
    }
};

} // namespace hdsa end

#endif // HDSA_PMR
