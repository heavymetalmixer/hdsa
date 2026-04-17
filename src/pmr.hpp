#ifndef HDSA_PMR_HPP
#define HDSA_PMR_HPP

// OS libraries
#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
#elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)
#include <sys/mman.h>
#endif // OS libraries


#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <type_traits>
#include <new>
#include <limits>
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

bool is_power_of_two(size_t x)
{
    if (x == 0) { return false; }

    return ((x & (x - 1)) == 0);
}

// If x isn't some power of 2, then increase it until it becomes a power of 2
// It returns 1 when the requested_size is 0, as this function is used for finding
// alignments, and there's no alignment 0
size_t round_pow_two(size_t requested_size)
{
    if ((requested_size == 0) || (requested_size == 1)) { return 1; }

    size_t power { 2 };

    while (requested_size > power)
    {
        power = power << 1;
    }

    return power;
}

// Returns true if "ptr" is aligned to "alignment"
bool is_aligned(const void* ptr, const size_t alignment)
{
    return (reinterpret_cast<uintptr_t>(ptr) % alignment) == 0;
}

// If requested_size isn't a power of 65536, then increase it until it becomes a power of 65536
size_t round_to_block(size_t requested_size, size_t sys_block_size)
{
    if (requested_size == 0) { return 0; }

    size_t reminder { requested_size % sys_block_size };

    if (reminder > 0)
    {
        return requested_size + (sys_block_size - reminder);
    }

    return requested_size;
}

/**
 * Align ptr to alignment to prevent slowdowns caused by un-aligned
 * memory access by the CPU.
 */
uintptr_t align_forward_uintptr(uintptr_t ptr, size_t alignment) noexcept
{
    uintptr_t p {};
    uintptr_t a {};
    uintptr_t modulo {};

    // If alignment isn't a power of 2, increase it up to the next power of 2
    if (!is_power_of_two(alignment))
    {
        alignment = round_pow_two(alignment);
    }

    p= ptr;
    a = static_cast<uintptr_t>(alignment);
    modulo = p & (a - 1); // Same as (size % alignment) but faster as 'alignment' is a power of two

    if (modulo != 0)
    {
        p += a - modulo;
    }

    return p;
}

/**
 * Same as align_forward_uintptr but for size_t
 */
uintptr_t align_forward_size(size_t ptr, size_t alignment = 2 * sizeof(void *)) noexcept
{
    size_t p {};
    size_t a {};
    size_t modulo {};

    // If alignment isn't a power of 2, increase it up to the next power of 2
    if (!is_power_of_two(alignment))
    {
        alignment = round_pow_two(alignment);
    }

    p= ptr;
    a = alignment;
    modulo = p & (a - 1); // Same as (size % alignment) but faster as 'alignment' is a power of two

    if (modulo != 0)
    {
        p += a - modulo;
    }

    return p;
}

/**
 * It returns the padding plus the size of the header of the current allocation
 */
size_t calc_padding_with_header(uintptr_t ptr, size_t header_size, uintptr_t alignment)
{
    uintptr_t p {};
    uintptr_t a {};
    uintptr_t modulo {};
    uintptr_t padding {};
    uintptr_t needed_space {};

    // If alignment isn't a power of 2, increase it up to the next power of 2
    if (!is_power_of_two(alignment))
    {
        alignment = round_pow_two(alignment);
    }

    p = ptr;
    a = alignment;
    modulo = p & (a-1); // (p % a) as it assumes alignment is a power of two

    padding = 0;
    needed_space = 0;

    // Same logic as 'align_forward_uintptr'
    if (modulo != 0)
    {
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

    return static_cast<size_t>(padding);
}

// A block is 16 pages of 4K each
constexpr size_t page_size { 4096 };
constexpr size_t block_size { page_size * 16 }; // 65536

/* 1) It works as the base for all the other allocators, it only provides ONE BLOCK of un-aligned memory.
 * Fortunately, other allocators out there can align the addresses themselves, and the ones in this
 * file/library always do.
 * 2) The "end_address" is the address at the end of "buffer", in case you
 * wanna create another VirtualPageAlloc right after this one (if the OS allows it).
 * If you wanna use it for that don't forget to add 1 to it or you'll be corrupting memory from
 * the last address of the current buffer.
 * 3) It allocates pages of virtual memory with OS-specific functions of 4 KB each, but
 * given that in 32 and 64 bits PCs Windows allocates within 64 KB boundaries called
 * "granularity", in order to use the pages in a linear way (one after another)
 * this allocator makes allocations in blocks of 64 KB(each block has 16 pages).
 * To be able to keep the same behavior on all OSes, Linux and Mac also get allocations in
 * blocks of 64 KB.
 * 4) The 64 KB blocks are also called "Virtual Memory Paragraphs" in case you wanna look
 * them up on a search engine.
 * 5) To know why Windows allocates in 64 KB granularity visit this link:
 * https://devblogs.microsoft.com/oldnewthing/20031008-00/?p=42223
 */
struct VirtualPageAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    uint8_t* end_address {};
    size_t block_amount {};
    size_t buffer_length {};

    VirtualPageAlloc() = default;

    explicit VirtualPageAlloc(void* buff, size_t blocks) noexcept
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

    /**
     * 1) Call the OS virtual memory allocation function with the indicated number of blocks.
     * 2) Update the block_amount by added the new blocks to it.
     * 3) The starting address will be just one byte after the latest block allocated
     * so all blocks stay as contiguous in the virtual memory.
     */
    void* page_allocate(size_t size) noexcept
    {
        size_t real_size { round_to_block(size, block_size) };
        size_t blocks { real_size / block_size };
        std::cout << "VirtualPageAlloc real size: " << real_size << '\n';
        std::cout << "VirtualPageAlloc amount of blocks to allocate: " << blocks << '\n';

        // OS selection for the virtual memory allocation function
    #if defined(_WIN32) || defined(_WIN64)
        buffer = static_cast<uint8_t*>(VirtualAlloc(static_cast<void*>(buffer + buffer_length), real_size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    #elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)
        buffer = static_cast<uint8_t*>(mmap(static_cast<void*>(buffer + buffer_length()), block_size * blocks, PROT_READ+PROT_WRITE, MAP_ANONYMOUS+MAP_SHARED, -1, 0));
    #endif

        block_amount = blocks;
        buffer_length = real_size;
        end_address = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(buffer) + static_cast<uintptr_t>(buffer_length));

        std::cout << "VirtualPageAlloc buffer size: " << buffer_length << '\n';

        return static_cast<void*>(buffer);
    }

    /**
     * 1) Call the OS virtual memory deallocation function for all the memory this allocator points to at once.
     * 2) Set all the allocator members to 0.
     */
    void page_deallocate() noexcept
    {
        std::cout << "A VirtualPageAlloc's memory is about to be released back to the OS. It has " << block_amount << " blocks of " << block_size << " bytes assigned to it.\n";

        // OS selection for the virtual memory allocation function
        // In both cases all the memory is released at the same time
    #if defined(_WIN32) || defined(_WIN64)
        VirtualFree(buffer, 0, MEM_RELEASE);
    #elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)
        munmap(static_cast<void*>(buffer), buffer_length());
    #endif

        buffer = nullptr;
        end_address = nullptr;
        buffer_length = 0;
        block_amount = 0;
    }

    void* do_allocate(size_t number_of_bytes, size_t alignment) override
    {
        return page_allocate(number_of_bytes);
    }

    void do_deallocate(void* p, size_t number_of_bytes, size_t alignment) override
    {
        page_deallocate();
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const VirtualPageAlloc*>(&other);
    }
};

/**
 * Memory resource for linear allocators, AKA Arena allocators, the most simple
 * and basic kind of allocator and also the fastest one
 */
struct ArenaAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    size_t buffer_length {};
    size_t current_offset {};
    size_t previous_offset {};
    size_t allocation_amount {};

    ArenaAlloc() = default;
    ArenaAlloc(const ArenaAlloc& other) = default;
    ArenaAlloc(ArenaAlloc&& other) noexcept = default;
    ArenaAlloc& operator=(const ArenaAlloc& other) = default;
    ArenaAlloc& operator=(ArenaAlloc&& other) noexcept = default;

    explicit ArenaAlloc(void* buff, size_t buffer_size) noexcept
    : buffer { static_cast<uint8_t*>(buff) },
      buffer_length { buffer_size }
    {
        HDSA_BASIC_ASSERT((buffer != nullptr), "The buffer is empty!\n");
        HDSA_BASIC_ASSERT((buffer_length > 0), "The size of the buffer is 0!\n");
        std::cout << "An Arena memory resource was created, and a buffer of " << buffer_length << " bytes was assigned to it\n";
    };

    /**
     * Assign an existing buffer to this resource if the resource was already constructed without it
     * or if you just wanna change the buffer it points to
     */
    void assign_buffer(void* buff, size_t buffer_size) noexcept
    {
        buffer = static_cast<uint8_t*>(buff);
        buffer_length = buffer_size;
        previous_offset = 0;
        current_offset = 0;
        allocation_amount = 0;

        std::cout << "A new buffer was assigned for the ArenaAlloc!\n";
    }

    size_t remaining_storage()
    {
        return buffer_length - current_offset;
    }

    void* arena_allocate(size_t number_of_bytes, size_t alignment) noexcept
    {
        HDSA_BASIC_ASSERT((number_of_bytes > 0), "Using 0 as buffer size is not allowed.\n");

        std::cout << "###############   ALLOCATION!   ################\n";

        uintptr_t start {};
        uintptr_t current_address {};
        uintptr_t aligned_address {};
        size_t aligned_offset {};

        std::cout << "allocation size: " << number_of_bytes << '\n';
        std::cout << "allocation_amount before: " << allocation_amount << '\n';
        std::cout << "remaining_space before: " << remaining_storage() << '\n';
        std::cout << "previous_offset before: " << previous_offset << '\n';
        std::cout << "current_offset before: " << current_offset << '\n';

        start = reinterpret_cast<uintptr_t>(buffer);
        current_address = start + static_cast<uintptr_t>(current_offset);
        aligned_address = align_forward_uintptr(current_address, alignment);

        std::cout << "current_address: " << current_address << '\n';
        std::cout << "aligned_address: " << aligned_address << '\n';

        // Subtract away the buffer address number so
        // aligned_offset becomes an actual relative offset like previous_offset and current_offset,
        // instead of the huge address number aligned_address has
        aligned_offset = static_cast<size_t>(aligned_address - start);
        std::cout << "aligned offset: " << aligned_offset << '\n';

        // Check to see if the buffer has space left
        if ((aligned_offset + number_of_bytes) <= buffer_length)
        {
            previous_offset = aligned_offset;
            current_offset = aligned_offset + number_of_bytes;
            allocation_amount += 1;

            std::cout << "previous_offset after: " << previous_offset << '\n';
            std::cout << "current_offset after: " << current_offset << '\n';
            std::cout << "remaining_space: " << remaining_storage() << '\n';
            std::cout << "allocation_amount after: " << allocation_amount << "\n\n\n";

            return reinterpret_cast<void*>(aligned_address);
        }

        // Return nullptr if the arena is out of memory
        std::cerr << "The buffer is full. The latest allocation couldn't be performed. Returning nullptr\n";
        return nullptr;
    }

    // It does nothing
    void arena_deallocate(void* ptr) noexcept
    {}

    // It changes the size of the latest allocation, not the buffer nor the object
    void* arena_resize(void* old_memory, size_t old_size, size_t new_size, size_t alignment) noexcept
    {
        std::cout << "***************   RESIZING!   ****************\n";

        uintptr_t previous_address {};
        uintptr_t start {};
        uintptr_t end {};
        uintptr_t old_mem {};

        start = reinterpret_cast<uintptr_t>(buffer);
        end = start + static_cast<uintptr_t>(buffer_length);
        previous_address = start + static_cast<uintptr_t>(previous_offset);
        old_mem = reinterpret_cast<uintptr_t>(old_memory);

        if (!is_power_of_two(alignment))
        {
            alignment = round_pow_two(alignment);
        }

        if ((old_memory == nullptr) || (old_size == 0))
        {
            return arena_allocate(new_size, alignment);
        }
        // Checking if old mem can still fit in the space available in the buffer
        else if ((start <= old_mem) && (old_mem < end))
        {
            // Checking if old_mem is the latest allocation
            if (previous_address == old_mem)
            {
                current_offset = previous_offset + new_size;
                std::cout << "Resizing the latest allocation. current_offset is now: " << current_offset << '\n';

                return old_memory;
            }
            else
            {
                void* new_memory { arena_allocate(new_size, alignment) };
                size_t copy_size { (old_size < new_size) ? old_size : new_size };

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

    /**
     * It doesn't really erase any memory nor objects, it only resets the offsets and
     * remaining_space so allocations can be done from the beginning of the buffer again.
     * In other words: All the data in there will be overwritten once new allocations are made.
     */
    void arena_reset() noexcept
    {
        previous_offset = 0;
        current_offset = 0;
        allocation_amount = 0;
        std::cout << "The ArenaAlloc has been reset!\n";
    }

    void* do_allocate(size_t number_of_bytes, size_t alignment) override
    {
        return arena_allocate(number_of_bytes, alignment);
    }

    // It does nothing
    void do_deallocate(void* p, size_t number_of_bytes, size_t alignment) override {}

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const ArenaAlloc*>(&other);
    }
};

/**
 * Think of this temporal arena as a checkpoint that saves the latest allocation offsets
 * so once the original ArenaAlloc allocates once more and aims to deallocate, it can
 * go back to that previous allocation offset that TempArena saved instead of having to
 * reset the whole buffer
 */
struct TempArena
{
    ArenaAlloc* arena {};
    size_t pre_offset {};
    size_t cur_offset {};

    TempArena() = default;
    TempArena(const TempArena& other) = default;
    TempArena(TempArena&& other) noexcept = default;
    TempArena& operator=(const TempArena& other) = default;
    TempArena& operator=(TempArena&& other) noexcept = default;

    explicit TempArena(ArenaAlloc* ar) noexcept
    : arena(ar),
      pre_offset(arena->previous_offset),
      cur_offset(arena->current_offset)
    {}

    // If you wanna assign a ArenaAlloc to a default-constructed TempArena,
    // or to reuse the same TempArena for a different ArenaAlloc
    void assign_arena(ArenaAlloc* ar)
    {
        arena = ar;
        pre_offset = arena->previous_offset;
        cur_offset = arena->current_offset;

        std::cout << "The TempArena pre_offset and cur_offset are: " << pre_offset << ", " << cur_offset << '\n';
    }

    // Cut ties with the current ArenaAlloc manually, so it can be used by
    // another one before the TempArena is destroyed
    void temp_arena_end()
    {
        if ((arena == nullptr) || (cur_offset == 0))
        {
            std::cout << "This temporal arena is empty. No changes will be performed to the ArenaAlloc.\n";
            pre_offset = 0;
            cur_offset = 0;
            arena = nullptr;
            return;
        }

        arena->previous_offset = pre_offset;
        arena->current_offset = cur_offset;
        pre_offset = 0;
        cur_offset = 0;
        arena = nullptr;
    }
};

/**
 * It holds the start_offset and padding of the previous allocation
 */
struct StackHeader
{
    size_t start_offset {};
    size_t padding {};
};

/**
 * This allocator allocates and deallocates in a LIFO way: Last-in, first-out.
 * This means that allocations can dellaocated one by one starting by the last one.
 * The whole memeory can also be deallocated at once like in ArenaAlloc.
 */
struct StackAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    size_t buffer_length {};
    size_t start_offset {};
    size_t current_offset {};
    size_t allocation_amount {};

    StackAlloc() = default;
    StackAlloc(const StackAlloc& other) = default;
    StackAlloc(StackAlloc&& other) noexcept = default;
    StackAlloc& operator=(const StackAlloc& other) = default;
    StackAlloc& operator=(StackAlloc&& other) noexcept = default;

    explicit StackAlloc(void* buff, size_t buff_len) noexcept
    : buffer(static_cast<uint8_t*>(buff)),
      buffer_length(buff_len)
    {}

    // Assign buffer and length in case the StackAlloc was already default-constructed
    // or all its memory was freed
    void assign_buffer(void* backing_buffer, size_t backing_buffer_length) noexcept
    {
        buffer = static_cast<uint8_t*>(backing_buffer);
        buffer_length = backing_buffer_length;
        current_offset = 0;
        allocation_amount = 0;
    }

    /**
     * In the allocation the first element is the allocated bytes, then the padding and
     * inside of it is the header in the last bytes of it.
     * current_offset is located after the allocated bytes without padding.
     * previous_offset is the same but for the previous allocation.
     * StackHeader stores the padding and start_offset of the current allocation.
     */
    void* stack_allocate(size_t size, size_t alignment) noexcept
    {
        std::cout << "###############   STACK ALLOCATION!   ################\n";

        if(buffer == nullptr)
        {
            return nullptr;
        }
        else if (size > 0)
        {
            uintptr_t start {};
            uintptr_t current_address {};
            uintptr_t next_address {};
            size_t padding {};
            StackHeader *header {};

            std::cout << "size: " << size << '\n';
            std::cout << "alignment: " << alignment << '\n';
            std::cout << "start_offset and current_offset before allocation: " << start_offset << ", " << current_offset << '\n';
            std::cout << "remaining space before allocation: " << (buffer_length - current_offset) << '\n';
            std::cout << "allocation_amount before: " << allocation_amount << '\n';

            // As the padding is 8 bits (1 byte), the largest alignment that can
            // be used is 128 bytes. This kind of alignment may be needed for any kind of SIMD instruction
            if (alignment > 128)
            {
                alignment = 128;
            }

            start = reinterpret_cast<uintptr_t>(buffer);
            current_address = start + static_cast<uintptr_t>(current_offset);
            padding = calc_padding_with_header(current_address, sizeof(StackHeader), static_cast<uintptr_t>(alignment));

            std::cout << "padding from the previous allocation: " << padding << '\n';

            // Stack allocator is out of memory
            if ((current_offset + padding + size) > buffer_length)
            {
                return nullptr;
            }

            next_address = current_address + static_cast<uintptr_t>(padding);
            header = reinterpret_cast<StackHeader*>(next_address - static_cast<uintptr_t>(sizeof(StackHeader)));
            header->padding = padding;
            header->start_offset = start_offset; // Store the previous start_offset in the header

            std::cout << "current_adress: " << current_address << '\n';
            std::cout << "next_adress: " << next_address << '\n';
            std::cout << "next offset: " << (next_address - start) << '\n';
            std::cout << "header offset: " << (reinterpret_cast<uintptr_t>(header) - start) << '\n';
            std::cout << "sizeof(StackHeader): " << sizeof(StackHeader) << '\n';
            std::cout << "header->start_offset: " << (header->start_offset) << '\n';
            std::cout << "header->padding: " << (header->padding) << '\n';

            start_offset = static_cast<size_t>(next_address - start); // Store the previous offset
            current_offset = start_offset + size;
            allocation_amount += 1;

            std::cout << "start_offset and current_offset after allocation: " << start_offset << ", " << current_offset << '\n';
            std::cout << "allocation_amount after: " << allocation_amount << '\n';
            std::cout << "remaining space: " << (buffer_length - current_offset) << "\n\n\n";

            return reinterpret_cast<void*>(next_address);
        }
        else
        {
            std::cout << "Allocation size is 0, returning nullptr.\n";
            return nullptr;
        }
    }

    /**
     * Making use of the previous_offset and the StackHeader it updates the offsets
     * to the ones of the previous allocation.
     */
    void stack_deallocate() noexcept
    {
        std::cout << "----------------   STACK DEALLOCATION!   ------------------\n";

        if((buffer != nullptr) && (current_offset > 0))
        {
            uintptr_t start {};
            uintptr_t current_address {};
            StackHeader* header {};
            size_t temp_prev_offset {};

            start = reinterpret_cast<uintptr_t>(buffer);
            current_address = static_cast<uintptr_t>(start_offset) + start;

            std::cout << "allocation_amount before: " << allocation_amount << '\n';
            std::cout << "start_offset and current_offset before allocation: " << start_offset << ", " << current_offset << '\n';
            std::cout << "current_address: " << current_address << '\n';

            header = reinterpret_cast<StackHeader*>(current_address - static_cast<uintptr_t>(sizeof(StackHeader)));
            temp_prev_offset = start_offset - header->padding;

            std::cout << "header offset " << (reinterpret_cast<uintptr_t>(header) - start) << '\n';
            std::cout << "header: " << current_address << '\n';
            std::cout << "header->padding: " << header->padding << '\n';
            std::cout << "header->start_offset: " << header->start_offset << '\n';

            current_offset = temp_prev_offset;
            start_offset = header->start_offset;
            allocation_amount -= 1;

            std::cout << "start_offset and current_offset after allocation: " << start_offset << ", " << current_offset << '\n';
            std::cout << "allocation_amount after: " << allocation_amount << '\n';
            std::cout << "remaining space: " << (buffer_length - current_offset) << "\n\n\n";
        }
        else
        {
            std::cout << "The StackAlloc is empty already\n\n\n";
        }
    }

    /**
     * Changes the size of a certain allocation pointed to by ptr.
     * If the allocation to resize isn't the latest one, a new allocation is
     * made with the new size at the end of the stack.
    */
    void* stack_resize(void* ptr, size_t old_size, size_t new_size, size_t alignment)
    {
        std::cout << "*****************   STACK RESIZING!   ******************\n";

        if (ptr == nullptr)
        {
            return stack_allocate(new_size, alignment);
        }
        else if ((new_size == 0) || (old_size == new_size))
        {
            return ptr;
        }
        else
        {
            uintptr_t start {};
            uintptr_t end {};
            uintptr_t current_address {};
            size_t min_size { (old_size < new_size) ? old_size : new_size };
            void *new_ptr {};
            StackHeader* header {};

            start = reinterpret_cast<uintptr_t>(buffer);
            end = start + static_cast<uintptr_t>(buffer_length);
            current_address = reinterpret_cast<uintptr_t>(ptr);

            std::cout << "start_offset and current_offset before resizing: " << start_offset << ", " << current_offset << '\n';

            HDSA_BASIC_ASSERT(((start <= current_address) && (current_address < end)), "ptr is out of bounds memory address, called from stack_resize()\n");

            // Treat as a double pop
            if (current_address >= (start + static_cast<uintptr_t>(current_offset)))
            {
                return nullptr;
            }

            header = reinterpret_cast<StackHeader*>(current_address - static_cast<uintptr_t>(sizeof(StackHeader)));

            std::cout << "header offset " << (reinterpret_cast<uintptr_t>(header) - start) << '\n';
            std::cout << "header: " << static_cast<void*>(header) << '\n';
            std::cout << "header->padding: " << header->padding << '\n';
            std::cout << "header->start_offset: " << header->start_offset << '\n';

            if ((current_address - start) == static_cast<uintptr_t>(start_offset))
            {
                if ((start_offset + new_size) <= buffer_length)
                {
                    current_offset = start_offset + new_size;
                    std::cout << "remaining space after resizing: " << (buffer_length - current_offset) << '\n';
                    std::cout << "start_offset and current_offset after: " << start_offset << ", "<< current_offset << '\n';
                    std::cout << "allocation_amount after: " << allocation_amount << "\n\n\n";
                    return ptr;
                }

                std::cerr << "There's not enough space for the new allocation size on the latest allocation.\n";
                return ptr;
            }

            new_ptr = stack_allocate(new_size, alignment);
            memmove(new_ptr, ptr, min_size);

            std::cout << "start_offset and current_offset after resizing: " << start_offset << ", " << current_offset << '\n';

            return new_ptr;
        }
    }

    // It resets the offsets so allocations happen from the beginning of the buffer
    // Basically, the same behavior as in ArenaAlloc
    void stack_reset()
    {
        current_offset = 0;
        start_offset = 0;
        allocation_amount = 0;
    }

    void* do_allocate(size_t number_of_bytes, size_t alignment) override
    {
        return stack_allocate(number_of_bytes, alignment);
    }

    void do_deallocate(void* p, size_t number_of_bytes, size_t alignment) override
    {
        stack_deallocate();
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const StackAlloc*>(&other);
    }
};

struct PoolFreeNode
{
    PoolFreeNode* next {};
};

struct PoolAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    size_t buffer_length {};
    size_t chunk_size {};
    PoolFreeNode* head {};
    size_t allocation_amount {};

    PoolAlloc() = default;
    PoolAlloc(const PoolAlloc& other) = default;
    PoolAlloc(PoolAlloc&& other) noexcept = default;
    PoolAlloc& operator=(const PoolAlloc& other) = default;
    PoolAlloc& operator=(PoolAlloc&& other) noexcept = default;

    explicit PoolAlloc(void* backing_buffer, size_t backing_buffer_length, size_t chunk_s, size_t chunk_alignment) noexcept
    {
        assign_buffer(backing_buffer, backing_buffer_length, chunk_s, chunk_alignment);
    }

    /**
     * It assigns "backing_buffer" to the PoolAlloc and makes sure each chunk is of
     * the same size and alignment.
     */
    void assign_buffer(void* backing_buffer, size_t backing_buffer_length, size_t chunk_s, size_t chunk_alignment) noexcept
    {
        std::cout << "########################### POOL ASIGNMENT #################################\n";

        std::cout << "buffer before assign_buffer(): " << static_cast<void*>(buffer) << '\n';
        std::cout << "buffer_length before assign_buffer(): " << buffer_length << '\n';
        std::cout << "chunk_size before assign_buffer(): " << chunk_size << '\n';
        std::cout << "chunk alignment: " << chunk_alignment << '\n';
        std::cout << "head before assign_buffer(): " << ((head != nullptr) ? static_cast<void*>(head) : "nullptr") << '\n';

        // Align "backing_buffer" to the specified chunk alignment
        uintptr_t initial_start { reinterpret_cast<uintptr_t>(backing_buffer) };
        uintptr_t start { align_forward_uintptr(initial_start, static_cast<uintptr_t>(chunk_alignment)) };
        backing_buffer_length -= static_cast<size_t>(start - initial_start);

        std::cout << "initial_start: " << initial_start << '\n';
        std::cout << "start: " << start << '\n';
        std::cout << "chunk_s before alignment: " << chunk_s << '\n';

        // Align the external chunk size up to the required chunk_alignment
        chunk_s = align_forward_size(chunk_s, chunk_alignment);

        std::cout << "chunk_s after alignment: " << chunk_s << '\n';

        HDSA_BASIC_ASSERT((chunk_s >= sizeof(PoolFreeNode)), "The provided chunk size is too small.\n");
        HDSA_BASIC_ASSERT((backing_buffer_length >= chunk_s), "The size of the backing buffer is smaller than the provided chunk size.\n");

        // Store the adjusted parameters
        buffer = static_cast<uint8_t*>(backing_buffer);
        buffer_length = backing_buffer_length;
        chunk_size = chunk_s;
        head = nullptr;

        std::cout << "buffer after assign_buffer(): " << static_cast<void*>(buffer) << '\n';
        std::cout << "buffer_length after assign_buffer(): " << buffer_length << '\n';
        std::cout << "chunk_size after assign_buffer(): " << chunk_size << '\n';

        pool_reset();
    }

    /**
     * It sets every node to be free, one by one.
     */
    void pool_reset() noexcept
    {
        std::cout << ".............................. POOL RESET ..................................\n";

        size_t chunk_count { buffer_length / chunk_size };

        std::cout << "buffer_length: " << buffer_length << '\n';
        std::cout << "chunk_size: " << chunk_size << '\n';
        std::cout << "chunk_count: " << chunk_count << '\n';

        // Set all chunks to be free
        for (size_t i {}; i < chunk_count; i++)
        {
            void* ptr { static_cast<void*>(&buffer[i * chunk_size]) };
            PoolFreeNode* node { reinterpret_cast<PoolFreeNode*>(ptr) };

            // Push a node into the free chunk list
            node->next = head;
            head = node;

            std::cout << "current head being set 'free': " << static_cast<void*>(head) << '\n';
        }

        allocation_amount = 0;
    }

    /**
     * It disconnects the current "head" from the list of free chunks and returns it
     */
    void* pool_allocate() noexcept
    {
        std::cout << "************************** POOL ALLOCATION ****************************\n";

        // Get the latest free node
        PoolFreeNode* node { head };

        if (node == nullptr)
        {
            return nullptr;
        }

        std::cout << "head before allocation: " << static_cast<void*>(head) << '\n';

        // Pop the free node
        head = head->next;

        std::cout << "head after allocation: " << static_cast<void*>(head) << '\n';
        std::cout << "node after allocation: " << static_cast<void*>(node) << '\n';

        allocation_amount += 1;

        return node;
    }

    /**
     * It connects the chunk being pointed at by ptr into the list of free chunks
     */
    void pool_deallocate(void* ptr) noexcept
    {
        std::cout << "------------------------------ POOL DEALLOCATION ----------------------------------\n";

        PoolFreeNode* node {};

        std::cout << "head before deallocation: " << head << '\n';

        void* start { buffer };
        void* end { static_cast<void*>(&buffer[buffer_length]) }; // Possible off-by-one bug on buffer_length

        if (ptr == nullptr)
        {
            return;
        }

        HDSA_BASIC_ASSERT((start <= ptr && ptr < end), "ptr is out of the bounds of the PoolAlloc's buffer.\n");

        // Push the free node
        node = reinterpret_cast<PoolFreeNode*>(ptr);
        node->next = head;
        head = node;
        allocation_amount -= 1;

        std::cout << "head after deallocation: " << head << '\n';
    }

    void* do_allocate(size_t number_of_bytes, size_t alignment) override
    {
        return pool_allocate();
    }

    void do_deallocate(void* p, size_t number_of_bytes, size_t alignment) override
    {
        pool_deallocate(p);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const PoolAlloc*>(&other);
    }
};


struct FreeListHeader
{
    size_t block_size {};
    size_t padding {};

};

struct FreeListNode
{
    size_t block_size {}; // block_size includes the size of FreeListNode
    FreeListNode* next {};
};

enum struct PlacementPolicy : size_t
{
    find_first = 0,
    find_best
};

struct FreeListAlloc : public std::pmr::memory_resource
{
    uint8_t* buffer {};
    size_t size {};
    size_t used_bytes {};
    FreeListNode* head {};
    PlacementPolicy policy { PlacementPolicy::find_best };

    FreeListAlloc() = default;
    FreeListAlloc(const FreeListAlloc& other) = default;
    FreeListAlloc(FreeListAlloc&& other) noexcept = default;
    FreeListAlloc& operator=(const FreeListAlloc& other) = default;
    FreeListAlloc& operator=(FreeListAlloc&& other) noexcept = default;

    explicit FreeListAlloc(void* data, size_t new_size, PlacementPolicy p) noexcept
    : buffer { static_cast<uint8_t*>(data) },
      size { new_size }
    {
        free_list_reset(p);
    }

    void free_list_reset(PlacementPolicy pol)
    {
        std::cout << ".............................. FREE LIST RESET ..................................\n";

        used_bytes = 0;
        FreeListNode* first_node { reinterpret_cast<FreeListNode*>(buffer) };
        first_node->block_size = size;
        first_node->next = nullptr;
        head = first_node;
        policy = pol;

        std::cout << "used_bytes: " << used_bytes << '\n';
        std::cout << "first_node: " << static_cast<void*>(first_node) << '\n';
        std::cout << "first_node->block_size: " << first_node->block_size << '\n';
        std::cout << "first_node->next: " << static_cast<void*>(first_node->next) << '\n';
        std::cout << "policy: " << ((policy == PlacementPolicy::find_best) ? "find_best" : "find_first") << '\n';
    }

    /**
     * It assigns a new buffer for an existing FreeListAlloc object and resets all the nodes
     */
    void assign_buffer(void* data, size_t new_size, PlacementPolicy p)
    {
        buffer = static_cast<uint8_t*>(data);
        size = new_size;
        free_list_reset(p);
    }

    /**
     * It looks for the first free node that has at least the required size plus the size of FreeListHeader
     */
    FreeListNode* free_list_find_first(size_t allocation_size, size_t alignment, size_t* allocation_padding, FreeListNode** allocation_previous_node)
    {
        FreeListNode* node { head };
        FreeListNode* previous_node {};
        size_t padding {};

        std::cout << "head and node: " << static_cast<void*>(node) << '\n';

        if (node == nullptr)
        {
            return nullptr;
        }

        while (node != nullptr)
        {
            padding = calc_padding_with_header(reinterpret_cast<uintptr_t>(node), static_cast<uintptr_t>(alignment), sizeof(FreeListHeader));
            size_t required_space { allocation_size + padding };
            std::cout << "padding: " << padding << '\n';
            std::cout << "allocation_size + padding: " << required_space << '\n';

            if (node->block_size >= required_space)
            {
                std::cout << "Found node: " << static_cast<void*>(node) << '\n';
                std::cout << "Found node->block_size: " << node->block_size << '\n';
                break;
            }

            previous_node = node;
            node = node->next;
            std::cout << "previous_node: " << static_cast<void*>(previous_node) << '\n';
            std::cout << "node: " << static_cast<void*>(node) << '\n';
        }

        if (allocation_padding != nullptr)
        {
            *allocation_padding = padding;
            std::cout << "*allocation_padding: " << *allocation_padding << '\n';
        }
        else
        {
            std::cout << "*allocation_padding: 0\n";
        }

        if (allocation_previous_node != nullptr)
        {
            *allocation_previous_node = previous_node;
        }

        std::cout << "*allocation_previous_node: " << static_cast<void*>(*allocation_previous_node) << '\n';
        std::cout << "Chosen node: " << static_cast<void*>(node) << '\n';

        return node;
    }

    /**
     * It looks for the node has the smallest size that can fit the required size plus the size of FreeListHeader
     */
    FreeListNode* free_list_find_best(size_t allocation_size, size_t alignment, size_t* allocation_padding, FreeListNode** allocation_previous_node)
    {
        std::cout << ".............................. FREE LIST FIND BEST ..................................\n";

        FreeListNode* node { head };
        FreeListNode* previous_node {};
        FreeListNode* best_node {};
        size_t padding {};
        size_t smallest_difference { ~static_cast<size_t>(0) }; // Maximum representable number by size_t,
        // which for 64 bits CPUs is 18446744073709551615, or ((2^64) - 1)
        std::cout << "head and node: " << static_cast<void*>(node) << '\n';

        if (node == nullptr)
        {
            return nullptr;
        }

        while (true)
        {
            padding = calc_padding_with_header(reinterpret_cast<uintptr_t>(node), static_cast<uintptr_t>(alignment), sizeof(FreeListHeader));
            size_t required_space { allocation_size + padding };
            std::cout << "padding: " << padding << '\n';
            std::cout << "allocation_size + padding: " << required_space << '\n';

            if ((node->block_size >= required_space) && ((node->block_size - required_space) < smallest_difference))
            {
                best_node = node;
                std::cout << "best_node: " << static_cast<void*>(best_node) << '\n';

                // Different line: Updates the smallest_difference to a smaller value every time the if is true
                smallest_difference = node->block_size - required_space;
                std::cout << "smallest_difference: " << smallest_difference << '\n';
            }

            if ((smallest_difference == 0) || (node->next == nullptr))
            {
                std::cout << "previous_node: " << static_cast<void*>(previous_node) << '\n';
                std::cout << "node: " << static_cast<void*>(node) << '\n';
                break;
            }

            previous_node = node;
            node = node->next;
            std::cout << "previous_node: " << static_cast<void*>(previous_node) << '\n';
            std::cout << "node: " << static_cast<void*>(node) << '\n';
        }

        if (allocation_padding != nullptr)
        {
            *allocation_padding = padding;
            std::cout << "*allocation_padding: " << *allocation_padding << '\n';
        }
        else
        {
            std::cout << "*allocation_padding: 0\n";
        }

        if (*allocation_previous_node != nullptr)
        {
            *allocation_previous_node = previous_node;
        }

        std::cout << "*allocation_previous_node: " << static_cast<void*>(*allocation_previous_node) << '\n';
        std::cout << "Chosen best_node: " << static_cast<void*>(best_node) << '\n';

        return best_node;
    }

    /**
     * It adds new_node to the "free list" of nodes by making it the next of head. If new_node is null this function does nothing
     */
    void free_list_node_insert(FreeListNode* new_node)
    {
        if (new_node != nullptr)
        {
            if (head != nullptr)
            {
                if (head != new_node)
                {
                    new_node->next = head->next;
                    head->next = new_node;
                }
                else
                {
                    std::cout << "The tail and new_node are the same, no new node will be inserted in the free list.\n";
                    return;
                }
            }
            else
            {
                head = new_node;
            }
        }
        else
        {
            std::cout << "new_node is null, no nodes will be inserted in the list.\n";
        }
    }

    /**
     * It removes del_node from the "free list" of nodes
     */
    void free_list_node_remove(FreeListNode* prev_node, FreeListNode* del_node)
    {
        std::cout << "************************** FREE LIST NODE REMOVE ****************************\n";

        if (del_node != nullptr)
        {
            if (head != nullptr)
            {
                if (prev_node != nullptr)
                {
                    if (prev_node == head)
                    {
                        if (prev_node == del_node)
                        {
                            head = del_node->next;
                            std::cout << "head: " << static_cast<void*>(head) << '\n';
                        }
                        else
                        {
                            head->next = del_node->next;
                            std::cout << "head->next: " << static_cast<void*>(head->next) << '\n';
                        }
                    }
                    else
                    {
                        if (prev_node == del_node)
                        {
                            prev_node = del_node->next;
                            std::cout << "prev_node: " << static_cast<void*>(prev_node) << '\n';
                        }
                        else
                        {
                            prev_node->next = del_node->next;
                            std::cout << "prev_node->next: " << static_cast<void*>(prev_node->next) << '\n';
                        }
                    }
                }
                else
                {
                    head = del_node->next;
                    std::cout << "head: " << static_cast<void*>(head) << '\n';
                }
            }
            else
            {
                std::cout << "head is null, no nodes will be removed from the list.\n";
                return;
            }
        }
        else
        {
            std::cout << "del_node is null, no nodes will be removed from the list.\n";
        }
    }

    /**
     * Allocates within a node according to the PlacementPolicy in use
     */
    void* free_list_allocate(size_t allocation_size, size_t alignment)
    {
        size_t padding {};
        FreeListNode* previous_node {};
        FreeListNode* node {};
        size_t alignment_padding {};
        size_t required_space {};
        size_t remaining_node_space {}; // Remaining space inside the node after the allocation size and padding were substracted from it
        FreeListHeader* header_ptr {};

        std::cout << "************************** FREE LIST ALLOCATION ****************************\n";
        std::cout << "allocation_size before: " << allocation_size << '\n';
        std::cout << "alignment before: " << alignment << '\n';

        if (allocation_size < sizeof(FreeListNode))
        {
            allocation_size = sizeof(FreeListNode);
            std::cout << "allocation_size after: " << allocation_size << '\n';
        }

        if (alignment < alignof(FreeListNode))
        {
            alignment = alignof(FreeListNode);
            std::cout << "alignment after: " << alignment << '\n';
        }

        if (policy == PlacementPolicy::find_best)
        {
            node = free_list_find_best(allocation_size, alignment, &padding, &previous_node);
        }
        else
        {
            node = free_list_find_first(allocation_size, alignment, &padding, &previous_node);
        }

        HDSA_BASIC_ASSERT((node != nullptr), "Can't allocate more space for the FreeListAlloc.\n");

        required_space = allocation_size + padding;
        std::cout << "required_space: " << required_space << '\n';

        // Thanks to the functions free_list_find_best and free_list_find_first (node->block_size - required_space) will always be positive or 0
        remaining_node_space = node->block_size - required_space;
        std::cout << "remaining_node_space: " << remaining_node_space << '\n';

        // If remaining_space is bigger than the size of a FreeListNode, then proceed to create a new node and add it to the free list of nodes
        if (remaining_node_space > sizeof(FreeListNode))
        {
            FreeListNode* new_node { reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(node) + static_cast<uintptr_t>(required_space)) };
            new_node->block_size = remaining_node_space;
            std::cout << "new_node: " << static_cast<void*>(new_node) << '\n';
            free_list_node_insert(new_node);
        }

        // Remove node from the free list of nodes, AKA it's now being used
        free_list_node_remove(previous_node, node);

        // Set the header for the node being allocated. This header is located (most of the time) in a different address from node
        // The whole node->block_size is supposed to be able to hold the padding and the bytes being allocated, so the padding starts
        // from the address of node, the data starts after the padding, and the header_ptr is right before the data, hence why
        // "alignment_padding" is smaller than padding
        alignment_padding = padding - sizeof(FreeListHeader);
        header_ptr = reinterpret_cast<FreeListHeader*>(reinterpret_cast<uintptr_t>(node) + static_cast<uintptr_t>(alignment_padding));

        // Unlike with free blocks, block_size for a used block is only the allocation size plus the size of the header
        header_ptr->block_size = allocation_size + sizeof(FreeListHeader);
        header_ptr->padding = alignment_padding;

        FreeListNode* result = reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(header_ptr) + static_cast<uintptr_t>(sizeof(FreeListHeader)));
        used_bytes += required_space;

        std::cout << "alignment_padding: " << alignment_padding << '\n';
        std::cout << "header_ptr: " << static_cast<void*>(header_ptr) << '\n';
        std::cout << "header_ptr->block_size: " << header_ptr->block_size << '\n';
        std::cout << "header_ptr->padding: " << header_ptr->padding << '\n';
        std::cout << "used_bytes: " << used_bytes << '\n';
        std::cout << "head: " << static_cast<void*>(head) << '\n';
        std::cout << "previous_node: " << static_cast<void*>(previous_node) << '\n';
        std::cout << "Result: " << static_cast<void*>(result) << '\n';

        return result;
    }

    /**
     * Tries to combine free_node and_free_node->next into a single free block
     * and tries to do the same with previous_node and free_node afterwards
     */
    void free_list_coalescence(FreeListNode* previous_node, FreeListNode* free_node)
    {
        std::cout << "------------------------------ FREE LIST COALESCENCE ----------------------------------\n";

        if ((free_node != nullptr) &&(previous_node != nullptr ))
        {
            if
            (
                (free_node->next != nullptr) &&
                (reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(free_node) + static_cast<uintptr_t>(free_node->block_size)) == free_node->next)
            )
            {
                free_node->block_size += free_node->next->block_size;
                free_list_node_remove(free_node, free_node->next);
                std::cout << "Combining free_node and free_node->next.\n";
            }

            if
            (
                (previous_node->next != nullptr) &&
                (reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(previous_node) + static_cast<uintptr_t>(previous_node->block_size)) == free_node)
            )
            {
                previous_node->block_size += free_node->block_size;
                free_list_node_remove(previous_node, free_node);
                std::cout << "Combining previous_node and free_node.\n";
            }

            // If head and previous_node are the only nodes in the free list, and both are next to each other, combine them into one
            if
            ((head->next == previous_node) && (previous_node->next == nullptr))
            {
                if (reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(previous_node) + static_cast<uintptr_t>(previous_node->block_size)) == head)
                {
                    previous_node->block_size += head->block_size;
                    free_list_node_remove(previous_node, head);
                    std::cout << "Combining head into previous_node.\n";
                }
                else if (reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(head) + static_cast<uintptr_t>(head->block_size)) == previous_node)
                {
                    head->block_size += previous_node->block_size;
                    free_list_node_remove(head, previous_node);
                    std::cout << "Combining previous_node into head.\n";
                }
            }

            std::cout << "free_node->next isn't null. free_node->block_size: " << free_node->block_size << '\n';
            std::cout << "previous_node->next isn't null. previous_node->block_size: " << previous_node->block_size << '\n';
        }
        else
        {
            std::cout << "One or both nodes are null, coalescence isn't possible.\n";
        }
    }

    /**
     * Adds the block to the free list of nodes, reduces the amount of space in use and merges two contiguous blocks if possible
     */
    void free_list_deallocate(void* ptr)
    {
        std::cout << "------------------------------ FREE LIST DEALLOCATION ----------------------------------\n";

        FreeListHeader* header {};
        FreeListNode* free_node {};
        FreeListNode* node {};

        if (ptr == nullptr)
        {
            std::cout << "The pointer to deallocate from the FreeListAlloc is null.\n";
            return;
        }

        // Put the free_node where the header of ptr is
        header = reinterpret_cast<FreeListHeader*>(reinterpret_cast<uintptr_t>(ptr) - static_cast<uintptr_t>(sizeof(FreeListHeader)));
        free_node = reinterpret_cast<FreeListNode*>(reinterpret_cast<uintptr_t>(header) - static_cast<uintptr_t>(header->padding));
        free_node->block_size = header->block_size + header->padding;
        free_node->next = nullptr;

        std::cout << "header: " << static_cast<void*>(header) << '\n';
        std::cout << "free_node: " << static_cast<void*>(free_node) << '\n';
        std::cout << "free_node->block_size: " << free_node->block_size << '\n';
        std::cout << "node/head: " << static_cast<void*>(node) << '\n';

        free_list_node_insert(free_node);

        std::cout << "used_bytes before deallocation: " << used_bytes << '\n';

        used_bytes -= free_node->block_size;
        free_list_coalescence(head, free_node);

        std::cout << "used_bytes after deallocation: " << used_bytes << '\n';
    }


    void* do_allocate(size_t number_of_bytes, size_t alignment) override
    {
        return free_list_allocate(number_of_bytes, alignment);
    }

    void do_deallocate(void* p, size_t number_of_bytes, size_t alignment) override
    {
        free_list_deallocate(p);
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const FreeListAlloc*>(&other);
    }
};

} // namespace hdsa end

#endif // HDSA_PMR_HPP
