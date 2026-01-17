#ifndef HDSA_ALLOCATOR_HPP
#define HDSA_ALLOCATOR_HPP

#include <cstddef>
#include <limits>
#include <iostream>
#include <memory>
#include <new>

namespace hdsa
{

struct Allocator
{
    Allocator() = default;
    Allocator(const Allocator& other) = default;
    Allocator(Allocator&& other) noexcept = default;
    Allocator& operator=(const Allocator& other) = default;
    Allocator& operator=(Allocator&& other) noexcept = default;
    virtual ~Allocator() = default;

    virtual void* do_allocate(std::size_t number_of_bytes, std::size_t alignment = alignof(std::max_align_t)) = 0;

    virtual void do_deallocate(void* buffer, std::size_t number_of_bytes, std::size_t alignment = alignof(std::max_align_t)) = 0;

    virtual bool do_is_equal(const Allocator& other) const noexcept = 0;
};

template<typename T>
struct AllocWrapper
{
    using value_type = T; // Only required alias
    using pointer = value_type*;
    using const_pointer = const value_type*;
    using void_pointer = void*;
    using const_void_pointer = const void*;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    // These last 4 aren't required bur recommended to set as true anyway
    // so it is possible to propagate them between containers on copy, move and swap
    using propagate_on_container_copy_assignment = std::true_type;
    using propagate_on_container_move_assignment = std::true_type;
    using propagate_on_container_swap = std::true_type;
    // using is_always_equal = std::true_type;

    Allocator* alloc;

    // Converts the allocator from one type to another.
    // It was deprecated in C++ 17 and deleted in 20, but the lack of it still causes
    // problems some times for custom allocators that don't use operator new nor delete
    template<typename U>
    struct rebind
    {
        using other = AllocWrapper<U>;
    };

    AllocWrapper() = default;
    AllocWrapper(const AllocWrapper& other) = default;
    AllocWrapper(AllocWrapper&& other) noexcept = default;
    AllocWrapper& operator=(const AllocWrapper& other) = default;
    AllocWrapper& operator=(AllocWrapper&& other) noexcept = default;

    template<typename U>
    AllocWrapper(const AllocWrapper<U>& other) {}

    ~AllocWrapper() = default;

    // Allocates memory for num_of_objects of type T, but doesn't construct them. REQUIRED
    pointer allocate(size_type num_of_objects)
    {
        // Modify later
        numAllocations += num_of_objects;
        return static_cast<pointer>(::operator new(sizeof(value_type) * num_of_objects));
    }

    // Allocates memory for num_of_objects of type T, but doesn't construct them.
    // It starts from the address of the cvp to allocate just right after
    pointer allocate(size_type num_of_objects, const_void_pointer cvp)
    {
        // Modify later
        return allocate(num_of_objects);
    }

    // Deallocates memory for num_of_objects of type T starting from first_object, but doesn't destroy them. REQUIRED
    void deallocate(pointer first_object, size_type num_of_objects) noexcept
    {
        // Modify later
        ::operator delete(first_object, sizeof(value_type) * num_of_objects);
        // ::operator delete(first_object);
    }

    // The largest size that can be allocated
    size_type max_size() const
    {
        return std::numeric_limits<size_type>::max();
    }

    // Calls the constructor for an object U on uninitialized_storage
    template<typename U, typename... Args>
    void construct(U* uninitialized_storage, Args&&... args)
    {
        new(uninitialized_storage) U(std::forward<Args>(args)...);
    }

    // Calls the destructor for an object U on storage
    template<typename U>
    void destroy(U* storage)
    {
        storage->~U();
    }

    // Calls the constructor for an object U on uninitialized_storage
    template<typename U, typename... Args>
    U* new_object(Args&&... args)
    {
        U* uninitialized_storage { *this->allocate}
        new(uninitialized_storage) U(std::forward<Args>(args)...);
    }
};

}

#endif // HDSA_ALLOCATOR_HPP end
