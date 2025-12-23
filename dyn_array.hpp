#ifndef DYN_ARRAY_HPP
#define DYN_ARRAY_HPP

#include <cstddef>
#include <limits>
#include <type_traits>
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

template<typename T>
class DynArray final
{
public:
    using value_type = T;
    using element_type = value_type;
    // using allocator_type = std::allocator<value_type>;

    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    using pointer = value_type*;
    using const_pointer = const value_type*;

    // using pointer = std::allocator_traits<allocator_type>::pointer;
    // using const_pointer = std::allocator_traits<allocator_type>::const_pointer;

    using reference = value_type&;
    using const_reference = const value_type&;

private:
    T* m_first_ptr { nullptr };
    std::size_t m_size {};
    std::size_t m_capacity {};


    struct Iterator final
    {
        using difference_type = std::ptrdiff_t;

        using value_type = DynArray::value_type;
        using element_type = value_type;

        using pointer = value_type*;

        using reference = value_type&;

        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::contiguous_iterator_tag;

    private:
        pointer m_ptr { nullptr };

    public:
        Iterator(pointer ptr)
        : m_ptr { ptr } {}

        pointer operator->()
        {
            return m_ptr;
        }

        reference operator*() const
        {
            return *m_ptr;
        }

        pointer data()
        {
            return m_ptr;
        }

        reference operator[](std::size_t position) const
        {
            return m_ptr[position];
        }

        Iterator& operator++()
        {
            ++m_ptr;
            return *this;
        }

        Iterator operator++(int)
        {
            Iterator iterator { *this };
            ++(*this);
            return iterator;
        }

        Iterator& operator--()
        {
            --m_ptr;
            return *this;
        }

        Iterator operator--(int)
        {
            Iterator iterator { *this };
            --(*this);
            return iterator;
        }

        Iterator& operator+=(int x)
        {
            m_ptr += x;
            return *this;
        }

        Iterator& operator-=(int x)
        {
            m_ptr -= x;
            return *this;
        }

        Iterator operator+(const difference_type x) const
        {
            return Iterator { m_ptr + x };
        }

        friend Iterator operator+(const difference_type x, const Iterator& other)
        {
            return other + x;
        }

        Iterator operator-(const difference_type x) const
        {
            return Iterator { m_ptr - x };
        }

        friend Iterator operator-(const difference_type x, const Iterator& other)
        {
            return other - x;
        }

        difference_type operator-(const Iterator& other) const
        {
            return m_ptr - other.m_ptr;
        }

        friend bool operator==(const Iterator& a, const Iterator& other)
        {
            return (a.m_ptr == other.m_ptr);
        }

        friend bool operator!=(const Iterator& a, const Iterator& other)
        {
            return (a.m_ptr != other.m_ptr);
        }

        friend bool operator<(const Iterator& a, const Iterator& other)
        {
            return (a.m_ptr < other.m_ptr);
        }

        friend bool operator>(const Iterator& a, const Iterator& other)
        {
            return (a.m_ptr > other.m_ptr);
        }

        friend bool operator<=(const Iterator& a, const Iterator& other)
        {
            return (a.m_ptr <= other.m_ptr);
        }

        friend bool operator>=(const Iterator& a, const Iterator& other)
        {
            return (a.m_ptr >= other.m_ptr);
        }

        auto operator<=>(const Iterator& other) const = default;

        friend std::ostream& operator<<(std::ostream& out, const Iterator& it)
        {
            out << it.m_ptr;
            return out;
        }
    };

    struct ConstIterator final
    {
        using difference_type = std::ptrdiff_t;

        using value_type = DynArray::value_type;
        using element_type = value_type;

        using pointer = const value_type*;

        using reference = const value_type&;

        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::contiguous_iterator_tag;

    private:
        pointer m_ptr { nullptr };

    public:
        ConstIterator(pointer ptr)
        : m_ptr { ptr } {}

        pointer operator->()
        {
            return m_ptr;
        }

        reference operator*() const
        {
            return *m_ptr;
        }

        pointer data()
        {
            return m_ptr;
        }

        reference operator[](std::size_t position) const
        {
            return m_ptr[position];
        }

        ConstIterator& operator++()
        {
            ++m_ptr;
            return *this;
        }

        ConstIterator operator++(int)
        {
            ConstIterator iterator { *this };
            ++(*this);
            return iterator;
        }

        ConstIterator& operator--()
        {
            --m_ptr;
            return *this;
        }

        ConstIterator operator--(int)
        {
            ConstIterator iterator { *this };
            --(*this);
            return iterator;
        }

        ConstIterator& operator+=(int x)
        {
            m_ptr += x;
            return *this;
        }

        ConstIterator& operator-=(int x)
        {
            m_ptr -= x;
            return *this;
        }

        ConstIterator operator+(const difference_type x) const
        {
            return m_ptr + x;
        }

        friend ConstIterator operator+(const difference_type x, const ConstIterator& other)
        {
            return other + x;
        }

        ConstIterator operator-(const difference_type x) const
        {
            return m_ptr - x;
        }

        friend ConstIterator operator-(const difference_type x, const ConstIterator& other)
        {
            return other - x;
        }

        difference_type operator-(const ConstIterator& other) const
        {
            return m_ptr - other.m_ptr;
        }

        friend bool operator==(const ConstIterator& a, const ConstIterator& other)
        {
            return (a.m_ptr == other.m_ptr);
        }

        friend bool operator!=(const ConstIterator& a, const ConstIterator& other)
        {
            return (a.m_ptr != other.m_ptr);
        }

        friend bool operator<(const ConstIterator& a, const ConstIterator& other)
        {
            return (a.m_ptr < other.m_ptr);
        }

        friend bool operator>(const ConstIterator& a, const ConstIterator& other)
        {
            return (a.m_ptr > other.m_ptr);
        }

        friend bool operator<=(const ConstIterator& a, const ConstIterator& other)
        {
            return (a.m_ptr <= other.m_ptr);
        }

        friend bool operator>=(const ConstIterator& a, const ConstIterator& other)
        {
            return (a.m_ptr >= other.m_ptr);
        }

        auto operator<=>(const ConstIterator& other) const = default;

        friend std::ostream& operator<<(std::ostream& out, const ConstIterator& c_it)
        {
            out << c_it.m_ptr;
            return out;
        }
    };

    struct ReverseIterator final
    {
        using difference_type = std::ptrdiff_t;

        using value_type = DynArray::value_type;
        using element_type = value_type;

        using pointer = value_type*;

        using reference = value_type&;

        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::contiguous_iterator_tag;

    private:
        pointer m_ptr { nullptr };

    public:
        ReverseIterator(pointer ptr)
        : m_ptr { ptr } {}

        pointer operator->()
        {
            return m_ptr;
        }

        reference operator*() const
        {
            return *m_ptr;
        }

        pointer data()
        {
            return m_ptr;
        }

        reference operator[](std::size_t position) const
        {
            return m_ptr[position];
        }

        ReverseIterator& operator++()
        {
            --m_ptr;
            return *this;
        }

        ReverseIterator operator++(int)
        {
            ReverseIterator iterator { *this };
            --(*this);
            return iterator;
        }

        ReverseIterator& operator--()
        {
            ++m_ptr;
            return *this;
        }

        ReverseIterator operator--(int)
        {
            ReverseIterator iterator { *this };
            ++(*this);
            return iterator;
        }

        ReverseIterator& operator+=(int x)
        {
            m_ptr -= x;
            return *this;
        }

        ReverseIterator& operator-=(int x)
        {
            m_ptr += x;
            return *this;
        }

        ReverseIterator operator+(const difference_type x) const
        {
            return m_ptr - x;
        }

        friend ReverseIterator operator+(const difference_type x, const ReverseIterator& other)
        {
            return other - x;
        }

        ReverseIterator operator-(const difference_type x) const
        {
            return m_ptr + x;
        }

        friend ReverseIterator operator-(const difference_type x, const ReverseIterator& other)
        {
            return other + x;
        }

        difference_type operator-(const ReverseIterator& other) const
        {
            return m_ptr - other.m_ptr;
        }

        friend bool operator==(const ReverseIterator& a, const ReverseIterator& other)
        {
            return (a.m_ptr == other.m_ptr);
        }

        friend bool operator!=(const ReverseIterator& a, const ReverseIterator& other)
        {
            return (a.m_ptr != other.m_ptr);
        }

        friend bool operator<(const ReverseIterator& a, const ReverseIterator& other)
        {
            return (a.m_ptr < other.m_ptr);
        }

        friend bool operator>(const ReverseIterator& a, const ReverseIterator& other)
        {
            return (a.m_ptr > other.m_ptr);
        }

        friend bool operator<=(const ReverseIterator& a, const ReverseIterator& other)
        {
            return (a.m_ptr <= other.m_ptr);
        }

        friend bool operator>=(const ReverseIterator& a, const ReverseIterator& other)
        {
            return (a.m_ptr >= other.m_ptr);
        }

        auto operator<=>(const ReverseIterator& other) const = default;

        friend std::ostream& operator<<(std::ostream& out, const ReverseIterator& rit)
        {
            out << rit.m_ptr;
            return out;
        }
    };

    struct ConstReverseIterator final
    {
        using difference_type = std::ptrdiff_t;

        using value_type = DynArray::value_type;
        using element_type = value_type;

        using pointer = const value_type*;

        using reference = const value_type&;

        using iterator_category = std::random_access_iterator_tag;
        using iterator_concept = std::contiguous_iterator_tag;

    private:
        pointer m_ptr { nullptr };

    public:
        ConstReverseIterator(pointer ptr)
        : m_ptr { ptr } {}

        pointer operator->()
        {
            return m_ptr;
        }

        reference operator*() const
        {
            return *m_ptr;
        }

        pointer data()
        {
            return m_ptr;
        }

        reference operator[](std::size_t position) const
        {
            return m_ptr[position];
        }

        ConstReverseIterator& operator++()
        {
            --m_ptr;
            return *this;
        }

        ConstReverseIterator operator++(int)
        {
            ConstReverseIterator iterator { *this };
            --(*this);
            return iterator;
        }

        ConstReverseIterator& operator--()
        {
            ++m_ptr;
            return *this;
        }

        ConstReverseIterator operator--(int)
        {
            ConstReverseIterator iterator { *this };
            ++(*this);
            return iterator;
        }

        ConstReverseIterator& operator+=(int x)
        {
            m_ptr -= x;
            return *this;
        }

        ConstReverseIterator& operator-=(int x)
        {
            m_ptr += x;
            return *this;
        }

        ConstReverseIterator operator+(const difference_type x) const
        {
            return m_ptr - x;
        }

        friend ConstReverseIterator operator+(const difference_type x, const ConstReverseIterator& other)
        {
            return other - x;
        }

        ConstReverseIterator operator-(const difference_type x) const
        {
            return m_ptr + x;
        }

        friend ConstReverseIterator operator-(const difference_type x, const ConstReverseIterator& other)
        {
            return other + x;
        }

        difference_type operator-(const ConstReverseIterator& other) const
        {
            return m_ptr - other.m_ptr;
        }

        friend bool operator==(const ConstReverseIterator& a, const ConstReverseIterator& other)
        {
            return (a.m_ptr == other.m_ptr);
        }

        friend bool operator!=(const ConstReverseIterator& a, const ConstReverseIterator& other)
        {
            return (a.m_ptr != other.m_ptr);
        }

        friend bool operator<(const ConstReverseIterator& a, const ConstReverseIterator& other)
        {
            return (a.m_ptr < other.m_ptr);
        }

        friend bool operator>(const ConstReverseIterator& a, const ConstReverseIterator& other)
        {
            return (a.m_ptr > other.m_ptr);
        }

        friend bool operator<=(const ConstReverseIterator& a, const ConstReverseIterator& other)
        {
            return (a.m_ptr <= other.m_ptr);
        }

        friend bool operator>=(const ConstReverseIterator& a, const ConstReverseIterator& other)
        {
            return (a.m_ptr >= other.m_ptr);
        }

        auto operator<=>(const ConstReverseIterator& other) const = default;

        friend std::ostream& operator<<(std::ostream& out, const ConstReverseIterator& rit)
        {
            out << rit.m_ptr;
            return out;
        }
    };

public:
    using iterator = Iterator;
    using const_iterator = ConstIterator;
    using reverse_iterator = ReverseIterator;
    using const_reverse_iterator = ConstReverseIterator;

private:
    // It increases or decreases the amount of memory used and moves the existing T elements into
    // the new buffer
    void mem_realloc(std::size_t element_amount)
    {
        if (m_first_ptr == nullptr)
        {
            if (element_amount == 0)
            {
                std::cout << "The amounts of element to get memory for is 0 and there's no allocated buffer, so the buffer won't change.\n";
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

        std::size_t old_capacity { m_capacity };
        m_capacity = element_amount;
        T* new_buffer { static_cast<T*>(::operator new(m_capacity * sizeof(T))) };

        // If size is bigger than element_amount the remaining T objects will be discarded
        if (m_size > element_amount)
        {
            std::cout << "The size is bigger than amount of elements for reallocation. The remaining T objects will be discarded.\n";

            for (std::size_t i {}; i < element_amount; i++)
            {
                new (&new_buffer[i]) T(std::move_if_noexcept(m_first_ptr[i]));
            }

            for (std::size_t i {}; i < m_size; i++)
            {
                m_first_ptr[i].~T();
            }
        }
        // This last case is for when the DynArray is growing to a bigger buffer and capacity
        else
        {
            if (m_size > 0)
            {
                for (std::size_t i {}; i < m_size; i++)
                {
                    new (&new_buffer[i]) T(std::move_if_noexcept(m_first_ptr[i]));
                }

                for (std::size_t i {}; i < m_size; i++)
                {
                    m_first_ptr[i].~T();
                }
            }
        }

        ::operator delete(m_first_ptr, old_capacity * sizeof(T));
        m_first_ptr = new_buffer;
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
        T* temp_ptr { old_object->m_first_ptr };
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
                new (m_first_ptr + i) T();
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
                new (m_first_ptr + i) T(element);
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
                    new (m_first_ptr + i) T(other[i]);
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
                new (m_first_ptr + i) T(other.begin()[i]);
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
            if (m_first_ptr != nullptr)
            {
                ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            }

            m_capacity = other.m_capacity;
            m_first_ptr = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
        }

        if (m_size > 0)
        {
            for (std::size_t i {}; i < m_size; i++)
            {
                new (m_first_ptr + i) T(other[i]);
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
            if (m_first_ptr != nullptr)
            {
                ::operator delete(m_first_ptr, m_capacity * sizeof(T));
            }

            m_capacity = other.size();
            m_first_ptr = static_cast<T*>(::operator new(m_capacity * sizeof(T)));
        }

        if (m_size > 0)
        {
            for (std::size_t i {}; i < m_size; i++)
            {
                new (m_first_ptr + i) T(other.begin()[i]);
            }
        }

        std::cout << "std::initializer_list assignment\n";
        return *this;
    }

    DynArray& operator=(DynArray&& other) noexcept
    {
        for (std::size_t i {}; i < m_size; i++)
        {
            m_first_ptr[i].~T();
        }

        m_size = other.m_size;
        other.m_size = 0;

        if (m_first_ptr != nullptr)
        {
            ::operator delete(m_first_ptr, m_capacity * sizeof(T));
        }

        m_capacity = other.m_capacity;
        other.m_capacity = 0;

        m_first_ptr = other.m_first_ptr;
        other.m_first_ptr = nullptr;

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

        return ((m_size > 0) && (m_size == m_capacity));
    }

    bool has_memory() { return (m_first_ptr != nullptr); }

    std::size_t size() const noexcept { return m_size; }

    std::size_t capacity() const noexcept { return m_capacity; }

    T* array_ptr() const noexcept { return m_first_ptr; }

    T& operator[](std::size_t position)
    {
        return m_first_ptr[position];
    }

    const T& operator[](std::size_t position) const
    {
        return m_first_ptr[position];
    }

    // It works the same as operator[] but it has bounds checking
    T& at_checked(const std::size_t position)
    {
        BASIC_ASSERT(!(is_empty()), "The DynArray is empty, you can't get elements from it.\n");
        BASIC_ASSERT((position < m_size), "The position must be a positive number and not bigger than the size of the DynArray.\n");

        return m_first_ptr[position];
    }

    const T& at_checked(const std::size_t position) const
    {
        BASIC_ASSERT(!(is_empty()), "The DynArray is empty, you can't get elements from it.\n");
        BASIC_ASSERT((position < m_size), "The position must be a positive number and not bigger than the size of the DynArray.\n");

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

    // Increases the buffer and capacity
    void reserve_memory(std::size_t element_amount)
    {
        BASIC_ASSERT((capacity() < std::numeric_limits<std::size_t>::max()), "The DynArray has a capacity that matches the limit of std::size_t, so it cannot grow any further.\n");

        if (element_amount <= m_capacity)
        {
            std::cout << "The amounts of element to reserve is inferior or equal to the current capacity, so reserve_memory() will do nothing.\n";
            return;
        }

        mem_realloc(element_amount);
    }

    void push_back(const T& t)
    {
        if (size() == std::numeric_limits<std::size_t>::max())
        {
            std::cout << "The DynArray has a number of elements that matches the limit of std::size_t, so new ones cannot be added.\n";
            return;
        }

        if (m_first_ptr == nullptr)
        {
            mem_realloc(1);
            new (m_first_ptr) T(t);
            m_size++;
            return;
        }

        if (is_full())
        {
            std::cout << "The DynArray is full. Growing it up.\n";
            grow_by_2();
        }

        new (m_first_ptr + m_size) T(t);
        m_size++;
    }

    void push_back(T&& t)
    {
        if (size() == std::numeric_limits<std::size_t>::max())
        {
            std::cout << "The DynArray has a number of elements that matches the limit of std::size_t, so new ones cannot be added.\n";
            return;
        }

        if (m_first_ptr == nullptr)
        {
            mem_realloc(1);
            new (m_first_ptr) T(std::move_if_noexcept(t));
            m_size++;
            return;
        }

        if (is_full())
        {
            std::cout << "The DynArray is full. Growing it up.\n";
            grow_by_2();
        }

        new (m_first_ptr + m_size) T(std::move_if_noexcept(t));
        m_size++;
    }

    // In-place construction, so no copy nor move operations for inserting the new T object
    // It's often faster unless reallocations are done
    template<typename... Args>
    T& emplace_back(Args&&... args)
    {
        BASIC_ASSERT((size() < std::numeric_limits<std::size_t>::max()), "The DynArray has a number of elements that matches the limit of std::size_t, so new ones cannot be added.\n");

        if (m_first_ptr == nullptr)
        {
            mem_realloc(1);
        }

        if (is_full())
        {
            std::cout << "The DynArray is full. Growing it up.\n";
            grow_by_2();
        }

        new (m_first_ptr + m_size) T(std::forward<Args>(args)...);
        m_size++;

        std::cout << "Pushing one element with in-place construction.\n";

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

    // Changes the size of the DynArray and creates default-constructed T objects if element_amount
    // is bigger than the DynArray size in the remaining spots.
    // It will reallocate if element_ammount is bigger than the capacity of the DynArray
    void resize(std::size_t element_amount)
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (m_capacity < element_amount)
        {
            mem_realloc(element_amount);
        }

        if (m_size == element_amount)
        {
            std::cout << "The current DynArray size and the new one are the same, so no resizing will happen.\n";
            return;
        }

        if (m_size < element_amount)
        {
            for (std::size_t i { m_size }; i < element_amount; i++)
            {
                new(m_first_ptr + i) T();
            }
        }
        else
        {
            for (std::size_t i { element_amount }; i < m_size; i++)
            {
                m_first_ptr[i].~T();
            }
        }

        m_size = element_amount;
    }

    // Changes the size of the DynArray and creates copies of "value" T objects if element_amount
    // is bigger than the DynArray size in the remaining spots.
    // It will reallocate if element_ammount is bigger than the capacity of the DynArray
    void resize(std::size_t element_amount, const T& value)
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (m_capacity < element_amount)
        {
            mem_realloc(element_amount);
        }

        if (m_size == element_amount)
        {
            std::cout << "The current DynArray size and the new one are the same, so no resizing will happen.\n";
            return;
        }

        if (m_size < element_amount)
        {
            for (std::size_t i { m_size }; i < element_amount; i++)
            {
                new(m_first_ptr + i) T(value);
            }
        }
        else
        {
            for (std::size_t i { element_amount }; i < m_size; i++)
            {
                m_first_ptr[i].~T();
            }
        }

        m_size = element_amount;
    }

    // Makes a reallocation to use a new smaller buffer just big enough to fit all the existing elements
    void shrink_to_size()
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (m_size == 0)
        {
            mem_realloc(0);
            return;
        }

        if (is_full())
        {
            std::cout << "The DynArray is already using only the necessary memory to contain all its elements, so nothing will be done.\n";
            return;
        }

        mem_realloc(m_size);
    }

    // It deletes a single element at "position" and replaces it with a default-initialized T object
    // position can go from 0 to (size() - 1)
    void reset_single(std::size_t position)
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (m_first_ptr == nullptr)
        {
            std::cout << "There's no buffer, so no elements can be reset.\n";
            return;
        }

        if (position >= m_size)
        {
            std::cout << "The element to delete is on a position bigger than the size of the DynArray.\n";
            return;
        }

        m_first_ptr[position].~T();
        new (m_first_ptr + position) T();
    }

    // It deletes all the elements from "beginning" to "end", and replaces them with default-initialized T objects
    // The range for both parameters can go from 0 to (size() - 1), and using the same number for both resets only one T object
    void reset_multiple(std::size_t beginning, std::size_t end)
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (m_first_ptr == nullptr)
        {
            std::cout << "There's no buffer, so no elements can be reset.\n";
            return;
        }

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
            new (m_first_ptr + i) T();
        }
    }

    // It deletes all the elements in the DynArray, and replaces them with default-initialized T objects
    void reset_all()
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

        if (m_first_ptr == nullptr)
        {
            std::cout << "There's no buffer, so no elements can be reset.\n";
            return;
        }

        if (m_size == 0)
        {
            std::cout << "The DynArray is empty, no elements can be reset.\n";
            return;
        }

        std::size_t temp_size { m_size };
        destroy_all();
        m_size = temp_size;

        for (std::size_t i {}; i < m_size; i++)
        {
            m_first_ptr[i].~T();
            new (m_first_ptr + i) T();
        }
    }

    // Unlike the other "reset" member functions, this one destroys all the T objects but doesn't replace them with new ones.
    // Also, it sets size and capacity to 0, and deallocates the buffer
    void reset_array()
    {
        BASIC_ASSERT((m_size <= m_capacity), "The size of the DynArray is bigger than its capacity!\n");

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
            out << "The DynArray is empty, cannot print any elements.\n";
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

    iterator begin()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return iterator(m_first_ptr);
    }

    iterator end()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return iterator(m_first_ptr + m_size);
    }

    const_iterator cbegin()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return const_iterator(m_first_ptr);
    }

    const_iterator cend()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return const_iterator(m_first_ptr + m_size);
    }

    reverse_iterator rbegin()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return reverse_iterator(m_first_ptr + (m_size - 1));
    }

    reverse_iterator rend()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return reverse_iterator(m_first_ptr - 1);
    }

    const_reverse_iterator crbegin()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return const_reverse_iterator(m_first_ptr + (m_size - 1));
    }

    const_reverse_iterator crend()
    {
        BASIC_ASSERT((m_first_ptr != nullptr), "The DynArray has no memory assigned to it, no iterators can be made from it.\n");

        return const_reverse_iterator(m_first_ptr - 1);
    }
};

} // namespace hdsa end

#endif // DYN_ARRAY_HPP
