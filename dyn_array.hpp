#ifndef DYN_ARRAY_HPP
#define DYN_ARRAY_HPP

#include <cstddef>
#include <limits>
#include <iterator>
#include <memory>
#include <new>
#include <iostream>
#include <utility>
#include <initializer_list>
#include <source_location>

/**
 * Personal implementation of a Dynamic Array. It uses ::operator new for memory allocation
 * without calling constructors but in the future it will be able to use custom allocators
*/

/**
 * TODO:
 * 1) Test everything, as much as possible.
 * 2) Write the code for the copy and move constructors and operators. DONE!
 * 3) Add the possibility to pass arguments to the T constructors, so they can be constructed in other ways than just default.
 * 4) Investigate about "iterators" and implement them if necessary, that include "const interators". DONE!
 * 5) Investigate about how to make the dynamic arrays retain "pointer stability" for reallocation operations. NOT POSSIBLE!
 * 6) Integrate custom allocators. This implementation is already using placement new and delete to allocate memory without calling constructors nor destructors.
 * 7) Investigate about how "emplace_back" works in std::vector and in-place construction does as well in general, so it can be implemented here. DONE!
 * 8) Look what other std::vector features could be good to have here.
 * 9) Choose which asserts should be changed for exceptions.
 * 10) Investigate about how to construct with Initializer Lists and how to combine it with In-place Construction. DONE!
 * 11) Investigate (later on, not for now) about C++ 20 ranges and see how to implement them here.
 * 12) Investigate about array slicing and see if it can be implemented here.
*/

// I don't like that C asserts don't work on Release builds so I made this one for the same purpose
// If you compare 2 or more values/variables for the "condition", make sure to wrap them in parenthesis
// like this: BASIC_ASSERT((1 > 2), "1 is smaller than 2\n")
// #define BASIC_ASSERT(condition, message)                    \
// if (!condition)                                             \
// {                                                           \
//     std::cerr                                               \
//         << "Assertion failed: " << message << '\n'          \
//         << "  Condition: " << #condition << '\n'            \
//         << "  File: " << __FILE__ << '\n'                   \
//         << "  Function: " << __func__ << '\n'               \
//         << "  Line: " << __LINE__ << '\n';                  \
//     std::abort();                                           \
// }                                                           \

#define BASIC_ASSERT(condition, message)                    \
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

namespace hdsa
{

template<typename DynArray>
class DynArrayIterator final
{
public:
    using difference_type = std::ptrdiff_t;

    using value_type = typename DynArray::value_type;

    using pointer = value_type*;

    using reference = value_type&;

    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::contiguous_iterator_tag;

private:
    pointer m_ptr { nullptr };

public:
    DynArrayIterator(pointer ptr)
    : m_ptr { ptr } {}

    DynArrayIterator& operator++()
    {
        m_ptr++;
        return *this;
    }

    DynArrayIterator operator++(int)
    {
        DynArrayIterator iterator { *this };
        ++(*this);
        return iterator;
    }

    DynArrayIterator& operator--()
    {
        m_ptr--;
        return *this;
    }

    DynArrayIterator operator--(int)
    {
        DynArrayIterator iterator { *this };
        --(*this);
        return iterator;
    }

    DynArrayIterator operator+(int x)
    {
        m_ptr += x;
        return *this;
    }

    DynArrayIterator operator+=(int x)
    {
        m_ptr += x;
        return *this;
    }

    DynArrayIterator operator-(int x)
    {
        m_ptr -= x;
        return *this;
    }

    DynArrayIterator operator-=(int x)
    {
        m_ptr -= x;
        return *this;
    }

    reference operator[](std::size_t position)
    {
        return m_ptr[position];
    }

    pointer operator->()
    {
        return m_ptr;
    }

    reference operator*()
    {
        return *m_ptr;
    }

    bool operator==(const DynArrayIterator& other)
    {
        return m_ptr == other.m_ptr;
    }

    bool operator!=(const DynArrayIterator& other)
    {
        return !(*this == other);
    }

    pointer data()
    {
        return m_ptr;
    }

    friend std::ostream& operator<<(std::ostream& out, const DynArrayIterator& it)
    {
        out << it.m_ptr;
        return out;
    }
};

template<typename DynArray>
class DynArrayConstIterator final
{
public:
    using difference_type = std::ptrdiff_t;

    using value_type = typename DynArray::value_type;

    using pointer = const value_type*;

    using reference = const value_type&;

    using iterator_category = std::random_access_iterator_tag;
    using iterator_concept = std::contiguous_iterator_tag;

private:
    pointer m_ptr { nullptr };

public:
    DynArrayConstIterator(pointer ptr)
    : m_ptr { ptr } {}

    DynArrayConstIterator& operator++()
    {
        m_ptr++;
        return *this;
    }

    DynArrayConstIterator operator++(int)
    {
        DynArrayConstIterator iterator { *this };
        ++(*this);
        return iterator;
    }

    DynArrayConstIterator& operator--()
    {
        m_ptr--;
        return *this;
    }

    DynArrayConstIterator operator--(int)
    {
        DynArrayConstIterator iterator { *this };
        --(*this);
        return iterator;
    }

    DynArrayConstIterator operator+(int x)
    {
        m_ptr += x;
        return *this;
    }

    DynArrayConstIterator operator+=(int x)
    {
        m_ptr += x;
        return *this;
    }

    DynArrayConstIterator operator-(int x)
    {
        m_ptr -= x;
        return *this;
    }

    DynArrayConstIterator operator-=(int x)
    {
        m_ptr -= x;
        return *this;
    }

    reference operator[](std::size_t position)
    {
        return m_ptr[position];
    }

    pointer operator->()
    {
        return m_ptr;
    }

    reference operator*()
    {
        return *m_ptr;
    }

    bool operator==(const DynArrayConstIterator& other)
    {
        return m_ptr == other.m_ptr;
    }

    bool operator!=(const DynArrayConstIterator& other)
    {
        return !(*this == other);
    }

    pointer data()
    {
        return m_ptr;
    }

    friend std::ostream& operator<<(std::ostream& out, const DynArrayConstIterator& c_it)
    {
        out << c_it.m_ptr;
        return out;
    }
};

template<typename T>
class DynArray final
{
public:
    using value_type = T;
    using allocator_type = std::allocator<value_type>;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using reference = value_type&;
    using const_reference = const value_type&;

    using pointer = std::allocator_traits<allocator_type>::pointer;
    using const_pointer = std::allocator_traits<allocator_type>::const_pointer;

    using iterator = DynArrayIterator<DynArray<value_type>>;
    using const_iterator = DynArrayConstIterator<DynArray<value_type>>;

private:
    T* m_first_ptr { nullptr };
    std::size_t m_size {};
    std::size_t m_capacity {};

    // It increases or decreases the amount of memory used and moves the existing T elements into
    // the new buffer
    void mem_realloc(std::size_t element_amount)
    {
        if (m_first_ptr == nullptr)
        {
            if (element_amount == 0)
            {
                std::cout << "The amounts of element to get memory for is 0 and there's no allocated buffer, so nothing will be done.\n";
                return;
            }

            m_capacity = element_amount;
            m_first_ptr = static_cast<T*>(::operator new(m_capacity * sizeof(T)));

            return;
        }

        if (element_amount == 0)
        {
            if (m_size > 0)
            {
                for (std::size_t i {}; i < m_size; i++)
                {
                    m_first_ptr[i].~T();
                }

                m_size = 0;
            }

            ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            m_capacity = 0;
            m_first_ptr = nullptr;
            return;
        }

        // If size is bigger than element_amount the remaining T objects will be discarded
        if (m_size > element_amount)
        {
            std::cout << "The size is bigger than amount of elements for reallocation. The remaining T objects will be discarded.\n";

            std::size_t old_capacity { m_capacity };
            m_capacity = element_amount;
            T* new_buffer { static_cast<T*>(::operator new(m_capacity * sizeof(T))) };

            for (std::size_t i {}; i < element_amount; i++)
            {
                new (&new_buffer[i]) T(std::move(m_first_ptr[i]));
            }

            for (std::size_t i {}; i < m_size; i++)
            {
                m_first_ptr[i].~T();
            }

            ::operator delete(m_first_ptr, old_capacity * sizeof(T));
            m_first_ptr = new_buffer;
        }
        else
        {
            // This last case is for when the DynArray is growing to a bigger buffer and capacity
            std::size_t old_capacity { m_capacity };
            m_capacity = element_amount;
            T* new_buffer { static_cast<T*>(::operator new(m_capacity * sizeof(T))) };

            if (m_size > 0)
            {
                for (std::size_t i {}; i < m_size; i++)
                {
                    new_buffer[i] = std::move(m_first_ptr[i]);
                }

                for (std::size_t i {}; i < m_size; i++)
                {
                    m_first_ptr[i].~T();
                }
            }

            ::operator delete(m_first_ptr, old_capacity * sizeof(T));
            m_first_ptr = new_buffer;
        }
    }

    // It increases the memory used by a factor of 2 for when the DynArray is full, or
    // just 1 block of memory of size of T if the DynArray had no memory at all
    void grow_by_2()
    {
        if (m_capacity == 0)
        {
            mem_realloc(1);
            return;
        }

        mem_realloc(m_capacity * 2);

        std::cout << "Growing the size";
    }

    // It changes old_ptr with nullptr and returns the previous value of old_ptr. It doesn't handle resources
    T* exchange_with_null(DynArray* old_object)
    {
        if (old_object->m_first_ptr == nullptr) { return old_object->m_first_ptr; }

        T* temp_ptr { &(*(old_object->m_first_ptr)) };
        old_object->m_first_ptr = nullptr;
        return temp_ptr;
    }

    std::size_t exchange_size(DynArray* old_object)
    {
        std::size_t temp_size { old_object->m_size };
        old_object->m_size = 0;
        return temp_size;
    }

    std::size_t exchange_capacity(DynArray* old_object)
    {
        std::size_t temp_capacity { old_object->m_capacity };
        old_object->m_capacity = 0;
        return temp_capacity;
    }

public:
    DynArray()
    {
        std::cout << "Default construction\n";
    }

    // It creates a DynArray with an "amount" number of default-initialized T objects
    explicit DynArray(std::size_t size)
    : m_size { size },
      m_capacity { size }
    {
        if (m_size > 0)
        {
            mem_realloc(m_capacity);

            for (std::size_t i {}; i < m_size; i++)
            {
                T* temp_ptr { m_first_ptr + i };
                temp_ptr = new (m_first_ptr + i) T();
            }
        }

        std::cout << "Size construction\n";
    }

    // It creates a DynArray with an "amount" number of copies of "element"
    explicit DynArray(std::size_t amount, const T& element)
    : m_size { amount },
      m_capacity { amount }
    {
        if (m_size > 0)
        {
            mem_realloc(m_capacity);

            for (std::size_t i {}; i < m_size; i++)
            {
                T* temp_ptr { m_first_ptr + i };
                temp_ptr = new (m_first_ptr + i) T(element);
                std::cout << "Element " << i << ": " << element <<'\n';
            }
        }

        std::cout << "Size and single element copy construction\n";
    }

    DynArray(const DynArray& other)
    : m_size { other.m_size },
      m_capacity { other.m_capacity }
    {
        if (m_capacity > 0)
        {
            mem_realloc(m_capacity);

            if (m_size > 0)
            {
                for (std::size_t i {}; i < m_size; i++)
                {
                    T* temp_ptr { m_first_ptr + i };
                    temp_ptr = new (m_first_ptr + i) T(other[i]);
                }
            }
        }

        std::cout << "Copy construction\n";

        std::cout << "Size: " << m_capacity << '\n';
        std::cout << "Capacity: " << m_capacity << '\n';
        std::cout << ((m_first_ptr == nullptr) ? "The buffer is nullptr\n" : "The buffer has memory assigned to it\n");
    }

    DynArray(std::initializer_list<T> other)
    : m_size { other.size() },
      m_capacity { other.size() }
    {
        if (m_capacity > 0)
        {
            mem_realloc(m_capacity);

            for (std::size_t i {}; i < m_size; i++)
            {
                T* temp_ptr { m_first_ptr + i };
                temp_ptr = new (m_first_ptr + i) T(other.begin()[i]);
            }
        }

        std::cout << "std::initializer_list construction\n";

        std::cout << "Size: " << m_capacity << '\n';
        std::cout << "Capacity: " << m_capacity << '\n';
        std::cout << ((m_first_ptr == nullptr) ? "The buffer is nullptr\n" : "The buffer has memory assigned to it\n");
    }

    DynArray(DynArray&& other) noexcept
    : m_first_ptr { exchange_with_null(&other) },
      m_size { exchange_size(&other) },
      m_capacity { exchange_capacity(&other) }
    {
        std::cout << "Move construction\n";
    }

    // No reallocations unless the other DynArray object is bigger in capacity
    DynArray& operator=(const DynArray& other)
    {
        for (std::size_t i {}; i < m_size; i++)
        {
            m_first_ptr[i].~T();
        }

        m_size = other.m_size;

        if (m_capacity < other.m_capacity)
        {
            ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            m_capacity = other.m_capacity;
            m_first_ptr = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
        }

        if (m_size > 0)
        {
            for (std::size_t i {}; i < m_size; i++)
            {
                T* temp_ptr { m_first_ptr + i };
                temp_ptr = new (m_first_ptr + i) T(other[i]);
            }
        }

        std::cout << "Copy assignment\n";
        return *this;
    }

    // No reallocations unless the other's size is bigger than the DynArray's capacity
    DynArray& operator=(std::initializer_list<T> other)
    {
        for (std::size_t i {}; i < m_size; i++)
        {
            m_first_ptr[i].~T();
        }

        m_size = other.size();

        if (m_capacity < other.size())
        {
            ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            m_capacity = other.size();
            m_first_ptr = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
        }

        if (m_size > 0)
        {
            for (std::size_t i {}; i < m_size; i++)
            {
                T* temp_ptr { m_first_ptr + i };
                temp_ptr = new (m_first_ptr + i) T(other.begin()[i]);
            }
        }

        std::cout << "std::initializer_list assignment\n";
        return *this;
    }

    DynArray& operator=(DynArray&& other) noexcept
    {
        m_first_ptr = other.m_first_ptr;
        other.m_first_ptr = nullptr;

        m_size = other.m_size;
        other.m_size = 0;

        m_capacity = other.m_capacity;
        other.m_capacity = 0;

        std::cout << "Move assignment\n";
        return *this;
    }

    // It calls the destructors for all T objects and resets size back to 0.
    // It doesn't deallocate the buffer
    void destroy_all()
    {
        for (std::size_t i {}; i < m_size; i++)
        {
            m_first_ptr[i].~T();
        }

        m_size = 0;
    }

    ~DynArray()
    {
        if (m_first_ptr != nullptr)
        {
            destroy_all();
            ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            m_first_ptr = nullptr;
            m_capacity = 0;
        }

        std::cout << "Destruction\n";
    }

    bool is_empty() const noexcept { return (m_size == 0); }

    bool is_full() const noexcept
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        return (m_size > 0 && m_size == m_capacity);
    }

    std::size_t size() const noexcept { return m_size; }

    std::size_t capacity() const noexcept { return m_capacity; }

    T* array_ptr() noexcept { return m_first_ptr; }

    const T* array_ptr() const noexcept { return m_first_ptr; }

    T& operator[](std::size_t position)
    {
        BASIC_ASSERT(!(is_empty()), "The DynArray is empty, you can't get elements from it.\n");
        BASIC_ASSERT((position < m_size), "The position must be smaller that the size of the DynArray.\n");

        return m_first_ptr[position];
    }

    const T& operator[](std::size_t position) const
    {
        BASIC_ASSERT(!(is_empty()), "The DynArray is empty, you can't get elements from it.\n");
        BASIC_ASSERT((position < m_size), "The position must be smaller that the size of the DynArray.\n");

        return m_first_ptr[position];
    }

    // It works the same as operator[] but it doesn't have bounds checking
    T& at_unchecked(const std::size_t position)
    {
        return m_first_ptr[position];
    }

    const T& at_unchecked(const std::size_t position) const
    {
        return m_first_ptr[position];
    }

    T& first()
    {
        BASIC_ASSERT(!is_empty(), "The DynArray is empty, you can't get the first element.\n");

        return m_first_ptr[0];
    }

    const T& first() const
    {
        BASIC_ASSERT(!is_empty(), "The DynArray is empty, you can't get the first element.\n");

        return m_first_ptr[0];
    }

    T& last()
    {
        BASIC_ASSERT(!is_empty(), "The DynArray is empty, you can't get the last element.\n");

        return m_first_ptr[m_size - 1];
    }

    const T& last() const
    {
        BASIC_ASSERT(!is_empty(), "The DynArray is empty, you can't get the last element.\n");

        return m_first_ptr[m_size - 1];
    }

    iterator begin()
    {
        return iterator(m_first_ptr);
    }

    iterator end()
    {
        return iterator(m_first_ptr + m_size);
    }

    const_iterator cbegin()
    {
        return const_iterator(m_first_ptr);
    }

    const_iterator cend()
    {
        return const_iterator(m_first_ptr + m_size);
    }

    void push_back(const T& t)
    {
        if (size() == std::numeric_limits<std::size_t>::max())
        {
            std::cout << "The DynArray has a number of elements that matches the limit of std::size_t, so new ones cannot be added.\n";
            return;
        }

        if (is_full())
        {
            std::cout << "The DynArray is full. Growing it up.\n";
            grow_by_2();
        }

        if (m_first_ptr == nullptr)
        {
            mem_realloc(1);
            m_first_ptr = new (m_first_ptr) T(t);
            m_size++;
            return;
        }

        T* temp_ptr { m_first_ptr + m_size };
        temp_ptr = new (m_first_ptr + m_size) T(t);
        m_size++;
    }

    void push_back(T&& t)
    {
        if (size() == std::numeric_limits<std::size_t>::max())
        {
            std::cout << "The DynArray has a number of elements that matches the limit of std::size_t, so new ones cannot be added.\n";
            return;
        }

        if (is_full())
        {
            std::cout << "The DynArray is full. Growing it up.\n";
            grow_by_2();
        }

        if (m_first_ptr == nullptr)
        {
            mem_realloc(1);
            m_first_ptr = new (m_first_ptr) T(std::move(t));
            m_size++;
            return;
        }

        T* temp_ptr { m_first_ptr + m_size };
        temp_ptr = new (m_first_ptr + m_size) T(std::move(t));
        m_size++;
    }

    // In-place construction, so no copy nor move operations for inserting the new T object
    // It's often faster unless reallocations are done
    template<typename... Args>
    T& emplace_back(Args&&... args)
    {
        BASIC_ASSERT((size() < std::numeric_limits<std::size_t>::max()), "The DynArray has a number of elements that matches the limit of std::size_t, so new ones cannot be added.\n");

        if (is_full())
        {
            std::cout << "The DynArray is full. Growing it up.\n";
            grow_by_2();
        }

        new (m_first_ptr + m_size) T(std::forward<Args>(args)...);
        m_size++;

        return m_first_ptr[m_size - 1];
    }

    void pop_back()
    {
        if (is_empty())
        {
            std::cout << "The DynArray is already empty, no elements will be popped out.\n";
            return;
        }

        m_size--;
        m_first_ptr[m_size].~T();
    }

    // Makes the DynArray bigger in capacity (memory being used) if the amount of
    // elements specified is bigger than the current capacity
    void reserve(std::size_t element_amount)
    {
        if (element_amount < m_capacity)
        {
            std::cout << "The amounts of element to reserve is inferior to the current capacity, so nothing will be done.\n";
            return;
        }

        mem_realloc(element_amount);
    }

    // Makes a reallocation to use a new smaller buffer just big enough to fit all the existing elements
    void shrink_to_size()
    {
        if (m_size == 0)
        {
            if (m_capacity == 0)
            {
                std::cout << "The size and capacity of the DynArray are already 0, so nothing will be done.\n";
                return;
            }

            mem_realloc(0);
            return;
        }

        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (is_full())
        {
            std::cout << "The DynArray is already using only the necessary memory to contain all its elements, so nothing will be done.\n";
            return;
        }

        mem_realloc(m_size);
    }

    // It deletes a single element at "position" and replaces it with a default-initialized T object
    void reset_single(std::size_t position)
    {
        if (position >= m_size)
        {
            std::cout << "The element to delete is on a position bigger than the size of the DynArray.\n";
            return;
        }

        m_first_ptr[position].~T();
        (m_first_ptr + position) = new (m_first_ptr + position) T();
    }

    // It deletes all the elements from "beginning" to "end", and replaces them with default-initialized T objects
    void reset_multiple(std::size_t beginning, std::size_t end)
    {
        if (end >= m_size)
        {
            std::cout << "The last element to delete is on a position bigger than the size of the DynArray.\n";
            return;
        }

        if (beginning > end)
        {
            std::cout << "The first position is bigger than the second one. Nothing will be done\n";
            return;
        }

        for (std::size_t i { beginning }; i <= end; i++)
        {
            m_first_ptr[i].~T();
            (m_first_ptr + i) = new (m_first_ptr + i) T();
        }
    }

    // It deletes all the elements in the DynArray, and replaces them with default-initialized T objects
    void reset_all()
    {
        if (m_size > 0)
        {
            std::size_t temp_size { m_size };
            destroy_all();
            m_size = temp_size;
        }

        for (std::size_t i {}; i < m_size; i++)
        {
            m_first_ptr[i].~T();
        }
    }

    // Unlike the other "reset" member functions, this one destroys all the T objects but doesn't replace them with new ones.
    // Also, it sets size and capacity to 0, and deallocates the buffer
    void reset_array()
    {
        if (m_size > 0)
        {
            destroy_all();
        }

        if (m_first_ptr != nullptr)
        {
            ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            m_first_ptr = nullptr;
        }

        m_capacity = 0;
    }

    friend std::ostream& operator <<(std::ostream& out, const DynArray& dyn)
    {
        if (dyn.is_empty())
        {
            out << "The DynArray is already empty, cannot print any elements.\n";
            return out;
        }

        out << "DynArray { ";

        for (std::size_t i {}; i < (dyn.size() - 1); i++)
        {
            out << dyn[i] << ", ";
        }

        out << dyn[dyn.size() - 1] << " }\n\n";

        return out;
    }
};

} // namespace hdsa end

#endif // DYN_ARRAY_HPP
