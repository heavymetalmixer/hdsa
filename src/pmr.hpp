#ifndef HDSA_PMR
#define HDSA_PMR

#include <windows.h>
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

bool is_power_of_two(std::size_t x)
{
    if (x == 0) { return false; }

    return ((x & (x - 1)) == 0);
}

// If x isn't some power of 2, then increase it until it becomes a power of 2
// It returns 1 when the requested_size is 0, as this function is used for finding
// alignments, and there's no aligment 0
std::size_t round_pow_two(std::size_t requested_size)
{
    if ((requested_size == 0) || (requested_size == 1)) { return 1; }

    std::size_t power { 2 };

    while (requested_size > power)
    {
        power = power << 1;
    }

    return power;
}

// A block is 16 pages of 4K each
constexpr std::size_t page_size { 4096 };
constexpr std::size_t block_size { page_size * 16 }; // 65536

// If requested_size isn't a power of 65536, then increase it until it becomes a power of 65536
std::size_t round_to_block(std::size_t requested_size, std::size_t sys_block_size)
{
    if (requested_size == 0) { return 0; }

    std::size_t reminder { requested_size % sys_block_size };

    if (reminder > 0)
    {
        return requested_size + (sys_block_size - reminder);
    }

    return requested_size;
}

/* It works as the base for all the other allocators, it only provides raw un-aligned memory,
 * so if you use memory from this allocator directly you'll need to align it yourself.
 * It allocates pages of virtual memory with OS-specific functions of 4 Kilobytes each, but
 * given that in 32 and 64 bits PCs Windows allocates within 64K boundaries called
 * "granularity", in order to use the pages in a linear way (one after another)
 * this allocator makes allocations in blocks of 64K(each block has 16 pages).
 * To be able to keep the same behavior on all OSes, Linux and Mac also get allocations in
 * blocks of 64K.
 * The 64K blocks are also called "Virtual Memory Paragraphs" in case you wanna look
 * them up on a search engine.
 * To know why Windows allocates in 64K granularity visit this link:
 * https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223
 */
struct VirtualPageAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    std::size_t block_amount {};
    std::size_t current_offset {};
    std::size_t previous_offset {};

    VirtualPageAlloc() = default;

    explicit VirtualPageAlloc(void* buff, std::size_t blocks) noexcept
    : buffer { static_cast<uint8_t*>(buff) },
      block_amount { blocks }
    {
        HDSA_BASIC_ASSERT((buffer != nullptr), "The buffer is empty!\n");
        HDSA_BASIC_ASSERT((block_amount > 0), "The VirtualPageAlloc resource has 0 pages assigned to it!\n");
        std::cout << "A VirtualPageAlloc resource was created, with " << block_amount << " blocks of " << block_size << " bytes each.\n";
    };

    VirtualPageAlloc(const VirtualPageAlloc& other) = default;
    VirtualPageAlloc(VirtualPageAlloc&& other) noexcept = default;
    VirtualPageAlloc& operator=(const VirtualPageAlloc& other) = default;
    VirtualPageAlloc& operator=(VirtualPageAlloc&& other) noexcept = default;

    ~VirtualPageAlloc()
    {
        std::cout << "A VirtualPageAlloc resource is about to be deleted. It has " << block_amount << " blocks of " << block_size << " bytes assigned to it.\n";
        // if (source_alloc != nullptr) { release(); }
    }

    /**
     * 1) Call the OS virtual memory allocation function with the indicated number of blocks.
     * 2) Update the block_amount by added the new blocks to it.
     */
    void* allocate_blocks(std::size_t blocks) noexcept
    {
        // Call the OS to get the amount of blocks of memory required.
        // The starting address will be just one byte after the latest block allocated
        // so all blocks stay as contiguous in the virtual memory
        buffer = static_cast<uint8_t*>(VirtualAlloc(static_cast<void*>(buffer + (block_amount * block_size)), block_size * blocks, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        block_amount += blocks;
        return static_cast<void*>(buffer);
    }

        /**
     * 1) Call the OS virtual memory deallocation function with the indicated number of blocks.
     * 2) Set all the allocator members to 0.
     */
    void* deallocate_blocks(std::size_t blocks) noexcept
    {
        VirtualFree(buffer, 0, MEM_RELEASE);
        buffer = nullptr;
        block_amount = 0;
        current_offset = 0;
        previous_offset = 0;
    }

    void* do_allocate(std::size_t number_of_bytes, std::size_t alignment) override
    {

    }

    // It does nothing
    void do_deallocate(void* p, std::size_t number_of_bytes, std::size_t alignment) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const VirtualPageAlloc*>(&other);
    }
};

// Memory resource for linear allocators, AKA Arena allocators, the most simple
// and basic kind of allocator and also the fastest one
struct ArenaAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    std::size_t buffer_length {};
    // std::pmr::memory_resource* source_alloc {}; // In PMR allocators from the Standard Library it's called "upstream"
    std::size_t current_offset {};
    std::size_t previous_offset {};

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

    ArenaAlloc() = default;

    explicit ArenaAlloc(void* buff, std::size_t buffer_size) noexcept
    : buffer { static_cast<uint8_t*>(buff) },
      buffer_length { buffer_size }
    {
        HDSA_BASIC_ASSERT((buffer != nullptr), "The buffer is empty!\n");
        HDSA_BASIC_ASSERT((buffer_length > 0), "The size of the buffer is 0!\n");
        std::cout << "An Arena memory resource was created, and a buffer of " << buffer_length << " bytes was assigned to it\n";
    };

    ArenaAlloc(const ArenaAlloc& other) = default;
    ArenaAlloc(ArenaAlloc&& other) noexcept = default;
    ArenaAlloc& operator=(const ArenaAlloc& other) = default;
    ArenaAlloc& operator=(ArenaAlloc&& other) noexcept = default;

    ~ArenaAlloc()
    {
        std::cout << "An Arena memory resource is about to be deleted. It has a buffer of " << buffer_length << " bytes assigned to it\n";
        // if (source_alloc != nullptr) { release(); }
    }

    // Assign an existing buffer to this resource if the resource was already constructed without it
    // or if you just wanna change the buffer it points to
    void assign_buffer(void* buff, std::size_t buffer_size)
    {
        buffer = static_cast<uint8_t*>(buff);
        buffer_length = buffer_size;
        previous_offset = 0;
        current_offset = 0;
    }

    // It doesn't really erase any memory nor objects, it only resets the offsets and
    // remaining_space so allocations can be done from the beginning of the buffer again
    // In other words: All the data in there will be overwritten once new allocations are made
    void arena_reset()
    {
        previous_offset = 0;
        current_offset = 0;
    }

    std::size_t remaining_storage()
    {
        return buffer_length - current_offset;
    }

    // Calls the deallocate method from the upstream resource, which for an upstream ArenaAlloc does nothing, AKA it's a no-op
    // void release()
    // {
    //     source_alloc->deallocate(buffer, buffer_length);
    // }

    uintptr_t align_forward(uintptr_t ptr, std::size_t alignment = alignof(std::max_align_t))
    {
        uintptr_t p, a, modulo;

        // If alignment isn't a power of 2, increase it up to the next power of 2
        if (!is_power_of_two(alignment))
        {
            alignment = round_pow_two(alignment);
        }

        //HDSA_BASIC_ASSERT(is_power_of_two(alignment)), "The alignment provided isn't a power of 2!\n");

        p = ptr;
        a = static_cast<uintptr_t>(alignment);
        modulo = p & (a - 1); // Same as (p % a) but faster as 'a' is a power of two

        // If 'p' address is not aligned, push the address to the next value which is aligned
        if (modulo != 0)
        {
            p += a - modulo;
        }

        // std::cout << "The alignment for this allocation is " << alignof(decltype(p)) << " bytes\n";
        std::cout << "The alignment for this allocation is " << alignment << " bytes\n";
        return p;
    }

    void* linear_allocate(std::size_t number_of_bytes, std::size_t alignment = alignof(std::max_align_t))
    {
        HDSA_BASIC_ASSERT((number_of_bytes > 0), "Using 0 as buffer size is not allowed.\n");

        // Align current_offset forward to the specified alignment
        uintptr_t current_ptr { reinterpret_cast<uintptr_t>(buffer) + static_cast<uintptr_t>(current_offset) };
        std::cout << "current_ptr: " << current_ptr << '\n';

        uintptr_t aligned_offset { align_forward(current_ptr, alignment) };
        std::cout << "aligned_offset (absolute): " << aligned_offset << '\n';

        // Subtract away the buffer address number so
        // aligned_offset becomes an actual relative offset like previous_offset and current_offset,
        // instead of the huge address number it got from current_ptr
        aligned_offset -= reinterpret_cast<uintptr_t>(buffer);

        std::cout << "aligned offset (relative): " << aligned_offset << '\n';

        // Check to see if the backing memory has space left
        // if (current_offset + number_of_bytes <= buffer_length) {
        if ((static_cast<std::size_t>(aligned_offset) + number_of_bytes) <= buffer_length)
        {
            void* ptr { &buffer[aligned_offset] };
            previous_offset = aligned_offset;
            std::cout << "previous_offset: " << previous_offset << '\n';

            current_offset = aligned_offset + number_of_bytes;
            std::cout << "current_offset: " << current_offset << '\n';
            std::cout << "remaining_space: " << (buffer_length - current_offset) << '\n';
            std::cout << "buffer: " << reinterpret_cast<void*>(buffer) << '\n';
            std::cout << "The allocation of " << number_of_bytes << " bytes was successful!\n";

            // Zero new memory by default
            // std::memset(ptr, 0, number_of_bytes);

            return ptr;
        }

        // Return nullptr if the arena is out of memory
        std::cerr << "The buffer is full. The latest allocation couldn't be performed. Returning nullptr\n";
        return nullptr;
    }

    // It changes the size of the latest allocation, not the buffer nor the object
    void* linear_resize(void* old_memory, std::size_t old_size, std::size_t new_size, std::size_t alignment = alignof(std::max_align_t))
    {
        uint8_t* old_mem { static_cast<uint8_t*>(old_memory) };

        if (!is_power_of_two(alignment))
        {
            alignment = round_pow_two(alignment);
        }

        if ((old_mem == nullptr) || (old_size == 0))
        {
            return linear_allocate(new_size, alignment);
        }
        // Checking if old mem can still fit in the space available in the buffer
        else if ((buffer <= old_mem) && old_mem < (buffer + buffer_length))
        {
            if ((buffer + previous_offset) == old_mem)
            {
                current_offset = previous_offset + new_size;

                // Zero ONLY the new memory by default
                // if (new_size > old_size)
                // {
                //     std::memset(&buffer[current_offset], 0, new_size - old_size);
                // }

                return old_memory;
            }
            else
            {
                void* new_memory { linear_allocate(new_size, alignment) };
                std::size_t copy_size { (old_size < new_size) ? old_size : new_size };

                // Copy across old memory to the new memory
                memmove(new_memory, old_memory, copy_size);
                return new_memory;
            }
        }
        else
        {
            // Return nullptr if the arena is out of memory
            std::cerr << "The buffer of ArenaAlloc is full. The latest allocation couldn't be performed. Returning nullptr\n";
            return nullptr;
        }
    }

    // It does nothing
    void arena_free(void* ptr) noexcept
    {}

    void* do_allocate(std::size_t number_of_bytes, std::size_t alignment) override
    {
        std::cout << "current_offset: " << current_offset << '\n';
        std::cout << "remaining_space: " << (buffer_length - current_offset) << '\n';
        return linear_allocate(number_of_bytes, alignment);
    }

    // It does nothing
    void do_deallocate(void* p, std::size_t number_of_bytes, std::size_t alignment) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const ArenaAlloc*>(&other);
    }
};

// Think of this temporal arena as a checkpoint that saves the latest allocation offsets
// so once the original ArenaAlloc allocates once more and deallocates, it can
// go back to that previous allocation offset that TempArena saved instead of having to
// reset the whole buffer
struct TempArena
{
	ArenaAlloc* lmm { nullptr };
	std::size_t pre_offset {};
	std::size_t cur_offset {};

    TempArena() = default;

    explicit TempArena(ArenaAlloc* arena) noexcept
    : lmm(arena),
      pre_offset(arena->previous_offset),
      cur_offset(arena->current_offset)
    {}

    TempArena(const TempArena& other) = default;
    TempArena(TempArena&& other) noexcept = default;
    TempArena& operator=(const TempArena& other) = default;
    TempArena& operator=(TempArena&& other) noexcept = default;

    ~TempArena()
    {
        std::cout << "A TempArena is about to be destroyed. Its offsets are: \n";
        std::cout << "pre_offset: " << pre_offset << '\n';
        std::cout << "cur_offset: " << cur_offset << '\n';
    }

    // Cut ties with the current ArenaAlloc manually, so it can be used by
    // another one before the TempArena is destroyed
    void temp_arena_end()
    {
        if ((lmm == nullptr) || (cur_offset == 0))
        {
            std::cout << "This temporal arena is empty. No changes will be performed to the Linear memory resource.\n";
            pre_offset = 0;
            cur_offset = 0;
            lmm = nullptr;
            return;
        }

        lmm->previous_offset = pre_offset;
        lmm->current_offset = cur_offset;
        pre_offset = 0;
        cur_offset = 0;
        lmm = nullptr;
    }

    // If you wanna assign a ArenaAlloc to a default-constructed TempArena,
    // or to reuse the same TempArena for a different ArenaAlloc
    void assign_arena(ArenaAlloc* arena)
    {
        lmm = arena;
        pre_offset = arena->previous_offset;
        cur_offset = arena->current_offset;
    }
};

struct StackHeader
{
    std::size_t padding {};
    std::size_t start_offset {};
};

struct StackAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    std::size_t buffer_length {};
    std::size_t current_offset {};
    std::size_t start_offset {};

    StackAlloc() = default;

    explicit StackAlloc(void* backing_buffer, std::size_t backing_buffer_length) noexcept
    : buffer(static_cast<uint8_t*>(backing_buffer)),
      buffer_length(backing_buffer_length)
    {}

    /**
     * TODO: Implement
     */
    ~StackAlloc() {}

    // Assign buffer and length in case the StackAlloc was already default-constructed
    // or all its memory was freed
    void assign_stack(void* backing_buffer, std::size_t backing_buffer_length) noexcept
    {
        buffer = static_cast<uint8_t*>(backing_buffer);
        buffer_length = backing_buffer_length;
        start_offset = 0;
        current_offset = 0;
    }

    uintptr_t align_forward(uintptr_t ptr, std::size_t alignment = alignof(std::max_align_t))
    {
        uintptr_t p, a, modulo;

        // If alignment isn't a power of 2, increase it up to the next power of 2
        if (!is_power_of_two(alignment))
        {
            alignment = round_pow_two(alignment);
        }

        //HDSA_BASIC_ASSERT(is_power_of_two(alignment)), "The alignment provided isn't a power of 2!\n");

        p = ptr;
        a = static_cast<uintptr_t>(alignment);
        modulo = p & (a - 1); // Same as (p % a) but faster as 'a' is a power of two

        // If 'p' address is not aligned, push the address to the next value which is aligned
        if (modulo != 0)
        {
            p += a - modulo;
        }

        // std::cout << "The alignment for this allocation is " << alignof(decltype(p)) << " bytes\n";
        std::cout << "The alignment for this allocation is " << alignment << " bytes\n";
        return p;
    }

    /**
     * In the allocation the first element is the header, then the allocated size and finally
     * the padding.
     * current_offset is located after the allocated bytes without padding.
     * start_offset is at th beginning the allocation. current_offset - start_offset gives
     * the size of the allocation, without the padding.
     * StackHeader stores the padding and start_offset of the previous allocation.
     */
    void* stack_push(std::size_t size, std::size_t alignment = alignof(std::max_align_t)) noexcept
    {
        std::cout << "###############   ALLOCATION!   ################\n";

        uintptr_t current_address, next_address;
        std::size_t padding;
        StackHeader* header;

        // if (!is_power_of_two(alignment))
        // {
        //     alignment = round_pow_two(alignment);
        // }

        std::cout << "size: " << size << '\n';
        std::cout << "alignment: " << alignment << '\n';
        std::cout << "current_offset and start_offset before: " << current_offset << ", " << start_offset << '\n';

        current_address = reinterpret_cast<uintptr_t>(buffer) + static_cast<uintptr_t>(current_offset);
        next_address = align_forward(current_address, alignment);
        padding = next_address - current_address;

        std::cout << "aligned offset: " << (next_address - reinterpret_cast<uintptr_t>(buffer)) << '\n';
        std::cout << "padding for the previous allocation: " << padding << '\n';

        // Stack allocator is out of memory
        if (current_offset + padding + size > buffer_length)
        {
            return nullptr;
        }

        header = reinterpret_cast<StackHeader*>(next_address);
        header->start_offset = start_offset;
        header->padding = padding;
        start_offset = static_cast<std::size_t>(next_address - reinterpret_cast<uintptr_t>(buffer)) + sizeof(StackHeader);

        std::cout << "header offset: " << (reinterpret_cast<uintptr_t>(header) - reinterpret_cast<uintptr_t>(buffer)) << '\n';
        std::cout << "sizeof(StackHeader): " << sizeof(StackHeader) << '\n';
        std::cout << "header->padding: " << (header->padding) << '\n';
        std::cout << "header->start_offset: " << (header->start_offset) << '\n';

        current_offset += padding + sizeof(StackHeader) + size;

        std::cout << "start_offset: " << start_offset << '\n';
        std::cout << "current_offset: " << current_offset << '\n';

        next_address = reinterpret_cast<uintptr_t>(buffer) + static_cast<uintptr_t>(current_offset);
        return reinterpret_cast<void*>(next_address);
    }

    /**
     * Making use of the start_offset and the StackHeader it updates the offsets
     * to the ones of the previous allocation.
     */
    void stack_pop()
    {
        // if (ptr != nullptr)
        // {
        //     uintptr_t start, end, current_address;
        //     StackHeader *header;
        //     std::size_t temp_previous_offset;

        //     start = reinterpret_cast<uintptr_t>(buffer);
        //     end = start + static_cast<uintptr_t>(buffer_length);
        //     current_address = reinterpret_cast<uintptr_t>(ptr);

        //     HDSA_BASIC_ASSERT(((start <= current_address) && (current_address < end)), "ptr is out of bounds memory!. Called from stack_free().\n");

        //     if (current_address >= (start + static_cast<uintptr_t>(current_offset)))
        //     {

        //     }

        //     header = reinterpret_cast<StackHeader*>(current_address - static_cast<uintptr_t>(sizeof(StackHeader)));
        //     temp_previous_offset = static_cast<size_t>((current_address - static_cast<uintptr_t>(header->padding)) - start);

        //     HDSA_BASIC_ASSERT((temp_previous_offset == header->previous_offset), "Out of order stack allocator free.\n");

        //     current_offset = previous_offset;
        //     previous_offset = header->previous_offset;
        // }

        std::cout << "----------------   DEALLOCATION!   ------------------\n";

        if(!((buffer == nullptr) && (current_offset == 0)))
        {
            uintptr_t start, current_address;
            StackHeader* header;
            std::size_t header_offset;

            std::cout << "start_offset before: " << start_offset << '\n';
            std::cout << "current_offset before: " << current_offset << '\n';

            start = reinterpret_cast<uintptr_t>(buffer);
            current_address = static_cast<uintptr_t>(start_offset) + start;

            header = reinterpret_cast<StackHeader*>(current_address - static_cast<uintptr_t>(sizeof(StackHeader)));
            header_offset = static_cast<std::size_t>(reinterpret_cast<uintptr_t>(header) - start);

            std::cout << "header_offset: " << header_offset << '\n';
            std::cout << "header->start_offset: " << header->start_offset << '\n';

            current_offset = header_offset - header->padding;
            start_offset = header->start_offset;

            std::cout << "start_offset after: " << start_offset << '\n';
            std::cout << "current_offset after: " << current_offset << '\n';
        }
    }

    void* stack_resize(void* ptr, std::size_t old_size, std::size_t new_size, std::size_t alignment = alignof(std::max_align_t))
    {
        if (ptr == nullptr)
        {
            return stack_push(new_size, alignment);
        }
        else if (new_size == 0)
        {
            stack_pop();
            return nullptr;
        }
        else if (old_size == new_size)
        {
            return ptr;
        }
        else
        {
            uintptr_t start, end, current_address;
            std::size_t min_size { (old_size < new_size) ? old_size : new_size };
            void* new_ptr;

            start = reinterpret_cast<uintptr_t>(buffer);
            end = start + static_cast<uintptr_t>(buffer_length);
            current_address = reinterpret_cast<uintptr_t>(ptr);

            HDSA_BASIC_ASSERT(((start <= current_address) && (current_address < end)), "ptr is out of bounds. Called from stack_resize().\n");

            // Treat as a double free
            if (current_address >= (start + static_cast<uintptr_t>(current_offset)))
            {
                return nullptr;
            }

            new_ptr = stack_push(new_size, alignment);
            memmove(new_ptr, ptr, min_size);
            return new_ptr;
        }
    }

    // It resets the offsets so allocations happen from the beginning of the buffer
    // Basically, the same behavior as in ArenaAlloc
    void stack_reset()
    {
        current_offset = 0;
        start_offset = 0;
    }

    void* do_allocate(std::size_t number_of_bytes, std::size_t alignment) override
    {
        std::cout << "current_offset: " << current_offset << '\n';
        std::cout << "start_offset: " << start_offset << '\n';
        std::cout << "remaining_space: " << (buffer_length - current_offset) << '\n';
        return stack_push(number_of_bytes, alignment);
    }

    void do_deallocate(void* p, std::size_t number_of_bytes, std::size_t alignment) override
    {
        stack_pop();
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const StackAlloc*>(&other);
    }
};


} // namespace hdsa end

#endif // HDSA_PMR
