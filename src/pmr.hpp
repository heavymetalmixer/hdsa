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

bool is_power_of_two(size_t x)
{
    if (x == 0) { return false; }

    return ((x & (x - 1)) == 0);
}

// If x isn't some power of 2, then increase it until it becomes a power of 2
// It returns 1 when the requested_size is 0, as this function is used for finding
// alignments, and there's no aligment 0
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

// A block is 16 pages of 4K each
constexpr size_t page_size { 4096 };
constexpr size_t block_size { page_size * 16 }; // 65536

struct VirtualPageHeader
{
    size_t start_offset {};
};

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

    ~VirtualPageAlloc()
    {
        std::cout << "A VirtualPageAlloc is about to be destroyed.\n";
    }

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

        std::cout << "real_size: " << real_size << '\n';
        std::cout << "Amount of blocks to allocate: " << blocks << '\n';

        // OS selection for the virtual memory allocation function
    #if defined(_WIN32) || defined(_WIN64)
        buffer = static_cast<uint8_t*>(VirtualAlloc(static_cast<void*>(buffer + buffer_length), block_size * blocks, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    #elif defined(__linux__) || defined(__APPLE__) || defined(__MACH__) || defined(__unix__) || defined(__unix)
        buffer = static_cast<uint8_t*>(mmap(static_cast<void*>(buffer + buffer_length()), block_size * blocks, PROT_READ+PROT_WRITE, MAP_ANONYMOUS+MAP_SHARED, -1, 0));
    #endif

        block_amount = blocks;
        buffer_length = block_amount * block_size;
        end_address = reinterpret_cast<uint8_t*>(reinterpret_cast<uintptr_t>(buffer) + static_cast<uintptr_t>(buffer_length));
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

    explicit ArenaAlloc(void* buff, size_t buffer_size) noexcept
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

    size_t remaining_storage()
    {
        return buffer_length - current_offset;
    }

    /**
     * Align ptr to alignment to prevent slowdowns caused by un-aligned
     * memory access by the CPU.
     */
    uintptr_t align_forward(uintptr_t ptr, size_t alignment = alignof(std::max_align_t)) noexcept
    {
        uintptr_t p, a, modulo;

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

    void* arena_allocate(size_t number_of_bytes, size_t alignment = alignof(std::max_align_t)) noexcept
    {
        HDSA_BASIC_ASSERT((number_of_bytes > 0), "Using 0 as buffer size is not allowed.\n");

        std::cout << "###############   ALLOCATION!   ################\n";

        uintptr_t start, current_address, aligned_address;
        size_t aligned_offset;

        std::cout << "allocation size: " << number_of_bytes << '\n';
        std::cout << "allocation_amount before: " << allocation_amount << '\n';
        std::cout << "remaining_space before: " << remaining_storage() << '\n';
        std::cout << "previous_offset before: " << previous_offset << '\n';
        std::cout << "current_offset before: " << current_offset << '\n';

        start = reinterpret_cast<uintptr_t>(buffer);
        current_address = start + static_cast<uintptr_t>(current_offset);
        aligned_address = align_forward(current_address, alignment);

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
    void* arena_resize(void* old_memory, size_t old_size, size_t new_size, size_t alignment = alignof(std::max_align_t)) noexcept
    {
        std::cout << "***************   RESIZING!   ****************\n";

        uintptr_t previous_address, start, end, old_mem;

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
 * so once the original ArenaAlloc allocates once more and deallocates, it can
 * go back to that previous allocation offset that TempArena saved instead of having to
 * reset the whole buffer
 */
struct TempArena
{
	ArenaAlloc* arena { nullptr };
	size_t pre_offset {};
	size_t cur_offset {};

    TempArena() = default;

    explicit TempArena(ArenaAlloc* ar) noexcept
    : arena(ar),
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
        if ((arena == nullptr) || (cur_offset == 0))
        {
            std::cout << "This temporal arena is empty. No changes will be performed to the Linear memory resource.\n";
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

    // If you wanna assign a ArenaAlloc to a default-constructed TempArena,
    // or to reuse the same TempArena for a different ArenaAlloc
    void assign_arena(ArenaAlloc* ar)
    {
        arena = ar;
        pre_offset = arena->previous_offset;
        cur_offset = arena->current_offset;
    }
};

/**
 * It holds the start_offset and padding of the previous allocation
 */
struct StackHeader
{
    size_t start_offset {};
    size_t padding;
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

    explicit StackAlloc(void* buff, size_t buff_len) noexcept
    : buffer(static_cast<uint8_t*>(buff)),
      buffer_length(buff_len)
    {}

    StackAlloc(const StackAlloc& other) = default;
    StackAlloc(StackAlloc&& other) noexcept = default;
    StackAlloc& operator=(const StackAlloc& other) = default;
    StackAlloc& operator=(StackAlloc&& other) noexcept = default;

    ~StackAlloc()
    {
        std::cout << "A StackAlloc is about to be destroyed. It has " << buffer_length << " bytes assigned to it.\n";
    }

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
     * It returns the padding plus the size of the header of the current allocation
     */
    size_t calc_padding_with_header(uintptr_t ptr, size_t header_size, uintptr_t alignment = alignof(std::max_align_t))
    {
        uintptr_t p, a, modulo, padding, needed_space;

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

        // Same logic as 'align_forward'
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

    /**
     * In the allocation the first element is the allocated bytes, then the padding and
     * inside of it is the header in the last bytes of it.
     * current_offset is located after the allocated bytes without padding.
     * previous_offset is the same but for the previous allocation.
     * StackHeader stores the padding and start_offset of the current allocation.
     */
    void* stack_push(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept
    {
        std::cout << "###############   ALLOCATION!   ################\n";

        if(buffer == nullptr)
        {
            return nullptr;
        }
        else if (size > 0)
        {
            uintptr_t start, current_address, next_address;
            size_t padding;
            StackHeader *header;

            std::cout << "size: " << size << '\n';
            std::cout << "alignment: " << alignment << '\n';
            std::cout << "start_offset and current_offset before allocation: " << start_offset << ", " << current_offset << '\n';
            std::cout << "remaining space before allocation: " << (buffer_length - current_offset) << '\n';
            std::cout << "allocation_amount before: " << allocation_amount << '\n';

            // As the padding is 8 bits (1 byte), the largest alignment that can
            // be used is 128 bytes
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
            header = reinterpret_cast<StackHeader*>(static_cast<size_t>(next_address) - sizeof(StackHeader));
            header->padding = static_cast<uint8_t>(padding);
            header->start_offset = start_offset; // store the previous start_offset in the header

            std::cout << "current_adress: " << current_address << '\n';
            std::cout << "next_adress: " << next_address << '\n';
            std::cout << "next offset: " << (next_address - start) << '\n';
            std::cout << "header offset: " << (reinterpret_cast<uintptr_t>(header) - start) << '\n';
            std::cout << "sizeof(StackHeader): " << sizeof(StackHeader) << '\n';
            std::cout << "header->start_offset: " << (header->start_offset) << '\n';
            std::cout << "header->padding: " << (header->padding) << '\n';

            start_offset = next_address - start; // Store the previous offset
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
    void stack_pop()
    {
        std::cout << "----------------   DEALLOCATION!   ------------------\n";

        if((buffer != nullptr) && (current_offset > 0))
        {
            uintptr_t start, end, current_address;
            StackHeader* header;
            size_t temp_prev_offset;

            start = reinterpret_cast<uintptr_t>(buffer);
            end = start + static_cast<uintptr_t>(buffer_length);
            // current_address = reinterpret_cast<uintptr_t>(ptr);
            current_address = static_cast<uintptr_t>(start_offset) + start;

            std::cout << "allocation_amount before: " << allocation_amount << '\n';
            std::cout << "start_offset and current_offset before allocation: " << start_offset << ", " << current_offset << '\n';
            std::cout << "current_address: " << current_address << '\n';

            header = reinterpret_cast<StackHeader*>(static_cast<size_t>(current_address) - sizeof(StackHeader));
            temp_prev_offset = start_offset - header->padding;

            std::cout << "header offset " << (reinterpret_cast<uintptr_t>(header) - start) << '\n';
            std::cout << "header: " << current_address << '\n';
            std::cout << "header->padding: " << header->padding << '\n';
            std::cout << "header->start_offset: " << header->start_offset << '\n';

            // HDSA_BASIC_ASSERT((temp_prev_offset == header->previous_offset), "Out of order stack_pop().\n");

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
    void* stack_resize(void* ptr, size_t old_size, size_t new_size, size_t alignment = alignof(std::max_align_t))
    {
        std::cout << "*****************   RESIZING!   ******************\n";

        if (ptr == nullptr)
        {
            return stack_push(new_size, alignment);
        }
        else if ((new_size == 0) || (old_size == new_size))
        {
            return ptr;
        }
        else
        {
            uintptr_t start, end, current_address;
            size_t min_size = (old_size < new_size) ? old_size : new_size;
            void *new_ptr;
            StackHeader* header;

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

            header = reinterpret_cast<StackHeader*>(static_cast<size_t>(current_address) - sizeof(StackHeader));

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

            new_ptr = stack_push(new_size, alignment);
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
        return stack_push(number_of_bytes, alignment);
    }

    void do_deallocate(void* p, size_t number_of_bytes, size_t alignment) override
    {
        stack_pop();
    }

    bool do_is_equal(const std::pmr::memory_resource& other) const noexcept override
    {
        return this == dynamic_cast<const StackAlloc*>(&other);
    }
};


} // namespace hdsa end

#endif // HDSA_PMR_HPP
